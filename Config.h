#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <LittleFS.h>
#include "DiagConfig.h"

// FreeRTOS
#include <arduino_freertos.h>
#include <semphr.h>
#include <LittleFS.h>


// === ADD THESE THREE LINES ===
extern LittleFS_QSPI FlashFS;     // matches FileSystemManager.h
#define LittleFS FlashFS          // makes ALL your existing LittleFS. calls work instantly
// =============================

// ==================== DEFAULTS ====================
#define DEFAULT_TIMEZONE_ID     "UTC"      // String, not int!
#define DEFAULT_DST_ENABLED     false
// =================================================


// ====================== DIAGNOSTICS ======================
#ifndef ENABLE_SERIAL_DIAGNOSTICS
  #define ENABLE_SERIAL_DIAGNOSTICS 1
#endif
#ifndef DIAG_SERIAL_DEFAULT_ENABLE
  #define DIAG_SERIAL_DEFAULT_ENABLE 1
#endif
#ifndef DIAG_SERIAL_DEFAULT_MASK
  #define DIAG_SERIAL_DEFAULT_MASK DBG_DEFAULT_DEV_MASK
#endif

// ====================== FILESYSTEM PATHS ======================
#define DIAG_SERIAL_CONFIG_DIR    "/Json_Config_Files"
#define DIAG_SERIAL_CONFIG_PATH   "/Json_Config_Files/diag_serial_config.json"
#define SYSTEM_CONFIG_PATH        "/Json_Config_Files/system_config.json"
#define SYSTEM_CONFIG_PATH_OLD    "/system_config.json"          // <-- added for migration
#define TIME_CONFIG_PATH          "/Json_Config_Files/time_config.json"
#define TIME_CONFIG_PATH_OLD      "/time_config.json"            // <-- added

// ====================== HANDLES ======================
extern SemaphoreHandle_t pumpStateMutex;
extern SemaphoreHandle_t temperatureMutex;
extern SemaphoreHandle_t fileSystemMutex;

// ====================== TEENSY 4.1 PIN MAP ======================
const int pinSDA = 18;
const int pinSCL = 19;
const int FURNACE_HEATING_PIN = 22;
const int DHW_HEATING_PIN     = 23;
const int ONE_WIRE_BUS_1      = 2;
const int ONE_WIRE_BUS_2      = 3;

#define MAX31865_CS_PIN   10
#define MAX31865_DO_PIN   12
#define MAX31865_DI_PIN   11
#define MAX31865_CLK_PIN  13

const int PANEL_LEAD_PUMP_RELAY = 4;
const int PANEL_LAG_PUMP_RELAY  = 5;
const int HEAT_TAPE_RELAY       = 6;
const int CIRC_PUMP_RELAY       = 7;
const int DHW_PUMP_RELAY        = 8;
const int STORAGE_HEAT_RELAY    = 9;
const int BOILER_CIRC_RELAY     = 24;
const int RECIRC_VALVE_RELAY    = 25;
const int Pump_9_Unused_Relay   = 28;
const int Pump_10_Unused_Relay  = 29;

#define PUMP_OFF  0
#define PUMP_ON   1
#define PUMP_AUTO 2

extern int pumpModes[10];
extern int pumpStates[10];
const int numPumps = 10;
extern const int pumpPins[10];

// ====================== PARAMETERS ======================
inline constexpr float DEFAULT_PanelOnDifferential      = 30.0f;
inline constexpr float DEFAULT_PanelLowDifferential     = 15.0f;
inline constexpr float DEFAULT_PanelOffDifferential     = 3.0f;
inline constexpr float DEFAULT_panelTminimum            = 125.0f;
inline constexpr float DEFAULT_StorageHeatingLimit      = 130.0f;
inline constexpr float DEFAULT_Circ_Pump_On             = 5.0f;
inline constexpr float DEFAULT_Circ_Pump_Off            = 5.0f;
inline constexpr float DEFAULT_Heat_Tape_On             = 35.0f;
inline constexpr float DEFAULT_Heat_Tape_Off            = 45.0f;
inline constexpr float DEFAULT_Boiler_Circ_On           = 106.0f;
inline constexpr float DEFAULT_Boiler_Circ_Off          = 110.0f;

// NEW FREEZE PROTECTION DEFAULTS
inline constexpr float  DEFAULT_CollectorFreezeTempF     = 32.0f;
inline constexpr uint8_t DEFAULT_CollectorFreezeConfirmMin = 5;
inline constexpr uint8_t DEFAULT_CollectorFreezeRunMin    = 3;
inline constexpr uint8_t DEFAULT_COLLECTOR_FREEZE_SENSORS[15] = {1,2,0}; // example - change as needed
inline constexpr float  DEFAULT_LineFreezeTempF          = 32.0f;
inline constexpr uint8_t DEFAULT_LineFreezeConfirmMin    = 5;
inline constexpr uint8_t DEFAULT_LineFreezeRunMin        = 3;
inline constexpr uint8_t DEFAULT_LINE_FREEZE_SENSORS[15] = {3,4,0}; // example

#define NUM_TEMP_SENSORS 14

extern const char* SENSOR_NAMES[15];
extern const char* SENSOR_FILE_NAMES[15];

float getTempByIndex(int idx);

// ====================== CONFIG STRUCTS ======================
struct SystemConfig {
    bool     diagSerialEnable;
    uint32_t diagSerialMask;
    float panelTminimumValue;
    float panelOnDifferential;
    float panelLowDifferential;
    float panelOffDifferential;
    float boilerCircOn;
    float boilerCircOff;
    float storageHeatingLimit;
    float circPumpOn;
    float circPumpOff;
    float heatTapeOn;
    float heatTapeOff;

    // NEW FREEZE PROTECTION
    float  collectorFreezeTempF;
    uint8_t collectorFreezeConfirmMin;
    uint8_t collectorFreezeRunMin;
    uint8_t collectorFreezeSensors[15];

    float  lineFreezeTempF;
    uint8_t lineFreezeConfirmMin;
    uint8_t lineFreezeRunMin;
    uint8_t lineFreezeSensors[15];
};

struct TimeConfig {
    String timeZoneId;
    bool   dstEnabled;
};

extern SystemConfig g_config;
extern TimeConfig   g_timeConfig;

// ====================== GLOBALS ======================
extern String g_tempWsPayload;
extern volatile bool g_sendTemperatures;   // fixed type

// AlarmManager functions
void AlarmManager_begin();
void AlarmHistory_begin();
void setupAlarmRoutes();




// ==================== Globals ====================
extern SemaphoreHandle_t pumpStateMutex;
extern SemaphoreHandle_t temperatureMutex;
extern SemaphoreHandle_t fileSystemMutex;

extern bool needToUpdatePumpRuntimes;
extern String g_tempWsPayload;
extern volatile bool g_sendTemperatures;   // note: volatile if used in ISRs

extern const int pumpPins[];               // or uint8_t if you prefer
// =================================================

// Define them here (or in a new Globals.cpp later)
SemaphoreHandle_t pumpStateMutex = nullptr;
SemaphoreHandle_t temperatureMutex = nullptr;
SemaphoreHandle_t fileSystemMutex = nullptr;

String g_tempWsPayload = "";
volatile bool g_sendTemperatures = false;
// std::mutex g_tempWsPayloadMutex;  // Uncomment if using std::mutex



#endif