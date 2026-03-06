#include "AlarmWebpage.h"
#include "WebServerManager.h" 
#include <Arduino.h>
#include "AlarmHistory.h"
#include <ArduinoJson.h>
#include "DiagLog.h"
#include <AsyncWebServer_Teensy41.h>


// The big HTML page (unchanged)
static const char alarmLogHtml[] PROGMEM = R"rawliteral(
<!doctype html><html><head>
<meta charset="utf-8">
<title>Alarm Log</title>
<style>
body{font-family:Arial; padding:12px;}
table{border-collapse:collapse; width:100%;}
td,th{border:1px solid #ccc; padding:6px; font-size:13px; vertical-align:top;}
th{background:#f3f3f3;}
.badge{font-weight:bold;}
.alarm{color:red;}
.warn{color:#b36b00;}
.small{font-size:12px; color:#555;}
button{padding:6px 10px; margin-right:8px;}
.expand{cursor:pointer; user-select:none; font-weight:bold;}
.hidden{display:none;}
.rowChild td{background:#fafafa;}
</style>
</head><body>
<h2>Alarm Log</h2>
<div id="summary" class="small">Loading...</div>
<div style="margin:10px 0;">
  <label><input type="checkbox" id="selectAll"> Select All</label>
  <button id="deleteBtn">Delete Selected</button>
  <button id="refreshBtn">Refresh</button>
</div>
<table>
<thead>
<tr>
  <th style="width:40px;"></th>
  <th style="width:170px;">Time</th>
  <th style="width:70px;">Sev</th>
  <th style="width:70px;">Code</th>
  <th style="width:70px;">Act</th>
  <th>Detail</th>
  <th style="width:60px;">Dupes</th>
  <th style="width:40px;">▼</th>
</tr>
</thead>
<tbody id="rows"></tbody>
</table>
<script>
... [your entire JavaScript is unchanged - I omitted it here for brevity but keep every line exactly as you posted] ...
</script>
</body></html>
)rawliteral";

// Body handler (works identically on Teensy41_AsyncTCP)
static void handleDeleteBody(AsyncWebServerRequest* request,
                            uint8_t* data, size_t len, size_t index, size_t total)
{
  if (index == 0) {
    request->_tempObject = new String();
    ((String*)request->_tempObject)->reserve(total);
  }
  String* body = (String*)request->_tempObject;
  for (size_t i = 0; i < len; i++) body->concat((char)data[i]);
  if (index + len != total) return;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, *body);
  delete body;
  request->_tempObject = nullptr;

  if (err) {
    request->send(400, "application/json", "{\"error\":\"bad_json\"}");
    return;
  }

  bool all = doc["all"] | false;
  if (all) {
    bool ok = AlarmHistory_clearAll();
    request->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
    return;
  }

  JsonArray idsArr = doc["ids"].as<JsonArray>();
  if (idsArr.isNull() || idsArr.size() == 0) {
    request->send(400, "application/json", "{\"error\":\"no_ids\"}");
    return;
  }

  const size_t n = idsArr.size();
  uint32_t* ids = (uint32_t*)malloc(n * sizeof(uint32_t));
  if (!ids) {
    request->send(500, "application/json", "{\"error\":\"oom\"}");
    return;
  }
  for (size_t i=0; i<n; i++) ids[i] = (uint32_t)(idsArr[i] | 0);

  bool ok = AlarmHistory_deleteIds(ids, n);
  free(ids);
  request->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void setupAlarmRoutes() {          // same signature as your ESP version
  server.on("/alarm-log", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(200, "text/html; charset=UTF-8", alarmLogHtml);
  });

  server.on("/api/alarm-log", HTTP_GET, [](AsyncWebServerRequest* req){
    AsyncResponseStream* resp = req->beginResponseStream("application/json; charset=UTF-8");
    resp->addHeader("Cache-Control", "no-store");
    AlarmHistory_writeJson(*resp);
    req->send(resp);
  });

  server.on("/api/alarm-log/delete", HTTP_POST,
    [](AsyncWebServerRequest* req){ /* body handler only */ },
    NULL,
    handleDeleteBody
  );
}