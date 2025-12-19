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
static BikeComputerConfig config = {27, 15};
static BikeComputer computer(sys, freq, lvl, bleService, config);

void setup()
{
  pinMode(27, OUTPUT);
  Serial.begin(115200);
  Serial.println("Start");
  computer.setup();
}

void loop()
{
  computer.update();
}
#endif
