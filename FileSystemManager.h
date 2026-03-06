// FileSystemManager.h - Updated version with externs only, no includes for libraries
#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

// Include necessary headers for types
#include <LittleFS.h>  // For LittleFS_QSPI, File
#include <arduino_freertos.h>  // For SemaphoreHandle_t

// Extern declarations
extern LittleFS_QSPI FlashFS;
extern SemaphoreHandle_t fileSystemMutex;

// Add this for g_fileSystemReady
extern bool g_fileSystemReady;

// Function prototypes (use const char* for strings to avoid String type)
File openLogFileUnlocked(const char* filename, const char* mode);
File openLogFileLocked(const char* filename, const char* mode);
void closeLogFileLocked(File& f);
const char* getFSStatsString();
void fileSystemCleanupTask(void *pvParameters);
bool takeFileSystemMutexWithRetry(const char *tag, unsigned long perAttemptTicks, int maxAttempts);  // Use unsigned long for TickType_t if needed

#endif // FILESYSTEM_MANAGER_H