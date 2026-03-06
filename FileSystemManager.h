// FileSystemManager.h - Updated version with externs only, no includes for libraries
#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

#include <Arduino.h>  // For String and F macro
#include <arduino_freertos.h>  // For SemaphoreHandle_t, TickType_t
#include <LittleFS.h>  // For LittleFS_QSPI, File

// Extern declarations only
extern LittleFS_QSPI FlashFS;
extern SemaphoreHandle_t fileSystemMutex;
extern bool g_fileSystemReady;

// Function prototypes (use const char* for strings to avoid String type)
File openLogFileUnlocked(const char* filename, const char* mode);
File openLogFileLocked(const char* filename, const char* mode);
void closeLogFileLocked(File& f);
const char* getFSStatsString();
void fileSystemCleanupTask(void *pvParameters);
bool takeFileSystemMutexWithRetry(const char *tag, TickType_t perAttemptTicks, int maxAttempts);

#endif // FILESYSTEM_MANAGER_H