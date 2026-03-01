#ifndef DIAGLOG_H
#define DIAGLOG_H

#include <Arduino.h>

// Debug Categories (Bitmask)
#define DBG_NONE        0x0000
#define DBG_NET         0x0001
#define DBG_RTC         0x0002
#define DBG_TIMESYNC    0x0004
#define DBG_PUMP        0x0008
#define DBG_TEMP        0x0010
#define DBG_WEB         0x0020
#define DBG_FS          0x0040
#define DBG_CONFIG      0x0080
#define DBG_TASK        0x0100
#define DBG_MEM         0x0200
#define DBG_ALARMLOG    0x0400
#define DBG_TEMPLOG     0x0800
#define DBG_PUMPLOG     0x1000
#define DBG_1WIRE       0x2000
#define DBG_RTD         0x4000
#define DBG_PERF        0x8000

#define DBG_ALL         0xFFFF

// Teensy-Safe Logging Macros
// These map your existing LOG_CAT calls to Serial.printf
// The 'cat' argument is currently ignored for simplicity, but could be used to filter.
#define LOG_CAT(cat, fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)      Serial.printf("[ERR] " fmt, ##__VA_ARGS__)
#define LOG_INF(fmt, ...)      Serial.printf("[INF] " fmt, ##__VA_ARGS__)
#define LOG_WRN(fmt, ...)      Serial.printf("[WRN] " fmt, ##__VA_ARGS__)

#endif