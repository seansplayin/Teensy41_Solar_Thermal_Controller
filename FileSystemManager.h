// FileSystemManager.h - Updated version
#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

#include <Arduino.h>  // For String and F macro
#include <LittleFS.h>  // For LittleFS_QSPI, File
#include <arduino_freertos.h>  // For SemaphoreHandle_t, TickType_t

// Extern declarations
extern LittleFS_QSPI FlashFS;
extern SemaphoreHandle_t fileSystemMutex;
extern bool g_fileSystemReady;

// Function prototypes
File openLogFileUnlocked(const char* filename, const char* mode);
File openLogFileLocked(const char* filename, const char* mode);
void closeLogFileLocked(File& f);
const char* getFSStatsString();
void fileSystemCleanupTask(void *pvParameters);
bool takeFileSystemMutexWithRetry(const char *tag, TickType_t perAttemptTicks, int maxAttempts);

#endif // FILESYSTEM_MANAGER_H