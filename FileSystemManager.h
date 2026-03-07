// FileSystemManager.h - Updated version with forward declarations
#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

#include <Arduino.h>  // For F macro and other Arduino types

// Forward declarations to avoid including headers
class LittleFS_QSPI;
struct File;

// Forward for FreeRTOS types without conflict
struct QueueDefinition;
typedef struct QueueDefinition * QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;
typedef unsigned long TickType_t;

// Extern declarations
extern LittleFS_QSPIFlash;
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