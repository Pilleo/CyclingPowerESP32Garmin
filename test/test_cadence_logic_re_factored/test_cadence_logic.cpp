#include <unity.h>
#include "CadenceLogic.h"
#include "drivers/ISensorDriver.h"
#include "../common/MockSystemWrapper.h"

// Mock Sensor Driver for testing
class MockSensorDriver : public ISensorDriver {
public:
    uint32_t eventCount = 0;
    uint32_t lastEventTime = 0;

    void begin() override {}
    void update() override {}
    uint32_t getEventCount() const override { return eventCount; }
    uint32_t getLastEventTime() const override { return lastEventTime; }

    void triggerPulse(uint32_t timestamp) {
        eventCount++;
        lastEventTime = timestamp;
    }

    void reset() {
        eventCount = 0;
        lastEventTime = 0;
    }
};

static MockSystemWrapper mockSys;
static MockSensorDriver mockDriver;
static CadenceLogic* cadenceLogicInstance;

void setUp(void) {
    mockSys.reset();
    mockDriver.reset();
    cadenceLogicInstance = new CadenceLogic(mockDriver, mockSys);
    cadenceLogicInstance->begin();
}

void tearDown(void) {
    delete cadenceLogicInstance;
    cadenceLogicInstance = nullptr;
}

// Helper to simulate a pulse and update the logic
void simulate_pulse(unsigned long timestamp) {
    mockSys.setMillis(timestamp);
    mockDriver.triggerPulse(timestamp);
    cadenceLogicInstance->update();
    cadenceLogicInstance->cadence(); // Call cadence to process the pulse
}

void test_cadence_initialization() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadenceLogicInstance->cadence());
    TEST_ASSERT_EQUAL(0, cadenceLogicInstance->totalRevs());
    TEST_ASSERT_FALSE(cadenceLogicInstance->shouldSleep());
}

void test_cadence_steady_60rpm() {
    // 60 RPM = 1 pulse per second (1000ms)
    for (int i = 1; i <= 10; ++i) {
        simulate_pulse(i * 1000);
    }
    mockSys.setMillis(10500); // Allow time for last calculation
    TEST_ASSERT_EQUAL_FLOAT(60.0f, cadenceLogicInstance->cadence());
}

void test_cadence_sudden_stop() {
    // Simulate 60 RPM
    simulate_pulse(1000);
    simulate_pulse(2000);
    mockSys.setMillis(2500);
    TEST_ASSERT_EQUAL_FLOAT(60.0f, cadenceLogicInstance->cadence());

    // Wait for idle timeout (5000ms)
    mockSys.setMillis(2000 + 5001);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadenceLogicInstance->cadence());
    TEST_ASSERT_FALSE(cadenceLogicInstance->shouldSleep());
}

void test_cadence_should_sleep() {
    simulate_pulse(1000);
    mockSys.setMillis(1000 + (60000 * 15) + 1); // Wait for deep sleep timeout
    cadenceLogicInstance->cadence(); // Update internal state
    TEST_ASSERT_TRUE(cadenceLogicInstance->shouldSleep());
}

void test_cadence_debouncing() {
    simulate_pulse(1000); // Valid pulse
    simulate_pulse(1100); // Debounced (interval is 100ms < 330ms)
    simulate_pulse(1200); // Debounced
    TEST_ASSERT_EQUAL(1, cadenceLogicInstance->totalRevs());

    simulate_pulse(1500); // Valid pulse (interval is 400ms > 330ms)
    TEST_ASSERT_EQUAL(2, cadenceLogicInstance->totalRevs());
}

void test_cadence_total_revs() {
    for (int i = 1; i <= 50; ++i) {
        simulate_pulse(i * 1000);
    }
    TEST_ASSERT_EQUAL(50, cadenceLogicInstance->totalRevs());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cadence_initialization);
    RUN_TEST(test_cadence_steady_60rpm);
    RUN_TEST(test_cadence_sudden_stop);
    RUN_TEST(test_cadence_should_sleep);
    RUN_TEST(test_cadence_debouncing);
    RUN_TEST(test_cadence_total_revs);
    return UNITY_END();
}
