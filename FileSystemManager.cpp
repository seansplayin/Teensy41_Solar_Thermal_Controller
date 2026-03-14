#include "FileSystemManager.h"
#include <Arduino.h>
#include <arduino_freertos.h>
#include "Logging.h"
#include "Config.h"
#include "DiagLog.h"

// Definitions
SemaphoreHandle_t fileSystemMutex = nullptr;
FlashFSWrapper FlashFS;

bool g_fileSystemReady = false;

// Mutex functions (moved from WebServerManager for shared use)
static bool takeFsMutex(TickType_t waitTicks) {
  if (!g_fileSystemReady || !fileSystemMutex) return false;
  return (xSemaphoreTake(fileSystemMutex, waitTicks) == pdTRUE);
}

static void giveFsMutex() {
  if (fileSystemMutex) xSemaphoreGive(fileSystemMutex);
}

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------
static size_t sumDirectoryBytesRecursive(const char* path) {
    File dir = SD.open(path, FILE_READ);
    if (!dir) return 0;

    if (!dir.isDirectory()) {
        size_t sz = dir.size();
        dir.close();
        return sz;
    }

    size_t total = 0;
    File entry = dir.openNextFile();
    while (entry) {
        if (entry.isDirectory()) {
            String childPath = entry.name();
            entry.close();
            total += sumDirectoryBytesRecursive(childPath.c_str());
        } else {
            total += entry.size();
            entry.close();
        }
        entry = dir.openNextFile();
    }

    dir.close();
    return total;
}

static bool copyFileForRename(const char* fromPath, const char* toPath) {
    File src = SD.open(fromPath, FILE_READ);
    if (!src) return false;

    File dst = SD.open(toPath, FILE_WRITE);
    if (!dst) {
        src.close();
        return false;
    }

    uint8_t buf[512];
    while (src.available()) {
        int n = src.read(buf, sizeof(buf));
        if (n <= 0) break;
        size_t written = dst.write(buf, (size_t)n);
        if (written != (size_t)n) {
            dst.close();
            src.close();
            return false;
        }
    }

    dst.flush();
    dst.close();
    src.close();
    return true;
}

// Add this missing function
bool deleteTemperatureLogsRecursive(const char* path) {
  if (!takeFsMutex(portMAX_DELAY)) return false;

  bool success = true;
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) {
    success = false;
  } else {
    File file = dir.openNextFile();
    while (file) {
      char fullPath[256];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", path, file.name());
      if (file.isDirectory()) {
        if (!deleteTemperatureLogsRecursive(fullPath)) success = false;
      } else {
        if (!SD.remove(fullPath)) success = false;
      }
      file.close();
      file = dir.openNextFile();
    }
    dir.close();
    if (success && strcmp(path, "/temperature_logs") != 0) {  // Don't delete root dir
      success = SD.rmdir(path);
    }
  }

  giveFsMutex();
  return success;
}

// -----------------------------------------------------------------------------
// FlashFSWrapper implementation
// -----------------------------------------------------------------------------
bool FlashFSWrapper::begin() {
    return SD.begin(BUILTIN_SDCARD);
}

bool FlashFSWrapper::format() {
    // No automatic SD card formatting here
    return false;
}

bool FlashFSWrapper::exists(const char* path) {
    return SD.exists(path);
}

bool FlashFSWrapper::exists(const String& path) {
    return SD.exists(path.c_str());
}

bool FlashFSWrapper::mkdir(const char* path) {
    return SD.mkdir(path);
}

bool FlashFSWrapper::mkdir(const String& path) {
    return SD.mkdir(path.c_str());
}

bool FlashFSWrapper::remove(const char* path) {
    return SD.remove(path);
}

bool FlashFSWrapper::remove(const String& path) {
    return SD.remove(path.c_str());
}

bool FlashFSWrapper::rmdir(const char* path) {
    if (!SD.exists(path)) return true;
    return false;
}

bool FlashFSWrapper::rmdir(const String& path) {
    return rmdir(path.c_str());
}

bool FlashFSWrapper::rename(const char* fromPath, const char* toPath) {
    if (!SD.exists(fromPath)) return false;

    if (SD.exists(toPath)) {
        SD.remove(toPath);
    }

    if (!copyFileForRename(fromPath, toPath)) {
        return false;
    }

    return SD.remove(fromPath);
}

bool FlashFSWrapper::rename(const String& fromPath, const String& toPath) {
    return rename(fromPath.c_str(), toPath.c_str());
}

File FlashFSWrapper::open(const char* path, uint8_t mode) {
    return SD.open(path, mode);
}

File FlashFSWrapper::open(const String& path, uint8_t mode) {
    return SD.open(path.c_str(), mode);
}

size_t FlashFSWrapper::totalBytes() const {
    return 0;  // not implemented yet for SD
}

size_t FlashFSWrapper::usedBytes() const {
    return sumDirectoryBytesRecursive("/");
}

// -----------------------------------------------------------------------------
// Public filesystem functions
// -----------------------------------------------------------------------------
void initializeFileSystem() {
    if (!fileSystemMutex) fileSystemMutex = xSemaphoreCreateMutex();

    LOG_CAT(DBG_FS, "[FS] Mounting SD card on BUILTIN_SDCARD...\n");

    if (!FlashFS.begin()) {
        LOG_ERR("[FS] SD mount failed.\n");
        g_fileSystemReady = false;
        return;
    }

    g_fileSystemReady = true;
    LOG_CAT(DBG_FS, "[FS] SD mounted successfully.\n");

    if (!FlashFS.exists("/Pump_Logs")) {
        FlashFS.mkdir("/Pump_Logs");
    }

    if (!FlashFS.exists("/Temperature_Logs")) {
        FlashFS.mkdir("/Temperature_Logs");
    }

    if (!FlashFS.exists("/Alarm_Logs")) {
        FlashFS.mkdir("/Alarm_Logs");
    }

    if (!FlashFS.exists(DIAG_SERIAL_CONFIG_DIR)) {
        FlashFS.mkdir(DIAG_SERIAL_CONFIG_DIR);
    }
}

File openLogFileUnlocked(const String& filename, const char* mode) {
    if (!g_fileSystemReady) return File();
    uint8_t m = (mode[0] == 'r') ? FILE_READ : FILE_WRITE;
    return FlashFS.open(filename.c_str(), m);
}

File openLogFileLocked(const String& filename, const char* mode) {
    if (!takeFsMutex(pdMS_TO_TICKS(1000))) return File();
    return openLogFileUnlocked(filename, mode);
}

void closeLogFileLocked(File& f) {
    if (f) f.close();
    giveFsMutex();
}

const char* getFSStatsString() {
    if (!g_fileSystemReady) return "{\"error\":\"FS Not Ready\"}";

    size_t total = FlashFS.totalBytes();
    size_t used = FlashFS.usedBytes();

    static char buf[64];
    snprintf(buf, sizeof(buf), "{\"usedBytes\":%zu,\"totalBytes\":%zu}", used, total);
    return buf;
}

void enforceTemperatureLogDiskLimit() {
    // Disabled for now in this Teensy SD skeleton
}

bool takeFileSystemMutexWithRetry(const char *tag, TickType_t perAttemptTicks, int maxAttempts) {
    if (!fileSystemMutex) return false;

    for (int i = 0; i < maxAttempts; i++) {
        if (xSemaphoreTake(fileSystemMutex, perAttemptTicks)) return true;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return false;
}

void fileSystemCleanupTask(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void LittleFSformat() {
    FlashFS.format();
}