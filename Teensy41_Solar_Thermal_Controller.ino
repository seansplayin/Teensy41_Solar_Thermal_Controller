#define CORE_TEENSY
#include "FileSystemManager.h"
#include <Arduino.h>
#include <arduino_freertos.h>
#include <RTClib.h>
#include <DallasTemperature.h>

// --- Application Includes ---
#include "Config.h"
#include "Logging.h"
#include "PumpManager.h"
#include "RTCManager.h"
#include "TemperatureControl.h"
#include "TimeSync.h"
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
#include "QNEthernet.h" 

using namespace qindesign::network;


extern String g_tempWsPayload;
extern volatile bool g_sendTemperatures;

// --- MAIN CONTROLLER TASK ---
static void TaskControllerMain(void* pvParameters) {
  
  Serial.println("[Boot] TaskControllerMain started");

  // 1. Initialize Mutexes
  pumpStateMutex = xSemaphoreCreateMutex();
  temperatureMutex = xSemaphoreCreateMutex();
  // fileSystemMutex init in initializeFileSystem()

  Serial.println("[System] Mutexes Created.");

  // 2. Initialize Hardware
    Serial.println("[Boot] Initializing filesystem...");
  initializeFileSystem();
  Serial.printf("[Boot] Filesystem ready = %s\n", g_fileSystemReady ? "true" : "false");

  if (!g_fileSystemReady) {
    Serial.println("[Boot] Filesystem init failed. Stopping main startup.");
    vTaskDelete(NULL);
  }

  Serial.println("[Boot] Initializing RTC...");
  setupRTC();

  Serial.println("[Boot] Initializing PT1000...");
  initPT1000Sensor();

  Serial.println("[Boot] Initializing DS18B20...");
  initDS18B20Sensors();

  Serial.println("[Boot] Initializing pumps...");
  initializePumps();
  
  // 3. Start Network & Web
  setupNetwork(); // network + HTTP routes + server begin

  // 4. Start Tasks
  Serial.println("[System] Skipping Application Tasks for HTTP isolation test");
  for (;;) {
    qindesign::network::Ethernet.loop();
    vTaskDelay(pdMS_TO_TICKS(1));
  }

}

// --- BOOTLOADER ---
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000);

  Serial.println("--- TEENSY 4.1 SOLAR CONTROLLER STARTING ---");
  Serial.println("[Boot] setup() reached");
  Serial.println("[Boot] Filesystem init will run inside TaskControllerMain()");

  // Create the main task
  xTaskCreate(TaskControllerMain, "MainCtrl", 8192, NULL, 1, NULL);

  // Start Scheduler
  vTaskStartScheduler();
}

void loop() {
  // Unused in FreeRTOS
}