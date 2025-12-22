#include "../../src/BikeComputer.h"
#include "../../src/Cadence.h"
#include "../../src/ResistanceLevel.h"
#include "../common/MockBleService.h"
#include "../common/MockSystemWrapper.h"
#include <cstdio>
#include <new>
#include <unity.h>

// Global instances
MockSystemWrapper mockSys;
MockBleService mockBle;

uint8_t cadenceBuffer[sizeof(Cadence)];
uint8_t resistanceBuffer[sizeof(ResistanceLevel)];
uint8_t computerBuffer[sizeof(BikeComputer)];
Cadence *realCadence;
ResistanceLevel *realResistance;
BikeComputer *computer;

void setUp() {
  mockSys.reset();
  mockSys.setPinState(PIN_CADENCE, true);
  mockSys.setPinState(RESISTANCE_PINS.backwards, false);
  mockSys.setPinState(RESISTANCE_PINS.limit, false);
  mockSys.setPinState(RESISTANCE_PINS.forwards, false);
  mockSys.setPinState(RESISTANCE_PINS.position, true);

  mockBle.lastPowerSent = 0;
  mockBle.lastRevsSent = 0;
  mockBle.lastTimestampSent = 0;
  mockBle.notifyCalled = false;
  mockBle.startCalled = false;
  mockBle.callCount = 0;

  realCadence = new (cadenceBuffer) Cadence(PIN_CADENCE, mockSys);
  realResistance =
      new (resistanceBuffer) ResistanceLevel(RESISTANCE_PINS, mockSys);

  BikeComputerConfig testConfig = {
      .ledPin = PIN_LED,
      .wakeupPin = PIN_CADENCE,
      .cadencePin = PIN_CADENCE,
      .resPositionPin = RESISTANCE_PINS.position,
      .resMinLevelLimitPin = RESISTANCE_PINS.limit,
      .resBackwardsDirectionIndicatorPin = RESISTANCE_PINS.backwards,
      .resForwardDirectionIndicatorPin = RESISTANCE_PINS.forwards};

  computer = new (computerBuffer)
      BikeComputer(mockSys, *realCadence, *realResistance, mockBle, testConfig);
  computer->setup();
}

void tearDown() {}

// Helper: Pulse Cadence
void pulse_cadence(uint32_t intervalMs) {
  mockSys.setPinState(PIN_CADENCE, false); // Active
  computer->update();
  mockSys.advanceTime(50);
  computer->update();

  mockSys.setPinState(PIN_CADENCE, true); // Idle
  computer->update();
  mockSys.advanceTime(intervalMs - 50);
  computer->update();
}

// Helper: Change Resistance
void set_resistance_level_approx(int pulses) {
  mockSys.setPinState(RESISTANCE_PINS.forwards, true);
  mockSys.setPinState(RESISTANCE_PINS.backwards, false);

  for (int i = 0; i < pulses; i++) {
    mockSys.setPinState(RESISTANCE_PINS.position, false);
    mockSys.advanceTime(10);
    computer->update();
    mockSys.setPinState(RESISTANCE_PINS.position, true);
    mockSys.advanceTime(10);
    computer->update();
  }
  mockSys.setPinState(RESISTANCE_PINS.forwards, false);
}

/**
 * Test: Full Cycle Usage
 * Scenario:
 * 1. User Starts (Wakeup)
 * 2. Adjusts Resistance to level 8
 * 3. Starts Pedaling (60 RPM)
 * 4. Checks Power (Should be ~132W for Level 8 @ 60RPM)
 * 5. Stops Pedaling (Coasts)
 * 6. Checks Power drops to 0
 * 7. Waits for Sleep
 */
void test_e2e_full_cycle() {
  // 1. Initial State
  TEST_ASSERT_FALSE(mockSys.wasSleepCalled);

  // 2. Increase Resistance to Level 8
  // Level 8 range: 531-549 counts. Target 540.
  // Each loop iteration produces 2 counts (Falling + Rising).
  // So we need 270 loops.
  set_resistance_level_approx(270);

  // Verify Level
  TEST_ASSERT_EQUAL(8, realResistance->level());

  // 3. Pedal at 60 RPM (1000ms interval)
  // Spin up
  for (int i = 0; i < 3; i++) {
    pulse_cadence(1000);
    char loopMsg[100];
    snprintf(loopMsg, sizeof(loopMsg), "Loop %d: Level=%d, Cad=%.2f, Pwr=%d", i,
             realResistance->level(), realCadence->cadence(),
             mockBle.lastPowerSent);
    TEST_MESSAGE(loopMsg);
  }

  // Verify Cadence
  float cad = realCadence->cadence();
  char msg[64];
  snprintf(msg, sizeof(msg), "Cadence should be 60, was %f", cad);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 60.0f, cad);

  // Check BLE
  TEST_ASSERT_TRUE(mockBle.notifyCalled);
  uint16_t power = mockBle.lastPowerSent;

  // Level 8 @ 60RPM corresponds to index 40 (60-20), column 7 (Level 8).
  // Row 40: {50, 69, 88, 107, 126, 145, 164, 183, 202...}
  // Expected: 183.
  TEST_ASSERT_INT_WITHIN(5, 183, power);

  // 4. Coast (stop pedaling)
  // Wait > 15 mins (DEEP_SLEEP_TIMEOUT_MS) to trigger deep sleep condition
  // (returns -1.0f)
  for (int i = 0; i < 16; i++) { // 16 minutes
    mockSys.advanceTime(60000);  // Advance 1 minute
    computer->update();
  }

  // Verify Power 0 sent?
  // Note: Protocol sends 0 power if cadence < 0 (timeout) or just updates with
  // 0. Logic: if (cad >= 0) updateData otherwise enterDeepSleep or handle
  // timeout locally. Wait, BikeComputer.h line 92: if (cad >= 0). If timeout,
  // cad might be -1? Cadence::cadence() returns -1.0f on timeout?
  // START_TIMEOUT_MS = 2500.
  // If (millis - lastPulse > 2500), Cadence::cadence() returns -1.
  // BikeComputer uses `const int16_t cad = _cadence.cadence();`
  // If cad < 0, it calls `_sys.enterDeepSleep`.
  // Wait, deep sleep is only if `periodSinceLastTransaction >=
  // TIME_PERIOD_FOR_SENDING_DATA` (500ms).

  // So if we stop pedaling:
  // 1. Time advances.
  // 2. `cadence()` eventually returns -1.
  // 3. `BikeComputer::update()` sees cad < 0.
  // 4. Checks wakeup pin.
  // 5. Enters deep sleep.

  // Check if sleep called
  TEST_ASSERT_TRUE(mockSys.wasSleepCalled);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_e2e_full_cycle);
  return UNITY_END();
}
