#include "NetworkManager.h"
#include <Arduino.h>
#include <AsyncWebServer_Teensy41.h>
#include "WebServerManager.h"
#include <QNEthernet.h>
#include "AlarmWebpage.h"
#include "FirstWebpage.h"
#include "SecondWebpage.h"
#include "ThirdWebpage.h"

using namespace qindesign::network;

// Explicit route-registration declarations
void setupAlarmRoutes();
void setupFirstPageRoutes();
void setupSecondPageRoutes();
void setupThirdPageRoutes();

static bool s_networkConnected = false;

bool isNetworkConnected() {
  return s_networkConnected && (Ethernet.localIP() != IPAddress(0, 0, 0, 0));
}

void setupNetwork() {
  Serial.println("[Network] Initializing QNEthernet...");

  Ethernet.setHostname("teensy41-solar");

  // Give the PHY / switch a brief moment before starting DHCP
  delay(250);

  bool started = Ethernet.begin();   // DHCP-enabled startup
  if (!started) {
    Serial.println("[Network] Ethernet.begin() failed!");
    s_networkConnected = false;
    return;
  }

  Serial.println("[Network] Waiting for DHCP / link...");

  unsigned long start = millis();
  bool sawLinkUp = false;

  while (millis() - start < 10000) {
    if (Ethernet.linkStatus() == LinkON) {
      sawLinkUp = true;
    }

    if (Ethernet.localIP() != IPAddress(0, 0, 0, 0)) {
      break;
    }

    delay(100);
  }

  if (Ethernet.localIP() != IPAddress(0, 0, 0, 0)) {
    s_networkConnected = true;

    Serial.print("[Network] Connected! IP: ");
    Serial.println(Ethernet.localIP());

    Serial.print("[Network] Gateway: ");
    Serial.println(Ethernet.gatewayIP());

    Serial.print("[Network] Subnet: ");
    Serial.println(Ethernet.subnetMask());
    } else {
    s_networkConnected = false;

    if (sawLinkUp || Ethernet.linkStatus() == LinkON) {
      Serial.println("[Network] DHCP failed, but link appears up.");
    } else {
      Serial.println("[Network] Cable disconnected.");
    }
    return;
  }

  initWebSocket();

  // Minimal debug route
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET /ping");
    request->send(200, "text/plain", "ok");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("[HTTP] 404 ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not found");
  });

  setupAlarmRoutes();
  setupFirstPageRoutes();
  setupSecondPageRoutes();
  setupThirdPageRoutes();

  server.begin();
  Serial.println("[Network] AsyncWebServer started on port 80");
  unsigned long waitStart = millis();
  while (millis() - waitStart < 10000) {
  Ethernet.loop();   // keep network stack alive
  delay(1);
}

Serial.println("[Network] Continuing boot...");


}