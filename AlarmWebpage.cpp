#include "AlarmWebpage.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "AlarmHistory.h"
#include "WebServerManager.h"
#include "FileSystemManager.h" // Needed for FlashFS
#include "DiagLog.h"
#include <AsyncWebServer_Teensy41.h>

extern AsyncWebServer server;

void setupAlarmRoutes() {
  // Serve the HTML Page manually
  server.on("/alarm-log", HTTP_GET, [](AsyncWebServerRequest *request){
      if (FlashFS.exists("/alarm_log.html")) {
          File f = FlashFS.open("/alarm_log.html", FILE_READ);
          if (f) {
              String s = f.readString();
              f.close();
              request->send(200, "text/html", s);
              return;
          }
      }
      request->send(404, "text/plain", "Alarm Log HTML Missing");
  });

  // API: Get JSON data
  server.on("/api/alarm-history", HTTP_GET, [](AsyncWebServerRequest *request){
      // We can create a chunked response here, but for simplicity/stability 
      // on Teensy, let's stream directly if possible, or buffer small amounts.
      // Since AlarmHistory is complex, we will send a basic empty array stub 
      // for now to ensure compilation, or hook up the real logic if stable.
      request->send(200, "application/json", "[]"); 
  });
}