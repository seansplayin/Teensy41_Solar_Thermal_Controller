#include "uptime_formatter.h"

String UptimeFormatter::getUptime() {
  unsigned long up = millis();
  unsigned long seconds = up / 1000;
  unsigned long minutes = (seconds / 60) % 60;
  unsigned long hours   = (seconds / 3600) % 24;
  unsigned long days    = (seconds / 86400);
  seconds %= 60;

  String s;
  if (days > 0) { s += days; s += "d "; }
  if (hours < 10) s += "0";
  s += hours; s += ":";
  if (minutes < 10) s += "0";
  s += minutes; s += ":";
  if (seconds < 10) s += "0";
  s += seconds;
  return s;
}