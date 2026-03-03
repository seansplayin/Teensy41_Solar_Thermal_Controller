#include "NetworkManager.h"
#include "WebServerManager.h"
#include <QNEthernet.h>
#include "AlarmWebpage.h"



using namespace qindesign::network;   // ← THIS FIXES Ethernet & LinkON

bool isNetworkConnected() {
  return Ethernet.linkStatus() == LinkON;
}

void setupNetwork() {
  Serial.println("[Network] Initializing QNEthernet...");
  Ethernet.begin(); // DHCP

  unsigned long start = millis();
  while (!Ethernet.localIP()) {
    if (millis() - start > 10000) {
      Serial.println("[Network] DHCP Failed!");
      break;
    }
    delay(100);
  }
  
  if (Ethernet.linkStatus() == LinkON) {
      Serial.print("[Network] Connected! IP: ");
      Serial.println(Ethernet.localIP());
  } else {
      Serial.println("[Network] Cable disconnected.");
  }

  // Register ALL web routes (exactly like your ESP version)
  setupAlarmRoutes();     // from AlarmWebpage.cpp
  // setupFirstPageRoutes();   // uncomment when you add FirstWebpage.cpp
  // ... add more later

  server.begin();
  Serial.println("[Network] AsyncWebServer started on port 80");
}