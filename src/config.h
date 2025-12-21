#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include "ResistanceLevel.h"

// --- SENSOR MODE ---
// Define as 1 to use interrupts for sensor readings, 0 to use polling.
#define USE_INTERRUPTS 0

// --- PINOUT CONFIGURATION ---
constexpr uint8_t PIN_CADENCE = 15;
constexpr uint8_t PIN_LED = 27;
constexpr uint8_t PIN_WAKEUP = PIN_CADENCE;

constexpr ResistanceLevelPins RESISTANCE_PINS = {
    .backwards = 4,
    .limit = 18,
    .position = 23,
    .forwards = 19
};

// --- COMMUNICATION ---
constexpr uint32_t SERIAL_BAUD_RATE = 115200;

// --- BLE ---
// The time period in milliseconds for sending data over BLE.
constexpr int TIME_PERIOD_FOR_SENDING_DATA = 510;

#endif // CONFIG_H
