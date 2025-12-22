#include "BikeComputer.h"
#include "BleStackAdapter.h"
#include "Cadence.h"
#include "ResistanceLevel.h"
#include "SystemWrapper.h"
#include <Arduino.h>
#include <unity.h>

/**
 * Hardware Saturated Test for ESP32
 *
 * Objective: Interactive testing of the full system on real hardware.
 * Instructions:
 * 1. Flash this test to the device.
 * 2. Connect via Serial (115200).
 * 3. Interact with the bike:
 *    - Spin the crank (Cadence).
 *    - Adjust resistance.
 *    - Connect with a BLE App (e.g., nRF Connect) to "Cycling Power".
 *
 * The test will run for 300 seconds (5 minutes) and log copious details.
 * It will PASS if the loop completes without crashing.
 */

// Use real system wrapper
SystemWrapper sys;
BleStackAdapter bleAdapter;
BikeComputerConfig config = {.ledPin = LED_BUILTIN, // Adjust if needed
                             .wakeupPin = 0,        // Example
                             .cadencePin = 34,
                             .resPositionPin = 32, // Check your pinout
                             .resMinLevelLimitPin = 35,
                             .resBackwardsDirectionIndicatorPin = 25,
                             .resForwardDirectionIndicatorPin = 26};

// We need to instantiate properly.
// Note: In main.cpp, these are usually global or static.
Cadence cadence(config.cadencePin, sys);
// Resistance pins need struct
ResistanceLevelPins resPins = {
    .limit = config.resMinLevelLimitPin,
    .backwards = config.resBackwardsDirectionIndicatorPin,
    .position = config.resPositionPin,
    .forwards = config.resForwardDirectionIndicatorPin};
ResistanceLevel resistance(resPins, sys);
BLECyclingPowerService bleService(bleAdapter);

BikeComputer computer(sys, cadence, resistance, bleService, config);

void setUp() {
  // Standard Arduino setup
  Serial.begin(115200);
  delay(2000); // Wait for serial
  Serial.println("=== STARTING SATURATED HARDWARE TEST ===");
  Serial.println("Spin the wheel! Move resistance! Connect BLE!");

  computer.setup();
}

void tearDown() {}

void test_interactive_loop() {
  unsigned long startTime = millis();
  unsigned long lastLog = 0;
  uint32_t initialHeap = ESP.getFreeHeap();

  // Run for 5 minutes
  const unsigned long DURATION = 300 * 1000;

  while (millis() - startTime < DURATION) {
    computer.update();

    // Log status every second
    if (millis() - lastLog > 1000) {
      lastLog = millis();

      float cad = cadence.cadence();
      uint8_t lvl = resistance.level();
      uint16_t revs = cadence.totalRevs();
      uint32_t currentHeap = ESP.getFreeHeap();

      Serial.printf("[STATUS] Time: %lu ms | Cadence: %.2f RPM | Level: %d | "
                    "Revs: %u | HEAP: %u\n",
                    millis(), cad, lvl, revs, currentHeap);

      if (bleService.isConnected()) {
        Serial.println(" -> BLE CONNECTED");
      } else {
        Serial.println(" -> BLE ADVERTISING/DISCONNECTED");
      }

      // Saturated test: Verify basic sanity
      TEST_ASSERT_TRUE_MESSAGE(currentHeap > 1000, "Heap critically low!");
      // Check for significant leak (soft check)
      if (currentHeap < initialHeap - 5000) {
        Serial.println("[WARNING] Heap dropped significantly (>5KB)");
      }
    }
  }

  uint32_t finalHeap = ESP.getFreeHeap();
  Serial.printf("=== TEST COMPLETE ===\nInitial Heap: %u | Final Heap: %u | "
                "Delta: %d\n",
                initialHeap, finalHeap, (int)finalHeap - (int)initialHeap);

  // Final validation
  TEST_ASSERT_INT_WITHIN(2000, initialHeap,
                         finalHeap); // Allow small fragmentation, but no leak
}

void setup() {
  // Unity harness
  UNITY_BEGIN();
  RUN_TEST(test_interactive_loop);
  UNITY_END();
}

void loop() {
  // Done
}
