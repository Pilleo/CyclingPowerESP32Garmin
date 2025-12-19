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

static unsigned long lastDataSentTimestamp = 0;
static uint16_t currentPower = 0;

static constexpr uint8_t backwordsPin = 4;
static constexpr uint8_t limitPin = 18;
static constexpr uint8_t forwardPin = 19;
static constexpr uint8_t positionPin = 23;
static ArduinoSystemWrapper sys;
static ResistanceLevel lvl(backwordsPin, limitPin, positionPin, forwardPin, sys);
static Cadence freq(15, sys);

void setup()
{
  pinMode(27, OUTPUT);
  Serial.begin(115200);
  Serial.println("Start");
  BLECyclingPowerService::setup_BLE_server_multiconnect_NimBLE();
}

uint8_t lastLevel = 1;
uint32_t millisSinceLatRevolution = 0;

uint32_t lastBlink = 0;
int incomingRPM = 45; // for incoming serial data

void loop()
{
  // read the incoming byte:
  // incomingRPM = Serial.read();

  // say what you got:
  // Serial.print("I received: ");
  // Serial.println(incomingRPM, DEC);

  int const timeFrameForBlink = 60000 / incomingRPM;
  long const start = sys.millis();

  if (start - lastBlink >= timeFrameForBlink)
  {
    // Serial.println("time slot: ");
    // Serial.print(timeFrameForBlink);
    sys.digitalWrite(27, LOW);
    lastBlink = start;
  }

  else if (start - lastBlink >= 1)
  {
    sys.digitalWrite(27, HIGH);
  }

  const unsigned long ms = sys.millis();
  const uint8_t l = lvl.level();
  if (lastLevel != l)
  {
    Serial.print("Level:         ");
    Serial.println(l);
    lastLevel = l;
  }

  const int16_t cad = freq.cadence();
  currentPower = Power::power(l, cad);
  // Serial.println(cad);

  const unsigned long periodSinceLastTransaction = ms - lastDataSentTimestamp;

  int timePerioudForSendingData = 510;

  if (periodSinceLastTransaction >= timePerioudForSendingData)
  {
    // Serial.println("time to send things");

    if (cad >= 0)
    {
      BLECyclingPowerService::loop_BLE_server_multiconnect_NimBLE(currentPower, freq);

      lastDataSentTimestamp = ms;
      Serial.println(cad);
    }
    else
    {
      Serial.println("nothing to sent, rpm is -1");
      Serial.println(cad);
      // deviceConnected = false;
      const bool cadenceState = sys.digitalRead(GPIO_NUM_15) != 0;
      sys.enterDeepSleep(GPIO_NUM_15, static_cast<int>(!cadenceState));
    }
  }
}
#endif
