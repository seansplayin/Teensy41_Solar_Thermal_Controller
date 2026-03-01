#include "Logging.h"
#include "TimeSync.h"
#include "RTCManager.h"
#include "PumpManager.h"
#include "Config.h"
#include <Arduino.h>
#include "FileSystemManager.h"
#include "TaskManager.h"
#include "DiagLog.h"
#include "arduino_freertos.h"

extern DateTime CurrentTime;
extern RTC_DS3231 rtc;
volatile bool Elapsed_Day = false;
extern void runHourlyHealthCheck();

extern SemaphoreHandle_t fileSystemMutex;

// --- MEMORY MONITORING (Teensy Version) ---
static void memMark(const char* tag) {
  if (!tag) tag = "?";
  // Teensy doesn't support heap_caps.
  // Can use internal RAM checks later.
  LOG_CAT(DBG_MEM, "[%s] Memory Check (Teensy)\n", tag);
}

// --- LOGGING QUEUE ---
extern QueueHandle_t logQueue;

void logPumpEvent(uint8_t pumpIndex, bool isStart, const DateTime &ts) {
  if (!logQueue) return;
  LogEvent ev{ pumpIndex, isStart, ts };
  xQueueSend(logQueue, &ev, 0);
}

// Helper function to parse datetime string
DateTime parseDateTimeFromLog(const String& datetimeStr) {
  int year = datetimeStr.substring(0, 4).toInt();
  int month = datetimeStr.substring(5, 7).toInt();
  int day = datetimeStr.substring(8, 10).toInt();
  int hour = datetimeStr.substring(11, 13).toInt();
  int minute = datetimeStr.substring(14, 16).toInt();
  int second = datetimeStr.substring(17, 19).toInt();
  return DateTime(year, month, day, hour, minute, second);
}

// --- AGGREGATION STUBS (To ensure compilation) ---
// You can re-enable the logic once the basic system is stable.
void aggregatePumptoDailyLogs(int pumpIndex) {}
void aggregateDailyToMonthlyLogs(int pumpIndex) {}
void aggregateMonthlyToYearlyLogs(int pumpIndex) {}
void performLogAggregation() {}

void setElapsed_Day() {
  Elapsed_Day = true;
  LOG_CAT(DBG_PUMPLOG, "Elapsed_Day flag set to true\n");
}

void checkTimeAndAct() {
  // Placeholder for scheduling logic
}

void listAllFiles() {
    // Stub
}

void readAndPrintLogFile(const String& filename) {
    // Stub
}