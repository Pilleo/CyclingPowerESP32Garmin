#include <unity.h>
#include "../../src/BikeComputer.h"
#include "../common/MockSystemWrapper.h"
#include "MockBleService.h"
#include "../../src/CadenceLogic.h"
#include "../../src/ResistanceLogic.h"
#include "../../src/drivers/ISensorDriver.h"
#include <new>

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
};

// Pin definitions
static constexpr uint8_t PIN_CADENCE = 15;
static constexpr uint8_t PIN_BACK = 4;
static constexpr uint8_t PIN_LIMIT = 18;
static constexpr uint8_t PIN_FWD = 19;
static constexpr uint8_t PIN_POS = 23;
static constexpr uint8_t PIN_LED = 27;

// Global instances
MockSystemWrapper mockSys;
MockBleService mockBle;
MockSensorDriver mockCadenceDriver;
MockSensorDriver mockResistanceDriver;

CadenceLogic* realCadence = nullptr;
ResistanceLogic* realResistance = nullptr;
BikeComputer* computer = nullptr;

void setUp() {
    mockSys.reset();
    mockCadenceDriver.eventCount = 0;
    mockCadenceDriver.lastEventTime = 0;
    mockResistanceDriver.eventCount = 0;
    mockResistanceDriver.lastEventTime = 0;

    mockBle.lastPowerSent = 0;
    mockBle.lastRevsSent = 0;
    mockBle.lastTimestampSent = 0;
    mockBle.notifyCalled = false;
    mockBle.startCalled = false;
    mockBle.callCount = 0;

    realCadence = new CadenceLogic(mockCadenceDriver, mockSys);
    realResistance = new ResistanceLogic({PIN_BACK, PIN_LIMIT, PIN_FWD}, mockResistanceDriver, mockSys);

    BikeComputerConfig testConfig = {
        .ledPin = PIN_LED,
        .wakeupPin = PIN_CADENCE,
        .cadencePin = PIN_CADENCE,
        .resPositionPin = PIN_POS,
        .resMinLevelLimitPin = PIN_LIMIT,
        .resBackwardsDirectionIndicatorPin = PIN_BACK,
        .resForwardDirectionIndicatorPin = PIN_FWD
    };

    computer = new BikeComputer(mockSys, *realCadence, *realResistance, mockBle, testConfig);
    computer->setup();
}

void tearDown() {
    delete computer;
    delete realResistance;
    delete realCadence;
}

void simulate_rpm(int rpm, int duration_ms) {
    if (rpm == 0) {
        mockSys.advanceTime(duration_ms);
        computer->update();
        return;
    }

    uint32_t interval = 60000 / rpm;
    int pulses = duration_ms / interval;

    for (int i = 0; i < pulses; i++) {
        mockSys.advanceTime(interval);
        mockCadenceDriver.triggerPulse(mockSys.millis());
        computer->update();
    }
}

void test_full_data_flow() {
    // Level 1 @ 60 RPM = 50 Watts.
    simulate_rpm(60, 5000);
    TEST_ASSERT_TRUE(mockBle.notifyCalled);
    TEST_ASSERT_INT_WITHIN(10, 50, mockBle.lastPowerSent);
}

void test_system_sleeps_after_timeout() {
    computer->update();
    TEST_ASSERT_FALSE(mockSys.wasSleepCalled);

    // Simulate some activity
    simulate_rpm(60, 1000);

    // Advance time beyond the deep sleep timeout
    mockSys.advanceTime(15 * 60 * 1000 + 1);
    computer->update();

    TEST_ASSERT_TRUE_MESSAGE(mockSys.wasSleepCalled, "System should have entered deep sleep");
    TEST_ASSERT_EQUAL(PIN_CADENCE, mockSys.sleepWakeupPin);
}

void test_ble_rate_limiting() {
    // Simulate some initial activity
    simulate_rpm(60, 2000);
    mockBle.callCount = 0;

    // Call update frequently over a short period
    for (int i = 0; i < 100; i++) {
        mockSys.advanceTime(10);
        computer->update();
    }

    // BLE update interval is 510ms. 100 * 10ms = 1000ms. Should send twice.
    TEST_ASSERT_INT_WITHIN(1, 2, mockBle.callCount);
}

void simulateResistanceLevelChange(int targetLevel) {
    mockSys.setPinState(PIN_FWD, true);
    mockSys.setPinState(PIN_BACK, false);

    int pulses_needed = 0;
    if (targetLevel == 5) {
        pulses_needed = 420; // Target the middle of the range for level 5 (411-429)
    }

    for (int i = 0; i < pulses_needed; i++) {
        mockSys.advanceTime(20); // Simulate time between pulses
        mockResistanceDriver.triggerPulse(mockSys.millis());
        computer->update();
    }

    mockSys.setPinState(PIN_FWD, false);
    computer->update();

    TEST_ASSERT_EQUAL(5, realResistance->level());
}

void test_resistance_data_pipeline() {
    // 1. Simulate Resistance Level 5
    simulateResistanceLevelChange(5);

    // 2. Simulate 90 RPM
    simulate_rpm(90, 5000);

    // 3. Assertions
    TEST_ASSERT_TRUE(mockBle.notifyCalled);

    // At Level 5 and 90 RPM, expected power is ~247W.
    TEST_ASSERT_INT_WITHIN(10, 247, mockBle.lastPowerSent);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_full_data_flow);
    RUN_TEST(test_system_sleeps_after_timeout);
    RUN_TEST(test_ble_rate_limiting);
    RUN_TEST(test_resistance_data_pipeline);
    return UNITY_END();
}
