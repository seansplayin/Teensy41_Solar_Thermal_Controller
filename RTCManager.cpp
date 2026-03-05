#include "RTCManager.h"
#include "Config.h"
#include "Logging.h"
#include "WebServerManager.h" 
#include "TimeSync.h"
#include "PumpManager.h"
#include <RTClib.h>
#include <Wire.h> // ✅ Required for Wire.begin
#include "AlarmManager.h"
#include "DiagLog.h"

RTC_DS3231 rtc;
DateTime CurrentTime; 

bool g_timeValid = false;
bool g_rtcOk = false;

void markTimeValid() { g_timeValid = true; }

void setupRTC() {
    Wire.setSDA(pinSDA);
    Wire.setSCL(pinSCL);
    Wire.begin(); 

    if (!rtc.begin()) {
        LOG_ERR("[RTC] Couldn't find RTC\n");
        g_rtcOk = false;
        AlarmManager_set(ALM_RTC_MISSING, ALM_ALARM, "RTC Not Found");
    } else {
        g_rtcOk = true;
        if (rtc.lostPower()) {
            LOG_WRN("[RTC] RTC lost power, setting default time\n");
            rtc.adjust(DateTime(__DATE__, __TIME__));
        }
    }
}

void refreshCurrentTime() {
    if (!g_rtcOk) return;
    CurrentTime = rtc.now();
}

DateTime getCurrentTime() { return CurrentTime; }
DateTime getCurrentTimeAtomic() { return CurrentTime; }

String getCurrentDateString() {
    char dateStr[13]; // Increased size
    sprintf(dateStr, "%04d-%02d-%02d", CurrentTime.year(), CurrentTime.month(), CurrentTime.day());
    return String(dateStr);
}

String getRtcTimeString() {
    char buffer[26]; // Increased size
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
             CurrentTime.year(), CurrentTime.month(), CurrentTime.day(),
             CurrentTime.hour(), CurrentTime.minute(), CurrentTime.second());
    return String(buffer);
}

int getCurrentYear() { return CurrentTime.year(); }

void printCurrentTime() {} 
void broadcastCurrentTime() {} 
void dateTimeTicker() {}