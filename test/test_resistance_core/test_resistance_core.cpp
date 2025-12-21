//
// Created by leanid on 20.12.25.
//
#include <unity.h>
#include "ResistanceLevel.h"
#include "../common/MockSystemWrapper.h"

// Pins don't matter for logic injection tests
const ResistanceLevelPins DUMMY_PINS = { .backwards = 0, .limit = 0, .position = 0, .forwards = 0 };

MockSystemWrapper mockSysResCore;
ResistanceLevel *resCore;

void setUp(void) {
    mockSysResCore.reset();
    resCore = new ResistanceLevel(DUMMY_PINS, mockSysResCore);
}

void tearDown(void) {
    delete resCore;
}

/**
 * Test: Pure Logic Increment
 * Rationale: Verify counter increases when forward flags are set, without using digitalRead.
 */
void test_res_core_increment() {
    // Initial State
    TEST_ASSERT_EQUAL(0, resCore->getPositionChangeCounter());

    // 1. Start Forward Movement (Long Pause > 1700ms accepted)
    // T=2000, Forward=True, Backward=False
    resCore->onPositionPulse(2000, true, false);

    TEST_ASSERT_EQUAL(1, resCore->getPositionChangeCounter());

    // 2. Continue Forward (Consistent < 50ms)
    // T=2030 (Delta 30ms)
    resCore->onPositionPulse(2030, true, false);

    TEST_ASSERT_EQUAL(2, resCore->getPositionChangeCounter());
}

/**
 * Test: Logic Debounce
 * Rationale: Verify pulses < 8ms are rejected.
 */
void test_res_core_debounce() {
    // Start at counter 10 (arbitrary setup via loop)
    for(int i=0; i<10; i++) {
        // T = 2000 + i*30
        resCore->onPositionPulse(2000 + (i*30), true, false);
    }
    TEST_ASSERT_EQUAL(10, resCore->getPositionChangeCounter());
    uint32_t lastTime = 2000 + (9*30); // 2270

    // Attempt Bounce (Delta 5ms)
    resCore->onPositionPulse(lastTime + 5, true, false);

    // Should ignore
    TEST_ASSERT_EQUAL(10, resCore->getPositionChangeCounter());

    // Valid Pulse (Delta 30ms)
    resCore->onPositionPulse(lastTime + 30, true, false);

    // Should accept
    TEST_ASSERT_EQUAL(11, resCore->getPositionChangeCounter());
}

/**
 * Test: Limit Reset Logic
 * Rationale: Verify onLimitReset clears the counter.
 */
void test_res_core_limit_reset() {
    // Set some value
    resCore->onPositionPulse(2000, true, false); // Counter 1
    TEST_ASSERT_EQUAL(1, resCore->getPositionChangeCounter());

    // Trigger Limit
    resCore->onLimitReset();

    TEST_ASSERT_EQUAL(0, resCore->getPositionChangeCounter());
    TEST_ASSERT_EQUAL(1, resCore->getLevel());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_res_core_increment);
    RUN_TEST(test_res_core_debounce);
    RUN_TEST(test_res_core_limit_reset);
    return UNITY_END();
}