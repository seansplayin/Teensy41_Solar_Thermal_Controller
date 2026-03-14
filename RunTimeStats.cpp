#include "Config.h"
#include "DiagLog.h"
#include <Arduino.h>
#include <arduino_freertos.h>

// The Teensy FreeRTOS library does not enable runtime stats by default.
// We stub this out to prevent compilation errors.

extern "C" void vConfigureTimerForRunTimeStats() {
  // Stub
}

extern "C" uint32_t portGetRunTimeCounterValue() {
  return millis() * 1000; 
}

void printTaskStats() {
    LOG_CAT(DBG_TASK, "[Task] Task statistics disabled (Library config).\n");
}