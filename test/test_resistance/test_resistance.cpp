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
    // set stuff up here
    mockSys = MockSystemWrapper(); // Reset mock system wrapper
    resistanceLevelInstance = new ResistanceLevel(BACKWARDS_PIN, LIMIT_PIN, POSITION_PIN, FORWARD_PIN, mockSys);
}

void tearDown(void) {
    // clean stuff up here
    delete resistanceLevelInstance;
    resistanceLevelInstance = nullptr;
}

void simulate_position_change(unsigned long &time) {
    // Simulate a transition from HIGH to LOW (or vice-versa) on the position pin
    // that triggers one increment of positionChangeCounter in ResistanceLevel::level()

    // Ensure enough time passes for the debouncing (sampleTime > 8)
    time += 10;
    mockSys.setMillis(time);

    // Toggle the position pin state and call level() to process the change
    // This assumes positionChangeCounter increments on either HIGH->LOW or LOW->HIGH transitions,
    // as long as `forward` is true.
    bool currentState = mockSys.digitalRead(POSITION_PIN);
    mockSys.setPinState(POSITION_PIN, !currentState);
    resistanceLevelInstance->level();
}

void test_resistance_initialization() {
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());
}

void test_resistance_limit_switch_reset() {
    // 1. Simulate forward movement to increase positionChangeCounter to a value in Level 4 range (e.g., 370)
    unsigned long time = 0;
    mockSys.setMillis(time);
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    for (int i = 0; i < 370; ++i) { // Set positionChangeCounter to 370
        simulate_position_change(time);
    }
    // With 370 position changes, level should be 4
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

    // 6. To verify positionChangeCounter is 0, move forward a few steps
    // and check that level remains 1. If it was not reset, level would become 2 after a few steps.
    mockSys.setPinState(LIMIT_PIN, false); // limit switch no longer active
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    for (int i = 0; i < 50; ++i) { // 50 steps is less than threshold for level 2 (169)
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());
}

void test_resistance_level_hysteresis() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    // Ensure moving forward and limit switch inactive
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    mockSys.setPinState(LIMIT_PIN, false);

    // Initial check: Level should be 1, counter 0
    TEST_ASSERT_EQUAL(1, resistanceLevelInstance->level());
    TEST_ASSERT_EQUAL(0, resistanceLevelInstance->getPositionChangeCounter());

    // Simulate up to count 168 (Expected Level 1)
    for (int i = 0; i < 168; ++i) {
        simulate_position_change(time);
    }
    TEST_ASSERT_EQUAL_MESSAGE(1, resistanceLevelInstance->level(), "Level should be 1 at count 168");
    TEST_ASSERT_EQUAL_MESSAGE(168, resistanceLevelInstance->getPositionChangeCounter(), "Position counter should be 168");

    // Simulate one more click to count 169 (Still Expected Level 1, as range is >169)
    simulate_position_change(time);
    TEST_ASSERT_EQUAL_MESSAGE(1, resistanceLevelInstance->level(), "Level should still be 1 at count 169");
    TEST_ASSERT_EQUAL_MESSAGE(169, resistanceLevelInstance->getPositionChangeCounter(), "Position counter should be 169");

    // Simulate one more click to count 170 (Expected Level 2)
    simulate_position_change(time);
    TEST_ASSERT_EQUAL_MESSAGE(2, resistanceLevelInstance->level(), "Level should become 2 at count 170");
    TEST_ASSERT_EQUAL_MESSAGE(170, resistanceLevelInstance->getPositionChangeCounter(), "Position counter should be 170");
}

void test_resistance_direction_detection() {
    unsigned long time = 0;
    mockSys.setMillis(time);

    // Ensure limit switch is inactive
    mockSys.setPinState(LIMIT_PIN, false);

    // 1. Simulate forward movement to increase positionChangeCounter
    TEST_MESSAGE("Simulating forward movement to increase counter.");
    mockSys.setPinState(BACKWARDS_PIN, false);
    mockSys.setPinState(FORWARD_PIN, true);
    for (int i = 0; i < 50; ++i) {
        simulate_position_change(time);
    }
    uint16_t initialCounter = resistanceLevelInstance->getPositionChangeCounter();
    TEST_ASSERT_TRUE_MESSAGE(initialCounter > 0, "Initial counter should be > 0");
    // TEST_MESSAGE_INT("Initial Counter: ", initialCounter); // Removed to avoid compilation error

    // 2. Simulate moving backwards
    TEST_MESSAGE("Simulating backward movement.");
    mockSys.setPinState(BACKWARDS_PIN, true);
    mockSys.setPinState(FORWARD_PIN, false);

    // 3. Toggle positionPin twice (using simulate_position_change) to ensure prevDirection is updated and counter decrements
    simulate_position_change(time); // First backward step: updates prevDirection to false
    simulate_position_change(time); // Second backward step: now !wasInPositiveDirection() is true, counter decrements

    // 4. Assert that positionChangeCounter has decreased by 1 from the initial counter
    uint16_t counterAfterBackwardMove = resistanceLevelInstance->getPositionChangeCounter();
    TEST_ASSERT_EQUAL_MESSAGE(initialCounter - 1, counterAfterBackwardMove, "Counter should decrease by 1 after two backward steps");
}


int main() {
    UNITY_BEGIN();
    RUN_TEST(test_resistance_initialization);
    RUN_TEST(test_resistance_limit_switch_reset);
    RUN_TEST(test_resistance_level_hysteresis);
    RUN_TEST(test_resistance_direction_detection);
    return UNITY_END();
}
