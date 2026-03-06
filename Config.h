#ifndef CONFIG_H
#define CONFIG_H
#include <arduino_freertos.h>
#include <Arduino.h>
#include <LittleFS.h>
#include "DiagConfig.h"
#include <semphr.h>
#include "FileSystemManager.h"




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


// ====================== TEENSY 4.1 PIN MAP ======================
constexpr int pinSDA = 18;
constexpr int pinSCL = 19;
constexpr int FURNACE_HEATING_PIN = 22;
constexpr int DHW_HEATING_PIN     = 23;
constexpr int ONE_WIRE_BUS_1      = 2;
constexpr int ONE_WIRE_BUS_2      = 3;

constexpr int MAX31865_CS_PIN   = 10;
constexpr int MAX31865_DO_PIN   = 12;
constexpr int MAX31865_DI_PIN   = 11;
constexpr int MAX31865_CLK_PIN  = 13;

constexpr int PANEL_LEAD_PUMP_RELAY = 4;
constexpr int PANEL_LAG_PUMP_RELAY  = 5;
constexpr int HEAT_TAPE_RELAY       = 6;
constexpr int CIRC_PUMP_RELAY       = 7;
constexpr int DHW_PUMP_RELAY        = 8;
constexpr int STORAGE_HEAT_RELAY    = 9;
constexpr int BOILER_CIRC_RELAY     = 24;
constexpr int RECIRC_VALVE_RELAY    = 25;
constexpr int Pump_9_Unused_Relay   = 28;
constexpr int Pump_10_Unused_Relay  = 29;

// Pump pin array (used by PumpManager)
constexpr int pumpPins[10] = {
    PANEL_LEAD_PUMP_RELAY,   // 0
    PANEL_LAG_PUMP_RELAY,    // 1
    HEAT_TAPE_RELAY,         // 2
    CIRC_PUMP_RELAY,         // 3
    DHW_PUMP_RELAY,          // 4
    STORAGE_HEAT_RELAY,      // 5
    BOILER_CIRC_RELAY,       // 6
    RECIRC_VALVE_RELAY,      // 7
    Pump_9_Unused_Relay,     // 8
    Pump_10_Unused_Relay     // 9
};

#define PUMP_OFF  0
#define PUMP_ON   1
#define PUMP_AUTO 2

extern int pumpModes[10];
extern int pumpStates[10];
const int numPumps = 10;


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



// ==================== Global Mutex ====================
extern SemaphoreHandle_t pumpStateMutex;
extern SemaphoreHandle_t temperatureMutex;


// =================================================


// ====================== GLOBALS ======================
extern volatile bool needToUpdatePumpRuntimes;
extern String g_tempWsPayload;
extern volatile bool g_sendTemperatures;   // note: volatile if used in ISRs
// ====================================================


#endif