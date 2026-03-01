#include <Arduino.h> // Ensure base types are loaded
#include <Adafruit_MAX31865.h>
#include "Max31865-PT1000.h"
#include "Config.h"
#include "arduino_freertos.h"
#include "DiagLog.h"
#include "AlarmManager.h"

#define RREF      4300.0
#define RNOMINAL  1000.0

// Use Hardware SPI Constructor
// Note: If you use the software SPI constructor (4 pins), you must define them.
// For HW SPI on Teensy (Pins 10-13), we usually just pass the CS pin, 
// but Adafruit lib supports explicit pins too.
Adafruit_MAX31865 thermo = Adafruit_MAX31865(MAX31865_CS_PIN, MAX31865_DI_PIN, MAX31865_DO_PIN, MAX31865_CLK_PIN);

float pt1000Current = 0.0;
float pt1000Average = 0.0;

#define pt1000NumReadings 3
float pt1000Values[pt1000NumReadings];
int pt1000Index = 0;

void initPT1000Sensor() {
    thermo.begin(MAX31865_4WIRE);
    for (int i = 0; i < pt1000NumReadings; i++) {
        pt1000Values[i] = -999.0;
    }
}

float calculatePT1000Average(float values[], int numReadings) {
    float sum = 0.0;
    int count = 0;
    for (int i = 0; i < numReadings; i++) {
        if (values[i] > -100.0f) {
            sum += values[i];
            count++;
        }
    }
    return (count > 0) ? (sum / count) : -999.0f;
}

void updatePT1000Readings() {
    uint8_t fault = thermo.readFault();
    if (fault) {
        LOG_ERR("[PT1000] Fault 0x%02X\n", fault);
        thermo.clearFault();
        pt1000Current = -999.0;
    } else {
        float newF = thermo.temperature(RNOMINAL, RREF) * 1.8f + 32.0f;
        pt1000Current = newF;
        
        pt1000Values[pt1000Index] = newF;
        pt1000Index = (pt1000Index + 1) % pt1000NumReadings;
        pt1000Average = calculatePT1000Average(pt1000Values, pt1000NumReadings);
    }
}