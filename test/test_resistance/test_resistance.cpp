#include <unity.h>
#include "ResistanceLevel.h"
#include "MockSystemWrapper.h"

// Define dummy pins for testing
const uint8_t BACKWARDS_PIN = 4;
const uint8_t LIMIT_PIN = 18;
const uint8_t FORWARD_PIN = 19;
const uint8_t POSITION_PIN = 23;

MockSystemWrapper mockSys;
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
    return UNITY_END();
}