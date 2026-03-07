// FileSystemManager.h - Updated version with forward declarations only
#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

// Forward declarations for types
class LittleFS_QSPI;
struct File;
typedef void * SemaphoreHandle_t;
typedef unsigned long TickType_t;

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