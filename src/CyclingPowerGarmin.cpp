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
#include "drivers/PollingDriver.h"
#include "drivers/InterruptDriver.h"
#include "ResistanceLogic.h"
#include "CadenceLogic.h"

namespace {
    constexpr ResistanceLogicPins resistancePins = {
        .backwards = config::RESISTANCE_BACKWARDS_PIN,
        .limit = config::RESISTANCE_LIMIT_PIN,
        .forwards = config::RESISTANCE_FORWARDS_PIN
    };
    constexpr uint32_t serialBaudRate = 115200;

    ArduinoSystemWrapper sys;

    // Drivers
    PollingDriver resistanceDriver(sys, config::RESISTANCE_POSITION_PIN);
    InterruptDriver cadenceDriver(sys, config::CADENCE_PIN);

    // Logic
    ResistanceLogic resistanceLogic(resistancePins, resistanceDriver, sys);
    CadenceLogic cadenceLogic(cadenceDriver, sys);

    NimBleStackAdapter bleAdapter;
    BLECyclingPowerService bleService(bleAdapter);
    BikeComputerConfig bikeConfig = {
        .ledPin = config::LED_PIN,
        .wakeupPin = config::WAKEUP_PIN,
        .cadencePin = config::CADENCE_PIN,
        .resPositionPin = config::RESISTANCE_POSITION_PIN,
        .resMinLevelLimitPin = config::RESISTANCE_LIMIT_PIN,
        .resBackwardsDirectionIndicatorPin = config::RESISTANCE_BACKWARDS_PIN,
        .resForwardDirectionIndicatorPin = config::RESISTANCE_FORWARDS_PIN
    };
    BikeComputer computer(sys, cadenceLogic, resistanceLogic, bleService, bikeConfig);
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
