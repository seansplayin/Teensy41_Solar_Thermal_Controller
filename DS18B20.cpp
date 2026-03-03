#include "DS18B20.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Config.h"
#include "DiagLog.h"
#include "AlarmManager.h"
#include <arduino_freertos.h>

// Remove watchdog header
// #include <esp_task_wdt.h>

#define NUM_SENSORS 13

OneWire oneWire1(ONE_WIRE_BUS_1);
OneWire oneWire2(ONE_WIRE_BUS_2);

DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);

float DTemp[NUM_SENSORS] = {0};
float DTempAverage[NUM_SENSORS] = {0.0};
float sensorOffsets[NUM_SENSORS] = {0};

const int numReadings = 3;
float DTempValues[NUM_SENSORS][numReadings] = {0};
int readingsIndex[NUM_SENSORS] = {0};
bool onBus1[NUM_SENSORS];

SensorMapping sensorMappings[NUM_SENSORS] = {
    // ... (Keep your mappings if you have them, or use a placeholder)
};

void initDS18B20Sensors() {
    sensors1.begin();
    sensors2.begin();
    sensors1.setWaitForConversion(false);
    sensors2.setWaitForConversion(false);
    // Init logic...
}

float calculateAverage(float values[], int numReadings) {
    float sum = 0;
    int count = 0;
    for (int i = 0; i < numReadings; i++) {
        if (values[i] != -127.0 && values[i] != 85.0 && values[i] != 0.0) {
            sum += values[i];
            count++;
        }
    }
    return (count > 0) ? (sum / count) : -127.0;
}

void updateDS18B20Temperature(int sensorIndex, float temperature) {
    // Logic...
}

void updateDS18B20Readings() {
   // Watchdog reset REMOVED
   // esp_task_wdt_reset(); 
   
   // Reading logic...
}