#include <Arduino.h>
#include <QNEthernet.h>
#include <AsyncWebServer_Teensy41.h>
#include "arduino_freertos.h"

// --- Application Includes ---
#include "Config.h"
#include "Logging.h"
#include "PumpManager.h"
#include "RTCManager.h"
#include "TemperatureControl.h"
#include "TimeSync.h"
#include "FileSystemManager.h"
#include "FirstWebpage.h"
#include "SecondWebpage.h"
#include "ThirdWebpage.h"
#include "TaskManager.h"
#include "Max31865-PT1000.h"
#include "DS18B20.h"
#include "TemperatureLogging.h"
#include "DiagLog.h"
#include "AlarmHistory.h" 

using namespace qindesign::network;

// --- GLOBAL RTOS HANDLES ---\
SemaphoreHandle_t pumpStateMutex;
SemaphoreHandle_t temperatureMutex;
// fileSystemMutex declared in FileSystemManager

// --- WEB SERVER ---\
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); 

// --- NETWORK SETUP ---\
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

// --- MAIN CONTROLLER TASK ---\
static void TaskControllerMain(void* pvParameters) {
  
  // 1. Initialize Mutexes
  pumpStateMutex = xSemaphoreCreateMutex();
  temperatureMutex = xSemaphoreCreateMutex();
  // fileSystemMutex init in initializeFileSystem()

  Serial.println("[System] Mutexes Created.");

  // 2. Initialize Hardware
  initializeFileSystem();
  setupRTC();
  initPT1000Sensor();     
  initDS18B20Sensors();   
  initializePumps();
  
  // 3. Start Network & Web
  setupNetwork();
  
  // 4. Start Tasks
  Serial.println("[System] Starting Application Tasks...");
  startAllTasks(); 

  // 5. Infinite Loop
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(10000)); 
    Serial.print("[System] Heap: ");
    Serial.println(xPortGetFreeHeapSize());
  }
}

// --- BOOTLOADER ---\
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000); 

  Serial.println("--- TEENSY 4.1 SOLAR CONTROLLER STARTING ---");

  // Create the main task
  xTaskCreate(TaskControllerMain, "MainCtrl", 8192, NULL, 1, NULL);

  // Start Scheduler
  vTaskStartScheduler();
}

void loop() {
  // Unused in FreeRTOS
}