#ifndef UPTIME_FORMATTER_H
#define UPTIME_FORMATTER_H

#include <Arduino.h>

class UptimeFormatter {
public:
  static String getUptime();
};

#endif