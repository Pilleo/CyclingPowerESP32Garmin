#ifndef NATIVE_TEST
// Bluedroid is incompatible with Garmin
// https://github.com/ihaque/pelomon/blob/main/pelomon/ble_constants.h
#include "Cadence.h"

#include <ResistanceLevel.h>
#include <Arduino.h>
#include <BLECyclingPowerService.h>

#include "SystemWrapper.h"
#include "BikeComputer.h"
#include "NimBleStackAdapter.h"

namespace {
    constexpr ResistanceLevelPins resistancePins = {
        .backwards = 4,
        .limit = 18,
        .position = 23,
        .forwards = 19
    };
    constexpr uint8_t cadencePin = 15;
    constexpr uint8_t ledPin = 27;
    constexpr uint32_t serialBaudRate = 115200;

    ArduinoSystemWrapper sys;
    ResistanceLevel lvl(resistancePins, sys);
    Cadence freq(cadencePin, sys);
    NimBleStackAdapter bleAdapter;
    BLECyclingPowerService bleService(bleAdapter);
    BikeComputerConfig config = {
        .ledPin = ledPin,
        .wakeupPin = cadencePin,
        .cadencePin = cadencePin,
        .resPositionPin = resistancePins.position,
        .resMinLevelLimitPin = resistancePins.limit,
        .resBackwardsDirectionIndicatorPin = resistancePins.backwards,
        .resForwardDirectionIndicatorPin = resistancePins.forwards
    };
    BikeComputer computer(sys, freq, lvl, bleService, config);
}

// --- FIX START: Guard setup/loop from Embedded Tests ---
// We defined -D EMBEDDED_TEST in platformio.ini
#if !defined(EMBEDDED_TEST) && !defined(UNIT_TEST)

void setup() {
    pinMode(config.ledPin, OUTPUT);
    Serial.begin(serialBaudRate);
    Serial.println("Start");
    computer.setup();
}

void loop() {
    computer.update();
}

#endif
// --- FIX END ---

#endif // End NATIVE_TEST
