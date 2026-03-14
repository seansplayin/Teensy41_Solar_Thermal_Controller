#include <arduino_freertos.h>
#include "TaskManager.h"
#include "FileSystemManager.h"
#include "FirstWebpage.h"
#include "Logging.h"
#include "PumpManager.h"
#include "RTCManager.h"
#include "SecondWebpage.h"
#include "ThirdWebpage.h"
#include "TemperatureControl.h"
#include "TemperatureLogging.h"
#include "TimeSync.h"
#include "Max31865-PT1000.h"
#include "AlarmManager.h"
#include <Arduino.h>
#include <Arduino.h>
#include <arduino_freertos.h>
#include <queue.h>
#include <semphr.h>
#include "DS18B20.h"
#include <RTClib.h>
#include "Config.h"
#include "AlarmHistory.h"
#include "MemoryStats.h"
//#include "TarGZ.h"
#include "DiagLog.h"
#include "WebServerManager.h"
#include <QNEthernet.h>

QueueHandle_t logQueue = nullptr;

// Mutex for temperature data access
extern SemaphoreHandle_t fileSystemMutex;
extern SemaphoreHandle_t temperatureMutex;
extern SemaphoreHandle_t pumpStateMutex;

// Flag variables
bool flagZeroLengthTime = false;
bool flagZeroLengthPumpState = false;
bool flagZeroLengthTemperatures = false;

// Temperature broadcast telemetry counters
volatile uint32_t g_tempBcastCalled = 0;
volatile uint32_t g_tempBcastSkipped = 0;


// Task Handles
TaskHandle_t thPumpControl = NULL;
TaskHandle_t thLogger = NULL;
TaskHandle_t thUpdateTemps = NULL;
TaskHandle_t thUpdatePumpRuntimes = NULL;
TaskHandle_t thTemperatureLogging = NULL;
TaskHandle_t thTgzProducer = NULL;

// --- WORKER TASKS ---

// --- WORKER TASKS ---

static void TaskNetworkPump(void *pvParameters) {
  (void)pvParameters;

  Serial.println("[TaskStart] TaskNetworkPump entered");

  for (;;) {
    qindesign::network::Ethernet.loop();   // Explicitly advance QNEthernet/lwIP
    vTaskDelay(pdMS_TO_TICKS(1));          // Keep it regular without hogging CPU
  }
}

void TaskLogger(void* pv) {
  Serial.println("[TaskStart] TaskLogger entered");
  LogEvent ev;
  for (;;) {
    if (xQueueReceive(logQueue, &ev, portMAX_DELAY)) {
       if (!g_fileSystemReady) continue;
       
       // Use standard FILE_WRITE for appending/writing
       if (xSemaphoreTake(fileSystemMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
           String fn = "/Pump_Logs/pump" + String(ev.pumpIndex + 1) + "_Log.txt";
           File f = FlashFS.open(fn.c_str(), FILE_WRITE); 
           if (f) {
             f.printf("%s %04d-%02d-%02d %02d:%02d:%02d\n",
                 ev.isStart ? "START" : "STOP",
                 ev.ts.year(),  ev.ts.month(), ev.ts.day(),
                 ev.ts.hour(),  ev.ts.minute(), ev.ts.second());
             f.close();
           }
           xSemaphoreGive(fileSystemMutex);
       }
    }
  }
}

void TaskPumpControl(void *pvParameters) {
  Serial.println("[TaskStart] TaskPumpControl entered");
  vTaskDelay(pdMS_TO_TICKS(5000)); // Boot delay
  for (;;) {
    PumpControl();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void TaskUpdateTemperatures(void *pvParameters) {
  Serial.println("[TaskStart] TaskUpdateTemperatures entered");
  Serial.println("[TaskUpdateTemperatures] initial delay starting");
  vTaskDelay(pdMS_TO_TICKS(3000));
  Serial.println("[TaskUpdateTemperatures] initial delay done");

  for (;;) {
    Serial.println("[TaskUpdateTemperatures] before updateTemperatures()");
    updateTemperatures();
    Serial.println("[TaskUpdateTemperatures] after updateTemperatures()");
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void TaskTemperatureLogging(void *pvParameters) {
  Serial.println("[TaskStart] TaskTemperatureLogging entered");
  Serial.println("[TaskStart] TaskTemperatureLogging calling setupTemperatureLogging()");
  setupTemperatureLogging();
  Serial.println("[TaskStart] TaskTemperatureLogging calling TaskTemperatureLogging_Run()");
  TaskTemperatureLogging_Run(pvParameters);
}

void startAllTasks() {
  Serial.println("[StartAllTasks] Entered");

  Serial.println("[StartAllTasks] Calling AlarmManager_begin()");
  AlarmManager_begin();
  Serial.println("[StartAllTasks] AlarmManager_begin() done");

  Serial.println("[StartAllTasks] Creating logQueue");
  logQueue = xQueueCreate(20, sizeof(LogEvent));
  Serial.println(logQueue ? "[StartAllTasks] logQueue created" : "[StartAllTasks] logQueue create FAILED");


 
  Serial.println("[StartAllTasks] Creating TaskNetworkPump");
  xTaskCreate(TaskNetworkPump, "NetPump", 3072, NULL, 2, NULL);

  Serial.println("[StartAllTasks] Creating TaskLogger");
  xTaskCreate(TaskLogger, "Logger", 4096, NULL, 1, &thLogger);

  Serial.println("[StartAllTasks] Creating TaskPumpControl");
  xTaskCreate(TaskPumpControl, "PumpControl", 4096, NULL, 4, &thPumpControl);

    Serial.println("[StartAllTasks] Creating TaskTemperatureLogging");
  xTaskCreate(TaskTemperatureLogging, "TempLog", 4096, NULL, 1, &thTemperatureLogging);

  Serial.println("[StartAllTasks] Creating TaskUpdateTemperatures");
  xTaskCreate(TaskUpdateTemperatures, "UpdateTemps", 4096, NULL, 3, &thUpdateTemps);

  Serial.println("[StartAllTasks] Creating TaskWebSocketTransmitter");
  xTaskCreate(TaskWebSocketTransmitter, "WsTx", 4096, NULL, 2, NULL);

  Serial.println("[StartAllTasks] Calling AlarmHistory_begin()");

  Serial.println("[StartAllTasks] Exit");

}