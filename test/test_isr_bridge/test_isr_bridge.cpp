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
    ResistanceLevel res(PIN_DUMMY, PIN_DUMMY, PIN_RES_POS, PIN_DUMMY, mockSysBridge);

    // 1. Enable
    res.enableInterrupt();

    // 2. Setup Pins for Direction (Forward)
    // The ISR will read these!
    // Since mockSysBridge is shared, we set state there.
    mockSysBridge.setPinState(PIN_DUMMY, false); // Back/Fwd mapped to dummy in constructor?
    // Wait, constructor uses specific pin numbers.
    // In this test: backwardsPin=0, forwardsPin=0.
    // This is ambiguous for the mock if both use pin 0.

    // Let's perform a cleaner setup:
    ResistanceLevel res2(10, 11, 23, 12, mockSysBridge);
    res2.enableInterrupt();

    // Set Forward: Fwd(12)=High, Back(10)=Low
    mockSysBridge.setPinState(12, true);
    mockSysBridge.setPinState(10, false);

    // 3. Simulate Interrupt
    mockSysBridge.setMillis(2000); // T=2000
    ResistanceLevel::isrPosition();

    // 4. Verify
    TEST_ASSERT_EQUAL(1, res2.getPositionChangeCounter());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cadence_isr_bridge);
    RUN_TEST(test_resistance_isr_bridge);
    return UNITY_END();
}