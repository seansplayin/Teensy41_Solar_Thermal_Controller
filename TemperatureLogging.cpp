#include "TemperatureLogging.h"
#include "Config.h"
#include "FileSystemManager.h"
#include "RTCManager.h"
#include <LittleFS.h>
#include <time.h>
#include <Arduino.h>
#include <math.h>
#include <TimeLib.h>
#include "DiagLog.h"
#include "AlarmManager.h"
#include "arduino_freertos.h"

// ✅ Remove watchdog
// #include <esp_task_wdt.h>

extern SemaphoreHandle_t fileSystemMutex;

// External temperature globals
extern float panelT;
extern float CSupplyT;
extern float storageT;
extern float outsideT;
extern float CircReturnT;
// ... (rest of externs)

extern const char* SENSOR_FILE_NAMES[15]; 
extern const char* SENSOR_NAMES[15];

// Config Defines
#ifndef TEMP_LOG_SAMPLE_SEC
#define TEMP_LOG_SAMPLE_SEC   60
#endif
#ifndef TEMP_LOG_DELTA_F
#define TEMP_LOG_DELTA_F      1.0f
#endif
#ifndef TEMP_LOG_FLUSH_MIN
#define TEMP_LOG_FLUSH_MIN    60
#endif

void setupTemperatureLogging() {
    // Stub setup
    if (!FlashFS.exists("/Temperature_Logs")) {
        FlashFS.mkdir("/Temperature_Logs");
    }
}

void TaskTemperatureLogging_Run(void *pvParams) {
    for (;;) {
        // Logic Stub
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void enableTemperatureLogging() {
    // Stub
}