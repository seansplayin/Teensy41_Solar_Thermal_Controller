#include <QNEthernet.h>



// --- NETWORK SETUP ---
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
  
  // Start Web Server
  server.begin();
}