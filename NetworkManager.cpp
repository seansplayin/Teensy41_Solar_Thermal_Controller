#include "NetworkManager.h"
#include <Arduino.h>
#include <AsyncWebServer_Teensy41.h>
#include "WebServerManager.h"
#include <QNEthernet.h>
#include "AlarmWebpage.h"
#include "FirstWebpage.h"

using namespace qindesign::network;

// Explicit route-registration declarations
void setupAlarmRoutes();
void setupFirstPageRoutes();

static bool s_networkConnected = false;
static bool s_ethernetStarted  = false;
static bool s_serverStarted    = false;
static uint32_t s_lastStatusMs = 0;
static bool s_reportedNoLink   = false;
static bool s_reportedWaiting  = false;

static void startHttpServerOnce() {
  if (s_serverStarted) return;

  // Minimal root test route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET /");
    request->send(200, "text/plain", "root-ok");
  });

  // Minimal ping test route
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET /ping");
    request->send(200, "text/plain", "ok");
  });

  // Register isolated FirstWebpage test routes on non-root URLs
  setupFirstPageRoutes();

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("[HTTP] 404 ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  s_serverStarted = true;
  Serial.println("[Network] AsyncWebServer started on port 80");
}

bool isNetworkConnected() {
  return s_networkConnected && (Ethernet.localIP() != IPAddress(0, 0, 0, 0));
}

void setupNetwork() {
  Serial.println("[Network] Initializing QNEthernet...");

  Ethernet.setHostname("teensy41-solar");

    if (!s_ethernetStarted) {
    const IPAddress kStaticIP(10, 20, 90, 39);
    const IPAddress kNetmask(255, 255, 255, 0);
    const IPAddress kGateway(10, 20, 90, 1);
    const IPAddress kDNS(10, 20, 90, 1);

    bool started = Ethernet.begin(kStaticIP, kNetmask, kGateway, kDNS);
    if (!started) {
      Serial.println("[Network] Ethernet.begin(static) failed!");
      s_networkConnected = false;
      return;
    }

    s_ethernetStarted = true;

    Serial.print("[Network] Static IP requested: ");
    Serial.println(kStaticIP);
  }

  Serial.println("[Network] Waiting for link / static IP...");
  s_lastStatusMs = millis();
}

void serviceNetwork() {
  if (!s_ethernetStarted) return;

  Ethernet.loop();

  const bool linkOn = (Ethernet.linkStatus() == LinkON);
  const IPAddress ip = Ethernet.localIP();
  const uint32_t now = millis();

  // Do not treat a non-zero static IP as "connected" unless the physical link is up.
  if (!linkOn) {
    s_networkConnected = false;

    if (!s_reportedNoLink) {
      Serial.println("[Network] Cable disconnected.");
      s_reportedNoLink = true;
      s_reportedWaiting = false;
    }
    return;
  }

  s_reportedNoLink = false;

  if (ip != IPAddress(0, 0, 0, 0)) {
    if (!s_networkConnected) {
      s_networkConnected = true;

      Serial.print("[Network] Connected! IP: ");
      Serial.println(ip);

      Serial.print("[Network] Gateway: ");
      Serial.println(Ethernet.gatewayIP());

      Serial.print("[Network] Subnet: ");
      Serial.println(Ethernet.subnetMask());

      s_reportedWaiting = false;
    }

    startHttpServerOnce();
    return;
  }

  s_networkConnected = false;

  if (!s_reportedWaiting || (now - s_lastStatusMs >= 1000)) {
    Serial.println("[Network] Link is up, waiting for static IP...");
    s_reportedWaiting = true;
    s_lastStatusMs = now;
  }
}