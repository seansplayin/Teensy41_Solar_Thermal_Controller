#include "TimeSync.h"
#include "Logging.h"
#include "RTCManager.h"
#include "Config.h"
#include "PumpManager.h"
#include <Wire.h>
#include <TimeLib.h>
#include <RTClib.h>
#include <arduino_freertos.h> 
#include "DiagLog.h"
#include "AlarmManager.h"
#include "timers.h"


extern DateTime CurrentTime;
bool needToSyncTime = true;
bool needNtpSync = true;
unsigned long lastNtpUpdateAttempt = 0;
const unsigned long ntpRetryInterval = 600000;
bool isNtpSyncDue = true;

static volatile bool g_ntpResyncRequested = false;

// --- FreeRTOS Timer for Retry ---
TimerHandle_t hNtpRetryTimer = NULL;

void onNtpRetry(TimerHandle_t xTimer) {
    tryNtpUpdate();
}

void requestImmediateNtpResync() {
    g_ntpResyncRequested = true;
}

// Stub for NTP Update
void tryNtpUpdate() {
    LOG_CAT(DBG_TIMESYNC, "[TimeSync] Trying NTP Update...\n");
    // If fail:
    // xTimerStart(hNtpRetryTimer, 0);
}

// Teensy 4.1 real-time clock provider for TimeLib
time_t getTeensy3Time() {
  return Teensy3Clock.get();
}

void initNTP() {
    LOG_CAT(DBG_TIMESYNC, "[TimeSync] NTP Init\n");
    // Create One-Shot Timer for retries (10 minutes)
    hNtpRetryTimer = xTimerCreate("NtpRetry", pdMS_TO_TICKS(600000), pdFALSE, (void*)0, onNtpRetry);
}

void checkAndSyncTime() {
    if (g_ntpResyncRequested) {
        g_ntpResyncRequested = false;
        tryNtpUpdate();
    }
}

void printCurrentRtcTime() {
    LOG_CAT(DBG_RTC, " Current time: %04d/%02d/%02d %02d:%02d:%02d\n",
        CurrentTime.year(), CurrentTime.month(), CurrentTime.day(),
        CurrentTime.hour(), CurrentTime.minute(), CurrentTime.second());
}

void initializeTime() {
    // Teensy RTC
    setSyncProvider(getTeensy3Time);
    initNTP();
}

