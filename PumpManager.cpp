#include "PumpManager.h"
#include <Arduino.h>
#include "Config.h"
#include "Logging.h"
#include "TemperatureControl.h"
#include <Arduino.h>
#include <arduino_freertos.h>
#include "timers.h"
#include "RTCManager.h"
#include "FileSystemManager.h"
#include "AlarmManager.h"
#include "DiagLog.h"
#include "Config.h"
#include <RTClib.h>
#include "DS18B20.h"

// ✅ This line fixes HIGH, LOW, OUTPUT, INPUT_PULLUP errors
using namespace arduino; 

// Define the mutex handles
SemaphoreHandle_t pumpStateMutex = NULL;

// pumpPins defined in Config.
extern const int pumpPins[];  

bool previousDHWCallStatus = false;
bool previousHeatingCallStatus = false;

const char* pumpNames[NUM_PUMPS] = {
    "Lead Pump", "Lag Pump", "Heat Tape", "Circulation Pump", "DHW Pump",
    "Storage Heating Pump", "Boiler Circ Pump", "Recirculation Valve", "Unused", "Unused"
};

int previousPumpStates[10];
int previousPumpModes[10];

int pumpModes[10] = {PUMP_AUTO, PUMP_AUTO, PUMP_AUTO, PUMP_AUTO, PUMP_AUTO, PUMP_AUTO, PUMP_AUTO, PUMP_AUTO, PUMP_AUTO, PUMP_AUTO};
int pumpStates[10] = {PUMP_OFF, PUMP_OFF, PUMP_OFF, PUMP_OFF, PUMP_OFF, PUMP_OFF, PUMP_OFF, PUMP_OFF, PUMP_OFF, PUMP_OFF};

bool pumpOnStateHigh[10] = {false, false, false, false, false, false, false, false, false, false};
bool pumpOffStateHigh[10] = {true, true, true, true, true, true, true, true, true, true};

extern float panelT, CSupplyT, storageT, outsideT, CircReturnT, supplyT, CreturnT;
extern float DhwSupplyT, DhwReturnT, HeatingSupplyT, HeatingReturnT;

extern SemaphoreHandle_t pumpStateMutex;
extern SemaphoreHandle_t temperatureMutex;

TimerHandle_t hPumpBroadcastTimer = NULL;

void broadcastPumpState(int pumpIndex);

void onPumpBroadcastTimer(TimerHandle_t xTimer) {
    broadcastPumpState(-1);
}

void initializePumps() {
    pinMode(DHW_HEATING_PIN, INPUT_PULLUP);
    pinMode(FURNACE_HEATING_PIN, INPUT_PULLUP);
    
    for (int i = 0; i < 10; i++) {
        pinMode(pumpPins[i], OUTPUT);
        digitalWrite(pumpPins[i], pumpOffStateHigh[i] ? HIGH : LOW);
        pumpStates[i] = PUMP_OFF;
    }

    hPumpBroadcastTimer = xTimerCreate("PumpBcast", pdMS_TO_TICKS(10000), pdTRUE, (void*)0, onPumpBroadcastTimer);
    if (hPumpBroadcastTimer) xTimerStart(hPumpBroadcastTimer, 0);
}

void setPumpMode(int pumpIndex, int mode) {
    if (pumpIndex < 0 || pumpIndex >= 10) return;
    if (xSemaphoreTake(pumpStateMutex, portMAX_DELAY)) {
        pumpModes[pumpIndex] = mode;
        if (mode == PUMP_ON) setPumpState(pumpIndex, PUMP_ON);
        else if (mode == PUMP_OFF) setPumpState(pumpIndex, PUMP_OFF);
        xSemaphoreGive(pumpStateMutex);
    }
    broadcastPumpState(pumpIndex);
}

void setPumpState(int pumpIndex, int state) {
    if (pumpIndex < 0 || pumpIndex >= 10) return;
    bool isActiveHigh = pumpOnStateHigh[pumpIndex];
    int signal = (state == PUMP_ON) ? (isActiveHigh ? HIGH : LOW) : (isActiveHigh ? LOW : HIGH);
    
    digitalWrite(pumpPins[pumpIndex], signal);
    pumpStates[pumpIndex] = state;
    broadcastPumpState(pumpIndex);
}

void broadcastPumpState(int pumpIndex) {} // Stub

void PumpControl() {
  if (xSemaphoreTake(temperatureMutex, 100)) {
      xSemaphoreGive(temperatureMutex);
  }
}