#include "HealthCheck.h"
#include "AlarmManager.h"
#include "Config.h"
#include <Arduino.h>
#include <arduino_freertos.h>
#include <string.h>
#include "DS18B20.h"
#include "TemperatureControl.h"
#include "FileSystemManager.h"
#include "MemoryStats.h"
#include "DiagLog.h"
#include <RTClib.h>
#include "RTCManager.h"   // for extern DateTime CurrentTime
#include <Arduino.h>


extern DateTime CurrentTime;   

static int  g_hc_lastY = -1;
static int  g_hc_lastM = -1;
static int  g_hc_lastD = -1;
static int  g_hc_lastH = -1;
static const uint8_t g_hc_windowSec = 2;

static inline bool hcTimeValid(const DateTime& t) {
  return (t.year() >= 2024 && t.year() <= 2099);
}

void checkHeap90() {
   // Stub
}

void checkFs90() {
  if (!g_fileSystemReady) return;
  
  // ✅ Fixed API names
  size_t total = FlashFS.totalSize();
  size_t used = FlashFS.usedSize();

  if (total > 0) {
    float pct = ((float)used / (float)total) * 100.0f;
    if (pct > 90.0f) {
      AlarmManager_set(ALM_FS_HIGH, ALM_WARN, "FS Usage High: %.1f%%", pct);
    } else {
      AlarmManager_clear(ALM_FS_HIGH, "FS Usage OK");
    }
  }
}

void runHourlyHealthCheck() {
  if (!hcTimeValid(CurrentTime)) return;
  if (CurrentTime.second() > g_hc_windowSec) return;

  const int y = CurrentTime.year();
  const int m = CurrentTime.month();
  const int d = CurrentTime.day();
  const int h = CurrentTime.hour();

  const bool alreadyRanThisHour =
    (g_hc_lastY == y && g_hc_lastM == m && g_hc_lastD == d && g_hc_lastH == h);

  if (alreadyRanThisHour) return;

  g_hc_lastY = y; g_hc_lastM = m; g_hc_lastD = d; g_hc_lastH = h;

  LOG_CAT(DBG_TASK, "[HealthCheck] runHourlyHealthCheck() millis=%lu\n", (unsigned long)millis());

  checkHeap90();
  checkFs90();

  if (storageT >= g_config.storageHeatingLimit) {
    AlarmManager_set(ALM_STORAGE_OVERTEMP, ALM_WARN, "Storage Temp High: %.1fF", storageT);
  } else {
    AlarmManager_clear(ALM_STORAGE_OVERTEMP, "Storage Temp Normal");
  }
}