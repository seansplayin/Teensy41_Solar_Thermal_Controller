#include "FileSystemManager.h"
#include "Logging.h" 
#include "Config.h"
#include <LittleFS.h>
#include <arduino_freertos.h>
#include "DiagLog.h"
#include <Arduino.h>


// Instantiate
LittleFS_QSPI FlashFS;

SemaphoreHandle_t fileSystemMutex = NULL;
bool g_fileSystemReady = false; 

void initializeFileSystem() {
    if (!fileSystemMutex) fileSystemMutex = xSemaphoreCreateMutex();

    LOG_CAT(DBG_FS, "[FS] Mounting FlashFS (LittleFS_QSPI)...\n");
    
    if (!FlashFS.begin()) {
        LOG_ERR("[FS] Mount failed. Formatting...\n");
        FlashFS.format(); 
        if (FlashFS.begin()) {
             g_fileSystemReady = true;
             LOG_CAT(DBG_FS, "[FS] Formatted and Mounted.\n");
        } else {
             g_fileSystemReady = false;
        }
    } else {
        LOG_CAT(DBG_FS, "[FS] Mounted successfully.\n");
        g_fileSystemReady = true;
    }
    
    if (g_fileSystemReady && !FlashFS.exists(DIAG_SERIAL_CONFIG_DIR)) {
        FlashFS.mkdir(DIAG_SERIAL_CONFIG_DIR);
    }
}

File openLogFileUnlocked(const String& filename, const char* mode) {
    if (!g_fileSystemReady) return File();
    uint8_t m = (mode[0] == 'r') ? FILE_READ : FILE_WRITE;
    return FlashFS.open(filename.c_str(), m);
}

File openLogFileLocked(const String& filename, const char* mode) {
    if (!takeFileSystemMutexWithRetry("openLocked", pdMS_TO_TICKS(1000), 1)) return File();
    return openLogFileUnlocked(filename, mode);
}

void closeLogFileLocked(File& f) {
    if (f) f.close();
    xSemaphoreGive(fileSystemMutex);
}

String getFSStatsString() {
    if (!g_fileSystemReady) return "{\"error\":\"FS Not Ready\"}";
    
    // ✅ Fixed API names: totalSize / usedSize
    size_t total = FlashFS.totalSize();
    size_t used = FlashFS.usedSize();
    
    return "{\"usedBytes\":" + String(used) + ",\"totalBytes\":" + String(total) + "}";
}

void enforceTemperatureLogDiskLimit() {}

bool takeFileSystemMutexWithRetry(const char *tag, TickType_t perAttemptTicks, int maxAttempts) {
    if (!fileSystemMutex) return false;
    for (int i=0; i<maxAttempts; i++) {
        if (xSemaphoreTake(fileSystemMutex, perAttemptTicks)) return true;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;
}
void closeAllOpenPumpLogs() {}
void LittleFSformat() { FlashFS.format(); }