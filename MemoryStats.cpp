#include "MemoryStats.h"
#include <Arduino.h>
#include <arduino_freertos.h>
#include <task.h>
#include "Config.h"
#include "DiagLog.h"
#include <malloc.h>

extern "C" struct mallinfo mallinfo(void);

extern TaskHandle_t thPumpControl;
extern TaskHandle_t thLogger;
extern TaskHandle_t thUpdateTemps;
extern TaskHandle_t thTemperatureLogging;
extern TaskHandle_t thTgzProducer;

static void printTaskHwm(const char* label, TaskHandle_t handle) {
    Serial.print("[Mem] StackHWM ");
    Serial.print(label);
    Serial.print(": ");

    if (handle == NULL) {
        Serial.println("not-created");
        return;
    }

    UBaseType_t hwmWords = uxTaskGetStackHighWaterMark(handle);
    Serial.print((uint32_t)hwmWords);
    Serial.print(" words (");
    Serial.print((uint32_t)hwmWords * sizeof(StackType_t));
    Serial.println(" bytes free-min)");
}

String getHeapInternalString() {
    struct mallinfo mi = mallinfo();
    return "arena=" + String(mi.arena) +
           " uordblks=" + String(mi.uordblks) +
           " fordblks=" + String(mi.fordblks) +
           " keepcost=" + String(mi.keepcost);
}

String getPsramString() {
    return "Teensy4.1 RAM2 malloc/new pool reported at link time";
}

void MemoryStats_printSnapshot(const char* tag) {
    struct mallinfo mi = mallinfo();

    Serial.println();
    Serial.print("[Mem] Snapshot: ");
    Serial.println(tag ? tag : "unknown");

    Serial.print("[Mem] mallinfo arena=");
    Serial.print(mi.arena);
    Serial.print(" uordblks=");
    Serial.print(mi.uordblks);
    Serial.print(" fordblks=");
    Serial.print(mi.fordblks);
    Serial.print(" keepcost=");
    Serial.println(mi.keepcost);

    TaskHandle_t current = xTaskGetCurrentTaskHandle();
    Serial.print("[Mem] CurrentTask: ");
    if (current != NULL) {
        Serial.print(pcTaskGetName(current));
        Serial.print(" HWM=");
        UBaseType_t curHwmWords = uxTaskGetStackHighWaterMark(current);
        Serial.print((uint32_t)curHwmWords);
        Serial.print(" words (");
        Serial.print((uint32_t)curHwmWords * sizeof(StackType_t));
        Serial.println(" bytes free-min)");
    } else {
        Serial.println("none");
    }

    printTaskHwm("TaskLogger", thLogger);
    printTaskHwm("TaskPumpControl", thPumpControl);
    printTaskHwm("TaskUpdateTemps", thUpdateTemps);
    printTaskHwm("TaskTemperatureLogging", thTemperatureLogging);
    printTaskHwm("TaskTgzProducer", thTgzProducer);

    Serial.println("[Mem] EndSnapshot");
    Serial.println();
}