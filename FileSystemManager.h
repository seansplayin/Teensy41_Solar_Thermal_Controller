#ifndef FILE_SYSTEM_MANAGER_H
#define FILE_SYSTEM_MANAGER_H

#include "Config.h" // Pulls in LittleFS before FreeRTOS

// ✅ Declare the specific Teensy Filesystem Object
extern LittleFS_QSPI FlashFS;

extern SemaphoreHandle_t fileSystemMutex;
extern bool g_fileSystemReady;

void initializeFileSystem();
File openLogFileUnlocked(const String& filename, const char* mode);
File openLogFileLocked(const String& filename, const char* mode);
void closeLogFileLocked(File& f);
String getFSStatsString();
void enforceTemperatureLogDiskLimit();
void LittleFSformat();
void closeAllOpenPumpLogs();

bool takeFileSystemMutexWithRetry(const char *tag, TickType_t perAttemptTicks, int maxAttempts);

#endif