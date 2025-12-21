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
#include "config.h"

namespace {
    ArduinoSystemWrapper sys;
    ResistanceLevel lvl(RESISTANCE_PINS, sys);
    Cadence freq(PIN_CADENCE, sys);
    NimBleStackAdapter bleAdapter;
    BLECyclingPowerService bleService(bleAdapter);
    BikeComputerConfig config = {
        .ledPin = PIN_LED,
        .wakeupPin = PIN_WAKEUP,
        .cadencePin = PIN_CADENCE,
        .resPositionPin = RESISTANCE_PINS.position,
        .resMinLevelLimitPin = RESISTANCE_PINS.limit,
        .resBackwardsDirectionIndicatorPin = RESISTANCE_PINS.backwards,
        .resForwardDirectionIndicatorPin = RESISTANCE_PINS.forwards
    };
    BikeComputer computer(sys, freq, lvl, bleService, config);
}

// --- FIX START: Guard setup/loop from Embedded Tests ---
// We defined -D EMBEDDED_TEST in platformio.ini
#if !defined(EMBEDDED_TEST) && !defined(UNIT_TEST)

void setup() {
    pinMode(config.ledPin, OUTPUT);
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("Start");
    computer.setup();
}

void loop() {
    computer.update();
}

#endif
// --- FIX END ---

#endif // End NATIVE_TEST
