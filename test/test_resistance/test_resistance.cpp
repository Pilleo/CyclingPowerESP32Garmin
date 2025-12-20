#include <unity.h>
#include "../../src/ResistanceLevel.h"
#include "../common/MockSystemWrapper.h"

// Define dummy pins for testing
const uint8_t BACKWARDS_PIN = 4;
const uint8_t LIMIT_PIN = 18;
const uint8_t FORWARD_PIN = 19;
const uint8_t POSITION_PIN = 23;

static MockSystemWrapper mockSys;
ResistanceLevel *resistanceLevelInstance;

void setUp(void) {
    mockSys = MockSystemWrapper();
    mockSys.setPinState(LIMIT_PIN, false);
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, false);
    mockSys.setPinState(POSITION_PIN, false);

    resistanceLevelInstance = new ResistanceLevel(BACKWARDS_PIN, LIMIT_PIN, POSITION_PIN, FORWARD_PIN, mockSys);
}

void tearDown(void) {
    delete resistanceLevelInstance;
    resistanceLevelInstance = nullptr;
}

// Helper: Simulates a pin toggle.
// INCREMENTS 'time' by 10ms internally.
void simulate_position_change(unsigned long &time) {
    // Ensure enough time passes for the debouncing (sampleTime > 8)
    time += 10;
    mockSys.setMillis(time);

    // Toggle the position pin state and call level() to process the change
    bool currentState = mockSys.digitalRead(POSITION_PIN);
    mockSys.setPinState(POSITION_PIN, !currentState);
    resistanceLevelInstance->level();
}

// ==========================================
//           ORIGINAL TESTS
// ==========================================

void test_resistance_initialization() {
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());
}

void test_resistance_limit_switch_reset() {
    unsigned long time = 0;
    mockSys.setMillis(time);
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // 1. Move to Level 4 (Counter ~370)
    for (int i = 0; i < 370; ++i) {
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(4, resistanceLevelInstance->level());

    // 2. Simulate moving backwards
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, false);

    // 3. Simulate limitPin going HIGH
    mockSys.setPinState(LIMIT_PIN, true);

    // 4. Call level() which triggers the check
    resistanceLevelInstance->level();

    // 5. Assert that level is reset to 1
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());

    // 6. Verify positionChangeCounter is 0 by moving forward slightly
    mockSys.setPinState(LIMIT_PIN, false);
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    for (int i = 0; i < 50; ++i) {
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());
}

void test_resistance_level_hysteresis() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    mockSys.setPinState(LIMIT_PIN, false);

    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());
    TEST_ASSERT_EQUAL(0, resistanceLevelInstance->getPositionChangeCounter());

    // Simulate up to count 168 (Level 1)
    for (int i = 0; i < 168; ++i) {
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());
    TEST_ASSERT_EQUAL(168, resistanceLevelInstance->getPositionChangeCounter());

    // Count 169 (Level 1)
    simulate_position_change(time);
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());
    TEST_ASSERT_EQUAL(169, resistanceLevelInstance->getPositionChangeCounter());

    // Count 170 (Level 2)
    simulate_position_change(time);
    TEST_ASSERT_EQUAL(2, resistanceLevelInstance->level());
    TEST_ASSERT_EQUAL(170, resistanceLevelInstance->getPositionChangeCounter());
}

void test_resistance_direction_detection() {
    unsigned long time = 0;
    mockSys.setMillis(time);
    mockSys.setPinState(LIMIT_PIN, false);

    // 1. Forward
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    for (int i = 0; i < 50; ++i) {
        simulate_position_change(time);
    }
    uint16_t initialCounter = resistanceLevelInstance->getPositionChangeCounter();
    TEST_ASSERT_TRUE(initialCounter > 0);

    // 2. Backward
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, false);

    // 3. Toggle positionPin twice
    simulate_position_change(time); // Consumed by hysteresis
    simulate_position_change(time); // Decrements

    // 4. Assert decrease
    uint16_t counterAfterBackwardMove = resistanceLevelInstance->getPositionChangeCounter();
    TEST_ASSERT_EQUAL(initialCounter - 1, counterAfterBackwardMove);
}

// ==========================================
//           NEW EDGE CASE TESTS
// ==========================================

void test_resistance_dead_zone_speed() {
    unsigned long time = 0;
    mockSys.setMillis(time);
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // 1. Initial Pulse (Startup/Pause wake)
    time = 2000;
    simulate_position_change(time);
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->getPositionChangeCounter());

    // 2. Pulse at 100ms (Inside Dead Zone)
    // simulate_position_change adds 10ms. We want 100ms total.
    // So we add 90ms manually.
    time += 90;
    simulate_position_change(time);

    TEST_ASSERT_EQUAL_MESSAGE(1, resistanceLevelInstance->getPositionChangeCounter(), "Pulse at 100ms interval should be ignored");
}

void test_resistance_idle_noise() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    // 1. Get Counter to 1
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    time = 2000;
    simulate_position_change(time);
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->getPositionChangeCounter());

    // 2. Set IDLE state (Both Low)
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, false);

    // 3. Simulate Noise (Fast pulse)
    time += 15; // +10 in helper = 25ms total
    simulate_position_change(time);

    TEST_ASSERT_EQUAL_MESSAGE(1, resistanceLevelInstance->getPositionChangeCounter(), "Counter changed during IDLE state");

    // 4. Set INVALID state (Both High)
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, true);

    simulate_position_change(time);
    TEST_ASSERT_EQUAL_MESSAGE(1, resistanceLevelInstance->getPositionChangeCounter(), "Counter changed during INVALID state");
}

void test_resistance_gap_latching() {
    unsigned long time = 0;
    mockSys.setMillis(time);
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // 1. Move to end of Level 1 (Counter 29)
    time = 2000;
    simulate_position_change(time); // 1

    // Add 28 more pulses to reach 29
    for(int i=0; i<28; i++) {
        time += 20; // +20ms manual + 10ms helper = 30ms interval (FAST)
        simulate_position_change(time);
    }

    TEST_ASSERT_EQUAL(29, resistanceLevelInstance->getPositionChangeCounter());
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());

    // 2. Move into the Gap (Counter 30)
    time += 20;
    simulate_position_change(time);

    TEST_ASSERT_EQUAL(30, resistanceLevelInstance->getPositionChangeCounter());
    // CRITICAL: Level should still be 1, because 30 is not defined in any range
    TEST_ASSERT_EQUAL_MESSAGE(1, resistanceLevelInstance->level(), "Level should remain 1 when counter is in the gap");
}

void test_resistance_exact_timing_boundaries() {
    unsigned long time = 0;
    mockSys.setMillis(time);
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // 1. Establish baseline (Counter 1)
    time = 2000;
    simulate_position_change(time);
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->getPositionChangeCounter());

    // 2. Pulse at EXACTLY 50ms interval
    // Helper adds 10ms. We add 40ms. Total 50ms.
    // Logic is delta < 50. 50 is NOT < 50. Should be ignored.
    time += 40;
    simulate_position_change(time);
    TEST_ASSERT_EQUAL_MESSAGE(1, resistanceLevelInstance->getPositionChangeCounter(), "Pulse at exact 50ms boundary should be ignored");

    // 3. Pulse at EXACTLY 1700ms interval
    // Helper adds 10ms. We add 1690ms. Total 1700ms.
    // Logic is delta > 1700. 1700 is NOT > 1700. Should be ignored.
    time += 1690;
    simulate_position_change(time);
    TEST_ASSERT_EQUAL_MESSAGE(1, resistanceLevelInstance->getPositionChangeCounter(), "Pulse at exact 1700ms boundary should be ignored");
}

void test_limit_switch_ignored_while_forward() {
    unsigned long time = 0;
    mockSys.setMillis(time);
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // 1. Move to Counter 10
    time = 2000;
    simulate_position_change(time);
    for(int i=0; i<9; i++) {
        time += 20;
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(10, resistanceLevelInstance->getPositionChangeCounter());

    // 2. Trigger Limit Switch
    mockSys.setPinState(LIMIT_PIN, true);

    // 3. Call update
    resistanceLevelInstance->level();

    // 4. Assert NO reset
    TEST_ASSERT_EQUAL_MESSAGE(10, resistanceLevelInstance->getPositionChangeCounter(), "Limit switch should be ignored when moving forward");
}
void test_backward_stability_and_hysteresis() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    // 1. BUILD MOMENTUM FORWARD (Count to 5)
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // Initial pulse (Wake up)
    time = 2000;
    simulate_position_change(time);

    // 4 more fast pulses to reach 5
    for(int i=0; i<4; i++) {
        time += 20; // 30ms interval (FAST)
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(5, resistanceLevelInstance->getPositionChangeCounter());

    // 2. SWITCH DIRECTION IMMEDIATELY (FORWARD -> BACKWARD)
    // The "Hysteresis" Check: First pulse should NOT decrement.
    // It consumes the pulse to update 'movingForward' history from True to False.
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, false);

    time += 20; // Fast pulse
    simulate_position_change(time);

    // If logic is correct, this stays 5. If buggy, it drops to 4.
    TEST_ASSERT_EQUAL_MESSAGE(5, resistanceLevelInstance->getPositionChangeCounter(), "Hysteresis failed: First backward pulse should be ignored");

    // 3. CONTINUOUS BACKWARD
    // Now history is False. Next fast pulse SHOULD decrement.
    time += 20;
    simulate_position_change(time);

    TEST_ASSERT_EQUAL_MESSAGE(4, resistanceLevelInstance->getPositionChangeCounter(), "Stability failed: Second backward pulse should decrement");

    // 4. IDLE INTERRUPTION
    // Go Idle (Both False). This sets internal history (movingForward) to False.
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, false);

    // Simulate a loop cycle without pulse to flush state
    resistanceLevelInstance->level();

    // 5. RESUME BACKWARD (FAST)
    // Since history was reset to False during idle, a fast backward pulse
    // satisfies (!movingForward). It should decrement IMMEDIATELY.
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, false);

    time += 20;
    simulate_position_change(time);

    TEST_ASSERT_EQUAL_MESSAGE(3, resistanceLevelInstance->getPositionChangeCounter(), "Resume failed: Should decrement immediately after idle");
}

void test_noise_rejection_in_idle_state() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    // 1. Set up a counter value of 5
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    time = 2000;
    simulate_position_change(time); // Pulse 1
    for(int i=0; i<4; i++) {
        time += 20;
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(5, resistanceLevelInstance->getPositionChangeCounter());

    // 2. Set IDLE State (Both Pins LOW)
    // This represents the user stopping pedaling.
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, false);

    // 3. Simulate NOISE (Fast pulses while Idle)
    // In the "Buggy" logic (generic else), these are interpreted as
    // "Not Forward" -> "Backward" -> Decrement.
    // In the "Fixed" logic (strict else if), these are ignored.
    time += 20;
    simulate_position_change(time); // Noise Pulse 1

    TEST_ASSERT_EQUAL_MESSAGE(5, resistanceLevelInstance->getPositionChangeCounter(), "Counter decremented on noise while IDLE");

    time += 20;
    simulate_position_change(time); // Noise Pulse 2
    TEST_ASSERT_EQUAL_MESSAGE(5, resistanceLevelInstance->getPositionChangeCounter(), "Counter decremented on noise while IDLE");

    // 4. Set INVALID State (Both Pins HIGH)
    // This represents a short or sensor glitch.
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, true);

    time += 20;
    simulate_position_change(time); // Glitch Pulse
    TEST_ASSERT_EQUAL_MESSAGE(5, resistanceLevelInstance->getPositionChangeCounter(), "Counter decremented on INVALID sensor state");
}


void test_direction_switch_hysteresis() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    // 1. Move Forward (Counter = 5)
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    time = 2000;
    simulate_position_change(time);
    for(int i=0; i<4; i++) {
        time += 20;
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(5, resistanceLevelInstance->getPositionChangeCounter());

    // 2. Switch Direction to BACKWARD
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, false);

    // 3. First Backward Pulse (Fast)
    // Should be consumed by hysteresis (History update: True -> False)
    time += 20;
    simulate_position_change(time);
    TEST_ASSERT_EQUAL_MESSAGE(5, resistanceLevelInstance->getPositionChangeCounter(), "First backward pulse should be consumed by hysteresis");

    // 4. Second Backward Pulse (Fast)
    // Should decrement (History is now False)
    time += 20;
    simulate_position_change(time);
    TEST_ASSERT_EQUAL_MESSAGE(4, resistanceLevelInstance->getPositionChangeCounter(), "Second backward pulse should decrement");
}

void test_resume_after_long_pause_any_direction() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    // 1. Move Forward to 5
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    time = 2000;
    simulate_position_change(time);
    for(int i=0; i<4; i++) {
        time += 20;
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(5, resistanceLevelInstance->getPositionChangeCounter());

    // 2. Long Pause (>1700ms)
    time += 2000;

    // 3. Resume BACKWARD immediately
    // Since it's a "Long Pause", hysteresis should be bypassed or handled by isResumeAfterPause logic.
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, false);

    simulate_position_change(time);

    // Logic: isResumeAfterPause (True) OR ...
    // Decrement condition should pass.
    TEST_ASSERT_EQUAL_MESSAGE(4, resistanceLevelInstance->getPositionChangeCounter(), "Should decrement immediately after long pause");
}
void test_strict_idle_noise_rejection() {
    // This test ensures that when the bike is IDLE (both pins LOW),
    // electrical noise on the position pin does NOT cause the resistance
    // level to drop.
    unsigned long time = 0;
    mockSys.setMillis(time);

    // 1. Setup: Move Forward to Level 1 (Counter = 10)
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // Initial wake-up pulse
    time = 2000;
    simulate_position_change(time);

    // 9 fast pulses to reach counter 10
    for(int i=0; i<9; i++) {
        time += 20;
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(10, resistanceLevelInstance->getPositionChangeCounter());

    // 2. Set IDLE State (Both Pins LOW)
    // The user stops pedaling/adjusting.
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, false);

    // 3. Simulate Multiple Noise Pulses
    // In a "buggy" implementation (generic else), these would decrement the counter.
    // In the "correct" implementation (else if isMovingBack), these must be ignored.

    // Noise Pulse 1 (Updates history)
    time += 20;
    simulate_position_change(time);

    // Noise Pulse 2 (Would decrement if buggy)
    time += 20;
    simulate_position_change(time);

    // Noise Pulse 3 (Would decrement if buggy)
    time += 20;
    simulate_position_change(time);

    // Assert: Counter should still be exactly 10.
    TEST_ASSERT_EQUAL_MESSAGE(10, resistanceLevelInstance->getPositionChangeCounter(), "Counter decremented on repeated noise pulses in IDLE state");
}
/**
 * Test 3.3.1: Invalid Hardware State (Short Circuit)
 * Simulates a hardware fault where both Forward and Backward pins are HIGH.
 * The logic should handle this gracefully (likely by doing nothing).
 */
void test_resistance_invalid_hardware_state() {
    // Setup stable state
    unsigned long time = 1000;
    mockSys.setMillis(time);
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // Pulse once to ensure we are "moving forward"
    simulate_position_change(time);
    uint16_t initialCounter = resistanceLevelInstance->getPositionChangeCounter();

    // Enter Invalid State: Both HIGH
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, true);

    // Trigger position pulse
    simulate_position_change(time);

    // Expectation:
    // Logic: isMovingBack = (Back && !For) = (1 && 0) = 0
    // Logic: isMovingFor  = (For && !Back) = (1 && 0) = 0
    // Neither condition is met. Counter should remain unchanged.
    TEST_ASSERT_EQUAL_MESSAGE(initialCounter, resistanceLevelInstance->getPositionChangeCounter(), "Counter changed during invalid hardware state (Both HIGH)");
}

/**
 * Test 3.3.2: Counter Overflow
 * Verifies behavior when the 16-bit position counter wraps around 65535.
 * This helps decide if we need to reset the system or if wrapping is benign.
 */
void test_resistance_counter_overflow() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    // Setup Forward movement
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // We can't loop 65000 times in a unit test easily without taking too long?
    // It's just memory updates, it's fast.

    // 1. Manually hack the internal state if possible?
    // Since we can't access private members easily without friend classes,
    // we will rely on the fact that ResistanceLevel starts at 0 (or restored).
    // Actually, we can just instantiate a fresh one.

    // But to save time, we can assume it starts near 0.
    // We need to inject 65536 pulses.
    // Let's do a loop. Native test is fast.

    uint16_t target = 65535;

    // Note: This loop assumes the logic simply increments.
    // We must ensure time advances enough to pass debounce (8ms).
    // simulate_position_change adds 10ms.

    // This is technically an integration test of the overflow behavior.
    // We expect it to wrap to 0.

    // NOTE: This test might take a second to run.
    int limit = 66000;
    uint16_t lastCounter = 0;
    bool wrapped = false;

    for(int i=0; i < limit; i++) {
        simulate_position_change(time);
        uint16_t c = resistanceLevelInstance->getPositionChangeCounter();

        if (c < lastCounter) {
            wrapped = true;
            break;
        }
        lastCounter = c;
    }

    TEST_ASSERT_TRUE_MESSAGE(wrapped, "Counter did not wrap around 65535");

    // Verify it wraps to 0 or 1
    // 65535 + 1 = 0 (uint16_t)
    // If logic is `positionChangeCounter++`, it becomes 0.
    // Check level at 0. Level ranges say {0, 29, 1}. So it should be Level 1.
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());
}
/**
 * Test: Noisy Input Handling (Baseline)
 * Rationale: Captures how the current polling loop handles signal "chatter".
 * We simulate a transition where the pin bounces rapidly.
 * The system should filter this and count exactly 1 change.
 */
void test_resistance_noisy_transition_baseline() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    // Setup: Ready to move forward
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);

    // 1. Establish initial state with a "Long Pause" to guarantee system is active
    // This increments the counter once and sets 'prevDirectionWasForward = true'.
    time = 2000;
    simulate_position_change(time);

    uint16_t startCounter = resistanceLevelInstance->getPositionChangeCounter();

    // Capture current pin state to flip it correctly
    bool currentPinState = (mockSys.digitalRead(POSITION_PIN) != 0);
    bool targetState = !currentPinState;

    // SIMULATE NOISY EDGE
    // We use a delta of 30ms.
    // Logic: 30ms < 50ms (MOVEMENT_TIMEOUT_MS) -> isConsistent = true.
    // Since isConsistent && prevDirectionWasForward, it WILL increment.

    // 1. Valid Transition (Flip State)
    time += 30;
    mockSys.setMillis(time);
    mockSys.setPinState(POSITION_PIN, targetState);
    resistanceLevelInstance->level(); // Process valid edge

    // 2. Bounce (Revert to old state) - 2ms later
    // Delta = 2ms. Logic: 2ms < 8ms (DEBOUNCE_TIME_MS). Ignored.
    time += 2;
    mockSys.setMillis(time);
    mockSys.setPinState(POSITION_PIN, !targetState);
    resistanceLevelInstance->level();

    // 3. Bounce (Return to target state) - 2ms later
    // Delta from last valid = 4ms. Ignored.
    time += 2;
    mockSys.setMillis(time);
    mockSys.setPinState(POSITION_PIN, targetState);
    resistanceLevelInstance->level();

    // Assert: The counter should have incremented exactly ONCE from the startCounter.
    uint16_t endCounter = resistanceLevelInstance->getPositionChangeCounter();
    TEST_ASSERT_EQUAL_MESSAGE(startCounter + 1, endCounter, "System should count exactly 1 pulse despite noise");
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_resistance_initialization);
    RUN_TEST(test_resistance_limit_switch_reset);
    RUN_TEST(test_resistance_level_hysteresis);
    RUN_TEST(test_resistance_direction_detection);

    RUN_TEST(test_resistance_dead_zone_speed);
    RUN_TEST(test_resistance_idle_noise);
    RUN_TEST(test_resistance_gap_latching);
    RUN_TEST(test_resistance_exact_timing_boundaries);
    RUN_TEST(test_limit_switch_ignored_while_forward);

    RUN_TEST(test_backward_stability_and_hysteresis);
    RUN_TEST(test_noise_rejection_in_idle_state);
    RUN_TEST(test_strict_idle_noise_rejection);
    RUN_TEST(test_direction_switch_hysteresis);
    RUN_TEST(test_resume_after_long_pause_any_direction);
    RUN_TEST(test_resistance_invalid_hardware_state);
    RUN_TEST(test_resistance_counter_overflow);
RUN_TEST(test_resistance_noisy_transition_baseline);


    return UNITY_END();
}