#include <unity.h>
#include "Cadence.h"
#include "../common/MockSystemWrapper.h"

// Define a dummy pin for testing
const uint8_t TEST_PIN = 15;

MockSystemWrapper mockSysCore;
Cadence *cadenceCore;

void setUp(void) {
    mockSysCore.reset();
    cadenceCore = new Cadence(TEST_PIN, mockSysCore);
}

void tearDown(void) {
    delete cadenceCore;
}

/**
 * Test: Pure Logic Verification
 * Rationale: Verifies that onPulse() correctly drives the internal logic.
 * We simulate 2 revolutions over exactly 2 seconds (average 60 RPM).
 */
void test_cadence_core_logic_injection() {
    // 1. Initial State
    mockSysCore.setMillis(0);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadenceCore->cadence());

    // 2. Inject First Pulse at 1000ms
    // (1000 > 330ms debounce limit, so accepted)
    cadenceCore->onPulse(1000);

    // 3. Inject Second Pulse at 2000ms
    cadenceCore->onPulse(2000);

    // 4. Update System Time to 2000ms
    mockSysCore.setMillis(2000);

    // 5. Verify Calculation
    // Interval = 2000ms - 0ms (start) = 2000ms.
    // Revs = 2.
    // RPM = 60000 / (2000 / 2) = 60.0 RPM.
    TEST_ASSERT_EQUAL_FLOAT(60.0f, cadenceCore->cadence());
    TEST_ASSERT_EQUAL(2, cadenceCore->totalRevs());
}

/**
 * Test: Internal Debouncing
 * Rationale: Verifies that onPulse() ignores pulses that are too close together.
 */
void test_cadence_core_debounce() {
    mockSysCore.setMillis(0);

    // T=400: Pulse 1 (Accepted because > 330 startup)
    cadenceCore->onPulse(400);

    // T=410: Pulse 2 (Bounce - Rejected)
    // 410 - 400 = 10ms < 330ms limit
    cadenceCore->onPulse(410);

    // T=1400: Pulse 3 (Accepted)
    // 1400 - 400 = 1000ms > 330ms limit
    cadenceCore->onPulse(1400);

    mockSysCore.setMillis(1400);

    // Total revs should be 2 (Pulse 1, Pulse 3). Pulse 2 ignored.
    TEST_ASSERT_EQUAL(2, cadenceCore->totalRevs());
}

void test_reading_reports_captured_pulse_timestamp_after_delayed_evaluation() {
    cadenceCore->onPulse(1000);
    cadenceCore->onPulse(2000);
    mockSysCore.setMillis(3000);

    const CadenceReading reading = cadenceCore->reading();

    TEST_ASSERT_EQUAL_UINT32(2, reading.totalCrankRevolutions);
    TEST_ASSERT_EQUAL_UINT16(2048, reading.lastCrankEventTime1024);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 40.0F, reading.rpm);
    TEST_ASSERT_EQUAL(CadencePhase::Moving, reading.phase);
    TEST_ASSERT_EQUAL_FLOAT(reading.rpm, cadenceCore->cadence());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cadence_core_logic_injection);
    RUN_TEST(test_cadence_core_debounce);
    RUN_TEST(test_reading_reports_captured_pulse_timestamp_after_delayed_evaluation);
    return UNITY_END();
}
