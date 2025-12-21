#include <unity.h>
#include "ResistanceLogic.h"
#include "drivers/ISensorDriver.h"
#include "../common/MockSystemWrapper.h" // Assuming a common mock system is available

// Mock Sensor Driver for testing
class MockSensorDriver : public ISensorDriver {
public:
    uint32_t eventCount = 0;
    uint32_t lastEventTime = 0;

    void begin() override {}
    void update() override {} // No-op for the mock
    uint32_t getEventCount() const override { return eventCount; }
    uint32_t getLastEventTime() const override { return lastEventTime; }

    // Helper to simulate a pulse
    void triggerPulse(uint32_t timestamp) {
        eventCount++;
        lastEventTime = timestamp;
    }

    void reset() {
        eventCount = 0;
        lastEventTime = 0;
    }
};

// Define pins for the logic class (excluding the position pin, which is now handled by the driver)
const ResistanceLogicPins PINS = {
    .backwards = 4,
    .limit = 18,
    .forwards = 19
};

static MockSystemWrapper mockSys;
static MockSensorDriver mockDriver;
static ResistanceLogic* resistanceLogicInstance;

void setUp(void) {
    mockSys.reset();
    mockDriver.reset();

    // Set initial pin states for direction and limit switches
    mockSys.setPinState(PINS.limit, false);
    mockSys.setPinState(PINS.backwards, false);
    mockSys.setPinState(PINS.forwards, false);

    resistanceLogicInstance = new ResistanceLogic(PINS, mockDriver, mockSys);
    resistanceLogicInstance->begin();
}

void tearDown(void) {
    delete resistanceLogicInstance;
    resistanceLogicInstance = nullptr;
}

// Helper: Simulates a sensor pulse and updates the logic
void simulate_pulse(unsigned long &time, unsigned long timeIncrement = 30) {
    time += timeIncrement;
    mockDriver.triggerPulse(time);
    resistanceLogicInstance->update();
}

void test_resistance_initialization() {
    TEST_ASSERT_EQUAL(1, resistanceLogicInstance->level());
    TEST_ASSERT_EQUAL(0, resistanceLogicInstance->getPositionChangeCounter());
}

void test_resistance_level_hysteresis() {
    unsigned long time = 0;
    mockSys.setPinState(PINS.forwards, true);

    // Simulate up to count 169 (should still be Level 1)
    for (int i = 0; i < 169; ++i) {
        simulate_pulse(time);
    }
    TEST_ASSERT_EQUAL(1, resistanceLogicInstance->level());
    TEST_ASSERT_EQUAL(169, resistanceLogicInstance->getPositionChangeCounter());

    // One more pulse to cross the threshold to Level 2
    simulate_pulse(time);
    TEST_ASSERT_EQUAL(2, resistanceLogicInstance->level());
    TEST_ASSERT_EQUAL(170, resistanceLogicInstance->getPositionChangeCounter());
}

void test_resistance_limit_switch_reset() {
    unsigned long time = 0;
    mockSys.setPinState(PINS.forwards, true);

    // 1. Move to Level 4 (Counter ~370)
    for (int i = 0; i < 370; ++i) {
        simulate_pulse(time);
    }
    TEST_ASSERT_EQUAL(4, resistanceLogicInstance->level());

    // 2. Simulate moving backwards and hitting the limit switch
    mockSys.setPinState(PINS.forwards, false);
    mockSys.setPinState(PINS.backwards, true);
    mockSys.setPinState(PINS.limit, true);

    // 3. Update logic to process the limit switch state
    resistanceLogicInstance->update();

    // 4. Assert that level and counter are reset
    TEST_ASSERT_EQUAL(1, resistanceLogicInstance->level());
    TEST_ASSERT_EQUAL(0, resistanceLogicInstance->getPositionChangeCounter());
}

void test_direction_switch_hysteresis() {
    unsigned long time = 0;
    mockSys.setPinState(PINS.forwards, true);

    // 1. Move Forward (Counter = 5)
    for (int i = 0; i < 5; ++i) {
        simulate_pulse(time, 2000); // Use long pauses to ensure counter increments
    }
    TEST_ASSERT_EQUAL(5, resistanceLogicInstance->getPositionChangeCounter());

    // 2. Switch Direction to BACKWARD
    mockSys.setPinState(PINS.forwards, false);
    mockSys.setPinState(PINS.backwards, true);

    // 3. First Backward Pulse (Fast) - should be consumed by hysteresis
    simulate_pulse(time, 30);
    TEST_ASSERT_EQUAL_MESSAGE(5, resistanceLogicInstance->getPositionChangeCounter(), "First backward pulse should be consumed");

    // 4. Second Backward Pulse (Fast) - should decrement
    simulate_pulse(time, 30);
    TEST_ASSERT_EQUAL_MESSAGE(4, resistanceLogicInstance->getPositionChangeCounter(), "Second backward pulse should decrement");
}

void test_resume_after_long_pause() {
    unsigned long time = 0;
    mockSys.setPinState(PINS.forwards, true);

    // 1. Move forward to 5
    for (int i = 0; i < 5; ++i) {
        simulate_pulse(time, 30);
    }
    TEST_ASSERT_EQUAL(5, resistanceLogicInstance->getPositionChangeCounter());

    // 2. Long Pause (>1700ms) and resume forward
    simulate_pulse(time, 2000);
    TEST_ASSERT_EQUAL(6, resistanceLogicInstance->getPositionChangeCounter());

    // 3. Long Pause and resume backward
    mockSys.setPinState(PINS.forwards, false);
    mockSys.setPinState(PINS.backwards, true);
    simulate_pulse(time, 2000);
    TEST_ASSERT_EQUAL(5, resistanceLogicInstance->getPositionChangeCounter());
}

void test_noise_in_idle_state_is_ignored() {
    unsigned long time = 0;
    mockSys.setPinState(PINS.forwards, true);

    // 1. Move forward to 5
    for (int i = 0; i < 5; ++i) {
        simulate_pulse(time, 30);
    }
    TEST_ASSERT_EQUAL(5, resistanceLogicInstance->getPositionChangeCounter());

    // 2. Go to IDLE state (no direction)
    mockSys.setPinState(PINS.forwards, false);
    mockSys.setPinState(PINS.backwards, false);

    // 3. Simulate noise pulses
    simulate_pulse(time, 30);
    simulate_pulse(time, 30);

    // 4. Counter should not have changed
    TEST_ASSERT_EQUAL(5, resistanceLogicInstance->getPositionChangeCounter());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_resistance_initialization);
    RUN_TEST(test_resistance_level_hysteresis);
    RUN_TEST(test_resistance_limit_switch_reset);
    RUN_TEST(test_direction_switch_hysteresis);
    RUN_TEST(test_resume_after_long_pause);
    RUN_TEST(test_noise_in_idle_state_is_ignored);
    return UNITY_END();
}
