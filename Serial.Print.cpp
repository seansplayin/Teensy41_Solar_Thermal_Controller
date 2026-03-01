#include "SerialPrint.h"
#include <stdarg.h>
#include <stdio.h>

void SERIAL_PRINT(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Teensy Serial is generally interrupt-safe enough for this usage
    Serial.print(buffer);
}