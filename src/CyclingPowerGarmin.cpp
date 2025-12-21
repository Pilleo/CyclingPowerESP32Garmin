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
constexpr uint8_t backwordsPin = 4;
constexpr uint8_t limitPin = 18;
constexpr uint8_t forwardPin = 19;
constexpr uint8_t positionPin = 23;
constexpr uint8_t cadencePin = 15;
constexpr uint8_t ledPin = 27;
constexpr uint32_t serialBaudRate = 115200;

ArduinoSystemWrapper sys;
ResistanceLevel lvl(backwordsPin, limitPin, positionPin, forwardPin, sys);
Cadence freq(cadencePin, sys);
NimBleStackAdapter bleAdapter;
BLECyclingPowerService bleService(bleAdapter);
BikeComputerConfig config = {
  .ledPin = ledPin,
  .wakeupPin = cadencePin,
  .cadencePin = cadencePin,
  .resPositionPin = positionPin,
  .resMinLevelLimitPin = limitPin,
  .resBackwardsDirectionIndicatorPin = backwordsPin,
  .resForwardDirectionIndicatorPin = forwardPin
};
BikeComputer computer(sys, freq, lvl, bleService, config);
}

// --- FIX START: Guard setup/loop from Embedded Tests ---
// We defined -D EMBEDDED_TEST in platformio.ini
#if !defined(EMBEDDED_TEST) && !defined(UNIT_TEST)

void setup()
{
  pinMode(config.ledPin, OUTPUT);
  Serial.begin(serialBaudRate);
  Serial.println("Start");
  computer.setup();
}

void loop()
{
  computer.update();
}

#endif
// --- FIX END ---

#endif // End NATIVE_TEST
