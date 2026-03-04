#include <Arduino.h>
#include <LittleFS.h>
LittleFS_Program LittleFS;   // uses internal flash (perfect for Teensy 4.1)

#include <arduino_freertos.h>


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
#include "uptime_formatter.h"
#include "AlarmWebpage.h"
#include "NetworkManager.h"
#include "WebServerManager.h"

using namespace qindesign::network;


// Define them here (or in a new Globals.cpp later)
SemaphoreHandle_t pumpStateMutex = nullptr;
SemaphoreHandle_t temperatureMutex = nullptr;
SemaphoreHandle_t fileSystemMutex = nullptr;

String g_tempWsPayload = "";
volatile bool g_sendTemperatures = false;
// std::mutex g_tempWsPayloadMutex;  // Uncomment if using std::mutex



// --- MAIN CONTROLLER TASK ---
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
  setupNetwork(); // now lives in NetworkManager.cpp
  
  // 4. Start Tasks
  Serial.println("[System] Starting Application Tasks...");
  startAllTasks(); 

  WebServerManager_begin();
}

// --- BOOTLOADER ---
void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin()) {              // 0 = use all available program flash
    Serial.println("LittleFS mount FAILED!");
    while (1);                          // halt if no filesystem
  }
  Serial.println("LittleFS mounted OK");

  while (!Serial && millis() < 4000); 

  Serial.println("--- TEENSY 4.1 SOLAR CONTROLLER STARTING ---");

  // Create the main task
  xTaskCreate(TaskControllerMain, "MainCtrl", 8192, NULL, 1, NULL);

  // Start Scheduler
  vTaskStartScheduler();

  WebServerManager_begin();
}

void loop() {
  // Unused in FreeRTOS
}