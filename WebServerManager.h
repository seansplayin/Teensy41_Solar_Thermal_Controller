#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H
#include <RTClib.h>
#include <AsyncWebServer_Teensy41.h>
#include <Teensy41_AsyncTCP.h>
#include "Config.h"
#include "TemperatureControl.h"
#include "arduino_freertos.h"

extern AsyncWebServer server;
extern AsyncWebSocket ws;
void setupRoutes();
void initWebSocket();
void startServer();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void broadcastMessageOverWebSocket(const String& message, const String& messageType);

// --- Gatekeeper Flags ---
extern volatile bool g_sendPumpStatus;
extern volatile bool g_sendAlarmState;
extern volatile bool g_sendConfig;
extern volatile bool g_sendTimeConfig;
extern volatile bool g_sendTemperatures;
extern volatile bool g_sendDateTime;

extern String g_tempWsPayload;
extern SemaphoreHandle_t g_tempWsPayloadMutex;

void TaskWebSocketTransmitter(void* pvParameters);

void setupLogDataRoute();
void updateAllRuntimes();
String prepareLogData(int pumpIndex, String timeframe);
unsigned long aggregateDailyLogsReport(int pumpIndex, DateTime currentTime);
unsigned long aggregatePreviousDailyLogsReport(int pumpIndex, DateTime currentTime);
unsigned long aggregateMonthlyLogsReport(int pumpIndex, DateTime currentTime);
unsigned long aggregatePreviousMonthlyLogsReport(int pumpIndex, DateTime currentTime);
unsigned long aggregateYearlyLogsReport(int pumpIndex, DateTime currentTime);
unsigned long aggregatePreviousYearlyLogsReport(int pumpIndex, DateTime currentTime);
unsigned long aggregateDecadeLogsReport(int pumpIndex, DateTime currentTime);

// **New: Function to Send Updated Temperatures**
void sendUpdatedTemperatures();


void broadcastAlarmStateOverWebSocket();


#endif // WEBSERVERMANAGER_H
