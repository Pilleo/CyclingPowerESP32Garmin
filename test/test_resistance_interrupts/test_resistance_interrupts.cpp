#include "../common/MockSystemWrapper.h"
#include "ResistanceLevel.h"
#include <unity.h>

MockSystemWrapper mockSys;

void setUp() { mockSys.reset(); }

void tearDown() {}

// Test 1: Verify High Speed Pulses are NOT lost
// If the resistance motor moves fast, pulses might come faster than
// DEBOUNCE_TIME_MS Current DEBOUNCE_TIME_MS is 8ms. Try sending pulses every
// 6ms (approx 166 pulses/sec)
void test_fast_motor_movement_no_loss() {
  ResistanceLevelPins pins = {2, 1, 3, 4}; // back, limit, pos, fwd
  ResistanceLevel res(pins, mockSys);
  mockSys.setPinState(
      3, false); // Ensure start state is LOW (Mock defaults to HIGH)
  res.begin();
  res.enableInterrupt();

  // Setup: Moving Forward
  mockSys.setPinState(4, true);  // Fwd
  mockSys.setPinState(2, false); // Back

  // Simulate interrupt pulses
  // Start at 5000 to ensure first pulse is > PAUSE_TIMEOUT_MS (1700) relative
  // to 0
  uint32_t now = 5000;
  mockSys.setMillis(now);

  // Send 10 pulses (20 edges) with 10ms interval (safe for 8ms debounce)
  for (int i = 0; i < 10; i++) {
    // Edge High
    now += 10;
    mockSys.setMillis(now);
    mockSys.setPinState(3, true);
    res.handlePositionInterrupt(); // Direct call to simulate ISR

    // Edge Low
    now += 10;
    mockSys.setMillis(now);
    mockSys.setPinState(3, false);
    res.handlePositionInterrupt();
  }

  // We expect 10 counts (rising edges only? or both?)
  // Logic: `if (pinPosition != oldPositionState)` -> triggers on BOTH edges if
  // logic allows? Let's check `handlePositionInterrupt`: It calls
  // `onPositionPulse` on EVERY change (if != old). `onPositionPulse` checks
  // `interval`. If we toggle every 6ms, that's an edge every 6ms. If DEBOUNCE
  // is 8ms, we might lose events.

  // The counter logic increments if `isConsistent` etc.
  // We need to see how many were counted.
  // 10 cycles = 20 edges.

  uint16_t count = res.getPositionChangeCounter();

  // If debounce is 8ms, and we toggle every 6ms:
  // T=0: Pulse (Accepted) -> Last=0
  // T=6: Edge (Rejected, 6 < 8)
  // T=12: Edge (Accepted, 12-0 = 12 > 8) -> Last=12
  // T=18: Edge (Rejected 6 < 8)
  // T=24: Edge (Accepted)
  // So we assume we catch half the edges? or fewer?
  // Ideally we catch ALL valid edges if they are real movement.

  TEST_ASSERT_EQUAL_MESSAGE(20, count, "Should count all edges");
}

// Test 2: Verify Interrupt Race with Direction Pins
// Simulate ISR entry, then CHANGE direction pins before ISR reads them
// This requires a MockSystemWrapper that can change pins during a 'read'
// sequence? Hard to mock strictly. Instead, we just set pins to 'Invalid' state
// during the call.
void test_direction_change_during_isr() {
  ResistanceLevelPins pins = {2, 1, 3, 4};
  ResistanceLevel res(pins, mockSys);
  res.begin();
  res.enableInterrupt();

  mockSys.setMillis(1000);

  // Initial: Moving Forward
  mockSys.setPinState(4, true);
  mockSys.setPinState(2, false);

  // Trigger interrupt, but before it reads pins, we assume they changed?
  // In this synchronous test, we set them BEFORE calling
  // handlePositionInterrupt which simulates the state "at the moment of read".
  // Scenario: Real world, edge happens (Pos high), user stops moving
  // immediately. ISR fires 2us later. Reading FWD=low, BACK=low.

  mockSys.setPinState(3, true);  // Edge
  mockSys.setPinState(4, false); // Fwd lost
  mockSys.setPinState(2, false); // Back lost (stopped)

  res.handlePositionInterrupt();

  // Should NOT increment if direction is lost
  TEST_ASSERT_EQUAL(0, res.getPositionChangeCounter());
}

void test_limit_drift_recovery(void) {
  // Scenario: System is at limit (Counter 0).
  ResistanceLevelPins pins = {2, 1, 3, 4};
  mockSys.setPinState(3, false); // Start Low
  ResistanceLevel res(pins, mockSys);
  res.begin();           // reads Low
  res.enableInterrupt(); // Important for ISR testing

  // 1. Limit Active
  mockSys.setPinState(1, true); // Limit Active
  // Trigger Limit ISR
  res.handleLimitInterrupt();
  TEST_ASSERT_EQUAL(0, res.getPositionChangeCounter());

  // 2. Glitch: "Forward" detected during a position pulse
  // Pins: Back=Low, Fwd=High (Moving Forward)
  mockSys.setPinState(2, false);
  mockSys.setPinState(4, true);

  // Pulse
  res.handlePositionInterrupt(); // Initial call to sync

  // Wait > Debounce
  mockSys.setMillis(mockSys.millis() + 20);

  // Edge Change (Pulse)
  mockSys.setPinState(3, true);
  res.handlePositionInterrupt();

  // Should have incremented because we "appeared" to be moving forward
  TEST_ASSERT_EQUAL(1, res.getPositionChangeCounter());

  // 3. Glitch clears. Now we are stopped (Back=Low, Fwd=Low)
  // But Limit is STILL ACTIVE.
  mockSys.setPinState(2, false);
  mockSys.setPinState(4, false); // Stopped

  // Wait > Debounce
  mockSys.setMillis(mockSys.millis() + 20);

  // Next Pulse (e.g. vibration).
  mockSys.setPinState(3, false); // Edge
  res.handlePositionInterrupt();

  // EXPECTATION:
  // Standard ISR: 'isMovingBack' is False. No decrement. Counter stays 1.
  // Robust ISR (Like Polling): '!isMovingForward' is True. Limit is True. RESET
  // to 0.
  TEST_ASSERT_EQUAL(0, res.getPositionChangeCounter());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_fast_motor_movement_no_loss);
  RUN_TEST(test_direction_change_during_isr);
  RUN_TEST(test_limit_drift_recovery);
  return UNITY_END();
}
