#include "MemoryStats.h"
#include "Config.h"
#include "DiagLog.h"
#include <malloc.h> // Standard C malloc info
#include <Arduino.h>

extern "C" struct mallinfo mallinfo(void);

String getHeapInternalString() {
    struct mallinfo mi = mallinfo();
    // Approximate free RAM calculation for Teensy
    // (This varies by model, treating simply for compile)
    uint32_t freeRam = 0; 
    return "Free: " + String(mi.fordblks) + " bytes";
}

String getPsramString() {
    return "EXT RAM: 8MB (Teensy 4.1)";
}

void MemoryStats_printSnapshot(const char* tag) {
    LOG_CAT(DBG_MEM, "[Mem] Snapshot %s\n", tag ? tag : "unknown");
}