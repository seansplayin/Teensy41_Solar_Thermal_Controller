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
#include "WebServerManager.h"
#include "Max31865-PT1000.h"
#include "AlarmManager.h"
#include <LittleFS.h>
#include <Arduino.h>

// ✅ Teensy FreeRTOS Headers (Flat structure)
#include <arduino_freertos.h>
#include <queue.h>
#include <semphr.h>

#include "DS18B20.h"

#include "Config.h"
#include "AlarmHistory.h"
#include "MemoryStats.h"
//#include "TarGZ.h"
#include "DiagLog.h"

// Remove ESP specific headers
// #include <esp_task_wdt.h>
// #include "esp_timer.h"

QueueHandle_t logQueue = nullptr;

// Define the mutex handles
SemaphoreHandle_t pumpStateMutex = NULL;
SemaphoreHandle_t temperatureMutex = NULL;
// fileSystemMutex is defined in FileSystemManager.cpp

// Flag variables
bool flagZeroLengthTime = false;
bool flagZeroLengthPumpState = false;
bool flagZeroLengthTemperatures = false;


// Task Handles
TaskHandle_t thPumpControl = NULL;
TaskHandle_t thLogger = NULL;
TaskHandle_t thUpdateTemps = NULL;
TaskHandle_t thUpdatePumpRuntimes = NULL;
TaskHandle_t thTemperatureLogging = NULL;
TaskHandle_t thTgzProducer = NULL;

// --- WORKER TASKS ---

void TaskLogger(void* pv) {
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
  vTaskDelay(pdMS_TO_TICKS(5000)); // Boot delay
  for (;;) {
    PumpControl();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void TaskUpdateTemperatures(void *pvParameters) {
  for (;;) {
    updateTemperatures();
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void TaskTemperatureLogging(void *pvParameters) {
  setupTemperatureLogging();
  TaskTemperatureLogging_Run(pvParameters);
}

void startAllTasks() {
  AlarmManager_begin();
  logQueue = xQueueCreate(20, sizeof(LogEvent));

  // Note: No "PinnedToCore" on Teensy
  xTaskCreate(TaskLogger, "Logger", 4096, NULL, 1, &thLogger);
  xTaskCreate(TaskUpdateTemperatures, "UpdateTemps", 4096, NULL, 3, &thUpdateTemps);
  xTaskCreate(TaskPumpControl, "PumpControl", 4096, NULL, 4, &thPumpControl);
  xTaskCreate(TaskTemperatureLogging, "TempLog", 4096, NULL, 1, &thTemperatureLogging);

  AlarmHistory_begin();
}