#include <Arduino.h>
#include <arduino_freertos.h>
#include <semphr.h>
#include <queue.h>
#include <task.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "FileSystemManager.h"
#include "AlarmHistory.h"
#include "DiagLog.h"
#include <stddef.h>
#include <stdint.h>
#include "AlarmManager.h"

extern bool g_fileSystemReady;
extern SemaphoreHandle_t fileSystemMutex;

static const char* DIR_PATH  = "/Alarm_Logs";
static const char* LOG_PATH  = "/Alarm_Logs/alarm_history.ndjson";
static const char* TMP_PATH  = "/Alarm_Logs/alarm_history.tmp";

static const uint8_t MAX_GROUPS = 40;
static const uint8_t MAX_DUPES  = 10;   // duplicates under the newest (so total per group = 1 + MAX_DUPES)

struct Rec {
  uint32_t id;
  uint32_t epoch;
  char detail[140];
};

struct Group {
  bool used = false;
  uint8_t sev = 0;
  uint16_t code = 0;
  uint8_t action = 0;
  char baseDetail[140] = {0};  // Normalized for grouping

  uint8_t count = 0;           // how many recs used in recs[]
  Rec recs[1 + MAX_DUPES];     // recs[0] newest, then older
  uint32_t latestEpoch = 0;
};

static Group s_groups[MAX_GROUPS];
static uint32_t s_nextId = 1;

static SemaphoreHandle_t s_histMutex = nullptr;

struct Msg {
  uint32_t id;
  uint32_t epoch;
  uint8_t sev;
  uint16_t code;
  uint8_t action;
  char detail[140];
};

static QueueHandle_t s_q = nullptr;
static TaskHandle_t  s_task = nullptr;

static bool s_dirtyNeedsPersist = false;

static void ensureMutex() {
  if (!s_histMutex) s_histMutex = xSemaphoreCreateMutex();
}

void jsonEscapePrint(Print& out, const char* s) {
  for (const char* p=s; p && *p; ++p) {
    if (*p=='\\' || *p=='"') out.print('\\');
    out.print(*p);
  }
}

static void normalizeDetailKey(const char* in, char* out, size_t outSize) {
  if (!out || outSize == 0) return;
  out[0] = '\0';
  if (!in) return;

  const char* cut = strchr(in, '(');              // cut off dynamic "(...)" portion
  size_t n = cut ? (size_t)(cut - in) : strlen(in);

  if (n >= outSize) n = outSize - 1;
  memcpy(out, in, n);
  out[n] = '\0';

  // trim trailing whitespace
  while (n > 0 && (out[n - 1] == ' ' || out[n - 1] == '\t')) {
    out[--n] = '\0';
  }
}

static void normalizeDetailForGrouping(const char* in, char* out, size_t outSz) {
  if (!out || outSz == 0) return;
  out[0] = '\0';
  if (!in) return;

  // Add patterns here as needed
  static const char* kPrefixes[] = {
    "Collector freeze cycle restart",
    "Collector freeze cycle start",
    "Circ freeze cycle restart",
    "Circ freeze cycle start",
    "Tank freeze protect started",
    "Tank freeze protect ended",
    "Line freeze cycle start",
    "Line freeze cycle restart"  // Add for new combined
  };

  for (size_t i = 0; i < (sizeof(kPrefixes) / sizeof(kPrefixes[0])); i++) {
    const char* p = kPrefixes[i];
    const size_t n = strlen(p);
    if (strncmp(in, p, n) == 0) {
      strncpy(out, p, outSz - 1);
      out[outSz - 1] = '\0';
      return;
    }
  }

  // Default: keep original detail
  strncpy(out, in, outSz - 1);
  out[outSz - 1] = '\0';
}

static bool groupMatches(const Group& g, uint8_t sev, uint16_t code, uint8_t action, const char* detail) {
  if (!g.used) return false;
  if (g.sev != sev || g.code != code || g.action != action) return false;

  char key[sizeof(g.baseDetail)];
  normalizeDetailForGrouping(detail ? detail : "", key, sizeof(key));

  return (strncmp(g.baseDetail, key, sizeof(g.baseDetail)) == 0);
}

static int findGroup(uint8_t sev, uint16_t code, uint8_t action, const char* detail) {
  for (int i=0;i<MAX_GROUPS;i++){
    if (groupMatches(s_groups[i], sev, code, action, detail)) return i;
  }
  return -1;
}

static int findFreeSlot() {
  for (int i=0;i<MAX_GROUPS;i++){
    if (!s_groups[i].used) return i;
  }
  return -1;
}

static int findOldestGroupIndex() {
  int idx = -1;
  uint32_t best = 0xFFFFFFFF;
  for (int i=0;i<MAX_GROUPS;i++){
    if (!s_groups[i].used) continue;
    if (s_groups[i].latestEpoch < best) {
      best = s_groups[i].latestEpoch;
      idx = i;
    }
  }
  return idx;
}

static void insertRecordIntoGroup(Group& g, uint32_t id, uint32_t epoch, const char* fullDetail) {
  // shift right by 1
  const uint8_t maxCount = (uint8_t)(1 + MAX_DUPES);
  uint8_t newCount = g.count < maxCount ? (g.count + 1) : maxCount;

  for (int i=(int)newCount-1; i>=1; --i) {
    g.recs[i] = g.recs[i-1];
  }
  g.recs[0].id = id;
  g.recs[0].epoch = epoch;
  strncpy(g.recs[0].detail, fullDetail ? fullDetail : "", sizeof(g.recs[0].detail)-1);
  g.recs[0].detail[sizeof(g.recs[0].detail)-1] = '\0';

  g.count = newCount;
  g.latestEpoch = epoch;
}

static void applyRecord(uint32_t id, uint32_t epoch, uint8_t sev, uint16_t code, uint8_t action, const char* fullDetail) {
  int gi = findGroup(sev, code, action, fullDetail);
  if (gi < 0) {
    int slot = findFreeSlot();
    if (slot < 0) {
      slot = findOldestGroupIndex();  // overwrite oldest group
      if (slot < 0) return;           // should never happen
    }
    Group& g = s_groups[slot];
    g = {}; // Value-initialize to zero/false
    g.used = true;
    g.sev = sev;
    g.code = code;
    g.action = action;
    normalizeDetailForGrouping(fullDetail ? fullDetail : "", g.baseDetail, sizeof(g.baseDetail));
    g.count = 0;
    g.latestEpoch = 0;
    insertRecordIntoGroup(g, id, epoch, fullDetail);
  } else {
    insertRecordIntoGroup(s_groups[gi], id, epoch, fullDetail);
  }
}

static bool persistUnlocked() {
  if (!g_fileSystemReady) return false;

  if (!takeFileSystemMutexWithRetry("[AlarmHistory] persist",
                                    pdMS_TO_TICKS(2000), 3)) {
    return false;
  }

  if (!FlashFS.exists(DIR_PATH)) FlashFS.mkdir(DIR_PATH);

  // Rewrite file atomically:
  if (FlashFS.exists(TMP_PATH)) FlashFS.remove(TMP_PATH);

  File f = FlashFS.open(TMP_PATH, FILE_WRITE);
  if (!f) { xSemaphoreGive(fileSystemMutex); return false; }

  // Write all records in group order (newest first per group)
  // This keeps file bounded to <= 40*(1+10) lines.
  for (int i=0;i<MAX_GROUPS;i++){
    if (!s_groups[i].used) continue;
    Group& g = s_groups[i];
    for (int r=0; r<g.count; r++){
      f.print("{\"id\":"); f.print(g.recs[r].id);
      f.print(",\"ts\":"); f.print(g.recs[r].epoch);
      f.print(",\"sev\":"); f.print(g.sev);
      f.print(",\"code\":"); f.print(g.code);
      f.print(",\"act\":"); f.print(g.action);
      f.print(",\"msg\":\""); jsonEscapePrint(f, g.recs[r].detail); f.print("\"}\n");
    }
  }
  f.close();

  // Replace old file
  if (FlashFS.exists(LOG_PATH)) FlashFS.remove(LOG_PATH);
  bool ok = FlashFS.rename(TMP_PATH, LOG_PATH);

  xSemaphoreGive(fileSystemMutex);
  return ok;
}

static void loadFromFSUnlocked() {
  if (!g_fileSystemReady) return;

  if (!takeFileSystemMutexWithRetry("[AlarmHistory] load",
                                    pdMS_TO_TICKS(2000), 3)) {
    return;
  }

  if (!FlashFS.exists(LOG_PATH)) {
    xSemaphoreGive(fileSystemMutex);
    return;
  }

  File f = FlashFS.open(LOG_PATH, FILE_READ);
  if (!f) { xSemaphoreGive(fileSystemMutex); return; }

  char line[256];
  while (f.available()) {
    size_t n = f.readBytesUntil('\n', line, sizeof(line)-1);
    line[n] = '\0';
    if (n < 10) continue;

    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, line);
    if (err) continue;

    uint32_t id   = doc["id"]  | 0;
    uint32_t ts   = doc["ts"]  | 0;
    uint8_t  sev  = doc["sev"] | 0;
    uint16_t code = doc["code"]| 0;
    uint8_t  act  = doc["act"] | 0;
    const char* msg = doc["msg"] | "";

    if (id >= s_nextId) s_nextId = id + 1;
    applyRecord(id, ts, sev, code, act, msg);
  }

  f.close();
  xSemaphoreGive(fileSystemMutex);
}

static void AlarmHistory_sink(const AlarmEvent* e) {
  if (!e || !s_q) return;
  Msg m;
  m.id = s_nextId++;          // monotonic id
  m.epoch = e->epoch;
  m.sev = (uint8_t)e->sev;
  m.code = (uint16_t)e->code;
  m.action = (uint8_t)e->action;
  strncpy(m.detail, e->detail, sizeof(m.detail)-1);
  m.detail[sizeof(m.detail)-1] = '\0';

  // Non-blocking enqueue (drop if full)
  xQueueSend(s_q, &m, 0);
}

static void AlarmHistory_task(void*) {
  for (;;) {
    Msg m;
    if (xQueueReceive(s_q, &m, portMAX_DELAY) == pdTRUE) {
      ensureMutex();
      if (xSemaphoreTake(s_histMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        applyRecord(m.id, m.epoch, m.sev, m.code, m.action, m.detail);
        s_dirtyNeedsPersist = true;

        // Persist immediately if FS is ready (events are rare; simplest + safest)
        if (g_fileSystemReady) {
          persistUnlocked();
          s_dirtyNeedsPersist = false;
        }

        xSemaphoreGive(s_histMutex);
      }
    }
  }
}

void AlarmHistory_begin() {
  ensureMutex();
  if (!s_q) s_q = xQueueCreate(16, sizeof(Msg));
  if (!s_task) xTaskCreate(AlarmHistory_task, "AlarmHistory", 4096, nullptr, 1, &s_task);

  // Subscribe AlarmManager -> AlarmHistory
  AlarmManager_setEventSink(AlarmHistory_sink);
}

void AlarmHistory_onFileSystemReady() {
  ensureMutex();
  if (xSemaphoreTake(s_histMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    // Load persisted state
    for (auto& group : s_groups) {
      group = {};
    }
    s_nextId = 1;
    loadFromFSUnlocked();

    // If we had buffered events before FS ready, ensure we persist soon.
    if (s_dirtyNeedsPersist) {
      persistUnlocked();
      s_dirtyNeedsPersist = false;
    }

    xSemaphoreGive(s_histMutex);
  }
}

static void collectUsedGroupIndices(int* idx, int& n) {
  n = 0;
  for (int i=0;i<MAX_GROUPS;i++){
    if (s_groups[i].used) idx[n++] = i;
  }
  // sort by latestEpoch desc (small bubble sort: n <= 40)
  for (int a=0;a<n;a++){
    for (int b=a+1;b<n;b++){
      if (s_groups[idx[b]].latestEpoch > s_groups[idx[a]].latestEpoch) {
        int t = idx[a]; idx[a] = idx[b]; idx[b] = t;
      }
    }
  }
}

void AlarmHistory_writeJson(Print& out) {
  ensureMutex();
  if (xSemaphoreTake(s_histMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    // Active alarms
    out.print("{\"active\":");
    AlarmManager_writeActiveJson(out);
    out.print(",\"history\":[");
    int idx[MAX_GROUPS];
    int n;
    collectUsedGroupIndices(idx, n);
    for (int i=0;i<n;i++){
      if (i>0) out.print(",");
      Group& g = s_groups[idx[i]];
      out.print("{\"sev\":"); out.print(g.sev);
      out.print(",\"code\":"); out.print(g.code);
      out.print(",\"action\":"); out.print(g.action);
      out.print(",\"msg\":\""); jsonEscapePrint(out, g.recs[0].detail); out.print("\"");
      out.print(",\"ts\":"); out.print(g.recs[0].epoch);
      out.print(",\"count\":"); out.print(g.count);
      out.print(",\"id\":"); out.print(g.recs[0].id);
      out.print("}");
    }
    out.print("]}");
    xSemaphoreGive(s_histMutex);
  }
}

bool AlarmHistory_deleteIds(const uint32_t* ids, size_t nIds) {
  if (nIds == 0) return true;

  ensureMutex();
  if (xSemaphoreTake(s_histMutex, pdMS_TO_TICKS(200)) != pdTRUE) return false;

  bool changed = false;

  for (size_t j=0;j<nIds;j++){
    uint32_t targetId = ids[j];

    for (int gi=0;gi<MAX_GROUPS;gi++){
      Group& g = s_groups[gi];
      if (!g.used) continue;

      for (int ri=0;ri<g.count;ri++){
        if (g.recs[ri].id == targetId) {
          // Shift left
          for (int k=ri; k<g.count-1; k++) {
            g.recs[k] = g.recs[k+1];
          }
          g.count--;
          changed = true;

          if (g.count == 0) {
            g = {}; // Clear group
          } else {
            // Update latest
            g.latestEpoch = g.recs[0].epoch;
          }
          break;
        }
      }
    }
  }

  xSemaphoreGive(s_histMutex);

  if (changed && g_fileSystemReady) persistUnlocked();
  return changed;
}

bool AlarmHistory_clearAll() {
  ensureMutex();
  if (xSemaphoreTake(s_histMutex, pdMS_TO_TICKS(200)) != pdTRUE) return false;

  for (auto& group : s_groups) {
    group = {};
  }
  s_nextId = 1;

  xSemaphoreGive(s_histMutex);

  if (g_fileSystemReady) persistUnlocked();
  return true;
}