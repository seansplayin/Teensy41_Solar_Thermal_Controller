#ifndef SERIALPRINT_H
#define SERIALPRINT_H

#include <Arduino.h>
#include "Config.h"

// Basic wrapper for thread-safe printing if needed later
void SERIAL_PRINT(const char* format, ...);

#endif