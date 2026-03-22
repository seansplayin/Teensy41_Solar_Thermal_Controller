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

static String ipToString(const IPAddress& ip) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return String(buf);
}

static bool s_networkConnected = false;
static bool s_ethernetStarted  = false;
static bool s_serverStarted    = false;
static uint32_t s_lastStatusMs = 0;
static bool s_reportedNoLink   = false;
static bool s_reportedWaiting  = false;

static uint32_t s_lastHeartbeatMs = 0;
static uint32_t s_lastHttpMs      = 0;
static uint32_t s_httpRequestCount = 0;
static uint32_t s_http404Count     = 0;
static bool s_lastLinkOnKnown      = false;
static bool s_lastLinkOn           = false;

static void noteHttpActivity() {
  s_httpRequestCount++;
  s_lastHttpMs = millis();
}

static void printNetworkDiag(const char* why) {
  const bool linkOn = (Ethernet.linkStatus() == LinkON);
  const IPAddress ip = Ethernet.localIP();

  Serial.print("[NetDiag] why=");
  Serial.println(why ? why : "(null)");

  Serial.print("[NetDiag] ethernetStarted=");
  Serial.print(s_ethernetStarted ? "true" : "false");
  Serial.print(" serverStarted=");
  Serial.print(s_serverStarted ? "true" : "false");
  Serial.print(" networkConnected=");
  Serial.println(s_networkConnected ? "true" : "false");

  Serial.print("[NetDiag] link=");
  Serial.print(linkOn ? "ON" : "OFF");
  Serial.print(" ip=");
  Serial.println(ip);

  Serial.print("[NetDiag] gateway=");
  Serial.print(Ethernet.gatewayIP());
  Serial.print(" subnet=");
  Serial.println(Ethernet.subnetMask());

  Serial.print("[NetDiag] httpRequests=");
  Serial.print(s_httpRequestCount);
  Serial.print(" http404=");
  Serial.print(s_http404Count);
  Serial.print(" msSinceLastHttp=");
  if (s_lastHttpMs == 0) {
    Serial.println("never");
  } else {
    Serial.println(millis() - s_lastHttpMs);
  }
}

static void startHttpServerOnce() {
  if (s_serverStarted) return;

  Serial.println("[Network] startHttpServerOnce() entering");

  // Minimal root test route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    noteHttpActivity();
    Serial.println("[HTTP] GET /");
    request->send(200, "text/plain", "root-ok");
  });

  // Minimal ping test route
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request) {
    noteHttpActivity();
    Serial.println("[HTTP] GET /ping");
    request->send(200, "text/plain", "ok");
  });

  // Dedicated network/server diagnostic route
  server.on("/net-diag", HTTP_GET, [](AsyncWebServerRequest *request) {
    noteHttpActivity();
    Serial.println("[HTTP] GET /net-diag");
    printNetworkDiag("HTTP /net-diag");

    String body;
    body.reserve(256);
    body += "ethernetStarted=";
    body += (s_ethernetStarted ? "true" : "false");
    body += "\nserverStarted=";
    body += (s_serverStarted ? "true" : "false");
    body += "\nnetworkConnected=";
    body += (s_networkConnected ? "true" : "false");
    body += "\nlink=";
    body += (Ethernet.linkStatus() == LinkON ? "ON" : "OFF");
       body += "\nip=";
    body += ipToString(Ethernet.localIP());
    body += "\ngateway=";
    body += ipToString(Ethernet.gatewayIP());
    body += "\nsubnet=";
    body += ipToString(Ethernet.subnetMask());
    body += "\nhttpRequests=";
    body += String(s_httpRequestCount);
    body += "\nhttp404=";
    body += String(s_http404Count);
    body += "\n";

    request->send(200, "text/plain; charset=UTF-8", body);
  });

  serveStaticAssets(server);
  serveFavicon(server);

  //setupAlarmRoutes();
  //setupSecondPageRoutes();
  //setupThirdPageRoutes();
  //setupRoutes();
  setupFirstPageRoutes();

  server.onNotFound([](AsyncWebServerRequest *request) {
    noteHttpActivity();
    s_http404Count++;
    Serial.print("[HTTP] 404 ");
    Serial.println(request->url());
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  s_serverStarted = true;
  Serial.println("[Network] AsyncWebServer started on port 80");
  printNetworkDiag("after server.begin");
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

  if (!s_lastLinkOnKnown || linkOn != s_lastLinkOn) {
    s_lastLinkOnKnown = true;
    s_lastLinkOn = linkOn;

    Serial.print("[Network] Link transition -> ");
    Serial.println(linkOn ? "ON" : "OFF");
    printNetworkDiag("link-transition");
  }

  // Do not treat a non-zero static IP as "connected" unless the physical link is up.
  if (!linkOn) {
    s_networkConnected = false;

    if (!s_reportedNoLink) {
      Serial.println("[Network] Cable disconnected.");
      s_reportedNoLink = true;
      s_reportedWaiting = false;
    }

    if ((now - s_lastHeartbeatMs) >= 5000) {
      printNetworkDiag("heartbeat-no-link");
      s_lastHeartbeatMs = now;
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
      printNetworkDiag("connected");
    }

    startHttpServerOnce();

    if ((now - s_lastHeartbeatMs) >= 5000) {
      printNetworkDiag("heartbeat-connected");
      s_lastHeartbeatMs = now;
    }
    return;
  }

  s_networkConnected = false;

  if (!s_reportedWaiting || (now - s_lastStatusMs >= 1000)) {
    Serial.println("[Network] Link is up, waiting for static IP...");
    s_reportedWaiting = true;
    s_lastStatusMs = now;
  }

  if ((now - s_lastHeartbeatMs) >= 5000) {
    printNetworkDiag("heartbeat-waiting-for-ip");
    s_lastHeartbeatMs = now;
  }
}