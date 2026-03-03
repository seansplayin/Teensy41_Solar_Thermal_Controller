#include "TemperatureControl.h"
#include "Logging.h"
#include "WebServerManager.h"
#include "PumpManager.h"
#include "Config.h"
#include "Max31865-PT1000.h"
#include "DS18B20.h"
#include "DiagLog.h"
#include <arduino_freertos.h>

// Mutex for temperature data access
extern SemaphoreHandle_t temperatureMutex;

// Remove Ticker
// Ticker temperatureTicker; 

// Define the temperature variables
float panelT = 0.0;
float CSupplyT = 0.0;
float storageT = 0.0;
float outsideT = 0.0;
float CircReturnT = 0.0;
float supplyT = 0.0;
float CreturnT = 0.0;
float DhwSupplyT = 0.0;
float DhwReturnT = 0.0;
float HeatingSupplyT = 0.0;
float HeatingReturnT = 0.0;
float dhwT = 0.0;
float PotHeatXinletT = 0.0;
float PotHeatXoutletT = 0.0;

// Previous values
float prev_pt1000Current = NAN;
float prev_pt1000Average = NAN;
float prev_DTemp[NUM_SENSORS];
float prev_DTempAverage[NUM_SENSORS];

// Flag from WebServerManager
extern String g_tempWsPayload;
extern SemaphoreHandle_t g_tempWsPayloadMutex;
extern volatile bool g_sendTemperatures;

void appendTemperature(String& json, float value, float& prevValue) {
    if (isnan(value)) return;
    if (json.length() > 0) json += ",";
    json += String(value, 1);
    
    // Simple change detection
    if (isnan(prevValue) || abs(value - prevValue) >= 0.1) {
        prevValue = value;
    }
}

void broadcastTemperatures() {
    if (xSemaphoreTake(temperatureMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        String message = "";
        // Simple serialization example
        message += "Panel:" + String(pt1000Current, 1);
        
        // Pass to WebServer Manager
        if (g_tempWsPayloadMutex) {
            if (xSemaphoreTake(g_tempWsPayloadMutex, pdMS_TO_TICKS(10))) {
                g_tempWsPayload = message;
                g_sendTemperatures = true;
                xSemaphoreGive(g_tempWsPayloadMutex);
            }
        }
        xSemaphoreGive(temperatureMutex);
    }
}

void updateTemperatures() {
  if (! xSemaphoreTake(temperatureMutex, portMAX_DELAY)) {
    LOG_ERR("[Temp] Failed to take temperatureMutex\n");
    return;
  }

  // 1. Request OneWire
  sensors1.requestTemperatures();
  sensors2.requestTemperatures();

  // 2. Read PT1000
  updatePT1000Readings();

  // 3. Read OneWire
  updateDS18B20Readings(); // Make sure this function exists in DS18B20.cpp

  // 4. Map to globals (panelT, etc)
  AssignTemperatureProbes();

  xSemaphoreGive(temperatureMutex);
  
  broadcastTemperatures();
}

void AssignTemperatureProbes() {
    // Mapping logic (Example)
    panelT = pt1000Average;
    CSupplyT = DTempAverage[0]; 
    // ... Fill in the rest based on your SensorMapping ...
}

void updateTemperature(int pumpIndex, float newTemperature) {
    // Stub
}