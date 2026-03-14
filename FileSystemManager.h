// FileSystemManager.h
#ifndef FILESYSTEM_MANAGER_H
#define FILESYSTEM_MANAGER_H

#include <Arduino.h>
#include <SD.h>

// ----------------------------------------------------------------------------
// SD-backed wrapper that preserves your existing FlashFS-style calls
// ----------------------------------------------------------------------------
class FlashFSWrapper {
public:
    bool begin();
    bool format();  // Not supported for SD here; returns false

    bool exists(const char* path);
    bool exists(const String& path);

    bool mkdir(const char* path);
    bool mkdir(const String& path);

    bool remove(const char* path);
    bool remove(const String& path);

    bool rmdir(const char* path);
    bool rmdir(const String& path);

    bool rename(const char* fromPath, const char* toPath);
    bool rename(const String& fromPath, const String& toPath);

    File open(const char* path, uint8_t mode = FILE_READ);
    File open(const String& path, uint8_t mode = FILE_READ);

    size_t totalBytes() const;  // Returns 0 for now (not implemented yet)
    size_t usedBytes() const;   // Recursive SD usage scan

    // compatibility helpers
    size_t totalSize() const { return totalBytes(); }
    size_t usedSize()  const { return usedBytes();  }
};

// Global filesystem object used everywhere else in your project
extern FlashFSWrapper FlashFS;

// Forward for FreeRTOS types without conflict
struct QueueDefinition;
typedef struct QueueDefinition * QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;
typedef unsigned long TickType_t;

// Extern declarations
extern SemaphoreHandle_t fileSystemMutex;
extern bool g_fileSystemReady;

// Function prototypes
File openLogFileUnlocked(const String& filename, const char* mode);
File openLogFileLocked(const String& filename, const char* mode);
void closeLogFileLocked(File& f);
const char* getFSStatsString();
void fileSystemCleanupTask(void *pvParameters);
bool takeFileSystemMutexWithRetry(const char *tag, TickType_t perAttemptTicks, int maxAttempts);
void initializeFileSystem();
void LittleFSformat();

#endif // FILESYSTEM_MANAGER_H