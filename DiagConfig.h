#ifndef DIAGCONFIG_H
#define DIAGCONFIG_H

#include <stdint.h>

// Debug Categories
enum : uint32_t {
  DBG_NONE      = 0u,
  DBG_MEM       = 1u << 0,
  DBG_FS        = 1u << 1,
  DBG_PUMPLOG   = 1u << 2,
  DBG_TEMPLOG   = 1u << 3,
  DBG_ALARMLOG  = 1u << 4,
  DBG_1WIRE     = 1u << 5,
  DBG_RTD       = 1u << 6,
  DBG_NET       = 1u << 7,
  DBG_PUMP      = 1u << 8,
  DBG_RTC       = 1u << 9,
  DBG_TARGZ     = 1u << 10,
  DBG_TIMESYNC  = 1u << 11,
  DBG_WEB       = 1u << 12,
  DBG_TASK      = 1u << 13,
  DBG_SENSOR    = 1u << 14,
  DBG_CONFIG    = 1u << 15,
  DBG_PERF      = 1u << 16,
  
  DBG_DEFAULT_DEV_MASK = 0xFFFFFFFFu
};

#endif