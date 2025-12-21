//
// Created by leanid on 21.12.25.
//
#include <unity.h>
#include "../../src/Cadence.h"
#include "../../src/ResistanceLevel.h"
#include "../common/MockSystemWrapper.h"

MockSystemWrapper mockSysBridge;
const uint8_t PIN_CAD = 15;
const uint8_t PIN_RES_POS = 23;
// Other pins dummy
const uint8_t PIN_DUMMY = 0;

void setUp() {
    mockSysBridge.reset();
}

void tearDown() {}

void test_cadence_isr_bridge() {
    Cadence cad(PIN_CAD, mockSysBridge);
    cad.begin(); // Initialize hardware state

    // 1. Enable (Sets static instance)
    cad.enableInterrupt();

    // 2. Simulate Hardware Interrupt
    // We call the STATIC function directly.
    mockSysBridge.setMillis(1000); // T=1000
    Cadence::isr();

    // 3. Verify
    // 1000 > 330 startup delay, so it should count 1 rev (or start interval)
    // Wait, onPulse(1000) starts the timer. It doesn't count a rev yet until the NEXT pulse.

    // Let's trigger another one
    mockSysBridge.setMillis(2000);
    Cadence::isr();

    // Now we should have 2 revs (one interval calculated)
    TEST_ASSERT_EQUAL(2, cad.totalRevs());
}


void test_resistance_isr_bridge() {
    // 1. Setup with explicit pins
    // Back=10, Fwd=12, Pos=23, Limit=11
    const ResistanceLevelPins pins = { .backwards = 10, .limit = 11, .position = 23, .forwards = 12 };
    ResistanceLevel res2(pins, mockSysBridge);
    res2.begin(); // Initialize hardware state (reads default HIGH from mock)
    res2.enableInterrupt();

    // 2. Set Forward Direction
    // Fwd(12)=High, Back(10)=Low
    mockSysBridge.setPinState(12, true);
    mockSysBridge.setPinState(10, false);

    // 3. Simulate Interrupt Event
    // IMPORTANT: We must toggle the Position Pin (23) state!
    // The constructor read Default (High). We set it Low to simulate an edge.
    mockSysBridge.setPinState(23, false);

    mockSysBridge.setMillis(2000); // T=2000 (Long pause, so accepted)

    // 4. Trigger ISR Bridge
    ResistanceLevel::isrPosition();

    // 4.5. Process the interrupt flag
    res2.update();

    // 5. Verify
    TEST_ASSERT_EQUAL(1, res2.getPositionChangeCounter());
}


void test_resistance_limit_isr_bridge() {
    // 1. Setup
    const ResistanceLevelPins pins = { .backwards = 10, .limit = 11, .position = 23, .forwards = 12 };
    ResistanceLevel res(pins, mockSysBridge);
    res.begin(); // Initialize hardware state
    res.enableInterrupt();

    // 2. Establish a Level/Counter
    // FIX: Use 2000ms (>1700ms) so it counts as a valid "Wake Up" pulse
    res.onPositionPulse(2000, true, false);
    TEST_ASSERT_EQUAL_MESSAGE(1, res.getPositionChangeCounter(), "Setup failed to increment counter");

    // 3. Case A: Trigger Limit while Moving Forward (Should IGNORE)
    mockSysBridge.setPinState(12, true);  // Fwd = High
    mockSysBridge.setPinState(10, false); // Back = Low
    mockSysBridge.setPinState(11, true);  // Limit = High

    ResistanceLevel::isrLimit();

    // Process the flag - should be ignored
    res.update();

    TEST_ASSERT_EQUAL_MESSAGE(1, res.getPositionChangeCounter(), "Should ignore limit switch while moving forward");

    // 4. Case B: Trigger Limit while Idle (Should RESET)
    mockSysBridge.setPinState(12, false); // Fwd = Low

    ResistanceLevel::isrLimit();
    res.update(); // Process the flag - should reset

    TEST_ASSERT_EQUAL_MESSAGE(0, res.getPositionChangeCounter(), "Should reset counter when limit hit and not moving forward");
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cadence_isr_bridge);
    RUN_TEST(test_resistance_isr_bridge);
    RUN_TEST(test_resistance_limit_isr_bridge);
    return UNITY_END();
}