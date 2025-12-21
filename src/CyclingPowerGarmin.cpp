#ifndef NATIVE_TEST
// Bluedroid is incompatible with Garmin
// https://github.com/ihaque/pelomon/blob/main/pelomon/ble_constants.h
#include "ble_constants.h"
#include "Cadence.h"
// #include <Power.h>
#include <ResistanceLevel.h>
#include <Arduino.h>
#include <BLECyclingPowerService.h>
#include <Power.h>
#include "SystemWrapper.h"
#include "BikeComputer.h"
#include "NimBleStackAdapter.h"

static constexpr uint8_t backwordsPin = 4;
static constexpr uint8_t limitPin = 18;
static constexpr uint8_t forwardPin = 19;
static constexpr uint8_t positionPin = 23;

static ArduinoSystemWrapper sys;
static ResistanceLevel lvl(backwordsPin, limitPin, positionPin, forwardPin, sys);
static Cadence freq(15, sys);
static NimBleStackAdapter bleAdapter;
static BLECyclingPowerService bleService(bleAdapter);
static BikeComputerConfig config = {
  .ledPin = 27,
  .wakeupPin = 15,
  .cadencePin = 15,
  .resPositionPin = positionPin,
  .resMinLevelLimitPin = limitPin,
  .resBackwardsDirectionIndicatorPin = 4,  // Included
  .resForwardDirectionIndicatorPin = 19    // Included
};
static BikeComputer computer(sys, freq, lvl, bleService, config);

// --- FIX START: Guard setup/loop from Embedded Tests ---
// We defined -D EMBEDDED_TEST in platformio.ini
#if !defined(EMBEDDED_TEST) && !defined(UNIT_TEST)

void setup()
{
  pinMode(config.ledPin, OUTPUT);
  Serial.begin(115200);
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