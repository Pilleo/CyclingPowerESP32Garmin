#include <unity.h>
#include "drivers/PollingDriver.h"
#include "SystemWrapper.h"
#include <vector>

class MockSystemWrapper : public ISystemWrapper {
public:
    uint32_t ms = 0;
    std::vector<int> pinStates;

    MockSystemWrapper(int numPins) : pinStates(numPins, 0) {}

    void pinMode(uint8_t pin, uint8_t mode) override {}
    int digitalRead(uint8_t pin) override { return pinStates[pin]; }
    void digitalWrite(uint8_t pin, int val) override { pinStates[pin] = val; }
    uint32_t millis() override { return ms; }
    void attachInterrupt(uint8_t pin, void (*isr)(), int mode) override {}
    void enableInterrupts() override {}
    void disableInterrupts() override {}
    void enterDeepSleep(uint8_t wakeupPin, int mode) override {}
};

const uint8_t TEST_PIN = 5;

void test_polling_driver_initialization() {
    MockSystemWrapper mockSys(10);
    PollingDriver driver(mockSys, TEST_PIN);
    driver.begin();
    TEST_ASSERT_EQUAL(0, driver.getEventCount());
    TEST_ASSERT_EQUAL(0, driver.getLastEventTime());
}

void test_polling_driver_no_change() {
    MockSystemWrapper mockSys(10);
    mockSys.pinStates[TEST_PIN] = 1;
    PollingDriver driver(mockSys, TEST_PIN);
    driver.begin();

    driver.update();
    TEST_ASSERT_EQUAL(0, driver.getEventCount());
}

void test_polling_driver_single_falling_edge() {
    MockSystemWrapper mockSys(10);
    mockSys.pinStates[TEST_PIN] = 1; // Start high
    PollingDriver driver(mockSys, TEST_PIN);
    driver.begin();

    mockSys.ms = 100;
    mockSys.pinStates[TEST_PIN] = 0; // Go low
    driver.update();

    TEST_ASSERT_EQUAL(1, driver.getEventCount());
    TEST_ASSERT_EQUAL(100, driver.getLastEventTime());
}

void test_polling_driver_multiple_events() {
    MockSystemWrapper mockSys(10);
    mockSys.pinStates[TEST_PIN] = 1; // Start high
    PollingDriver driver(mockSys, TEST_PIN);
    driver.begin();

    mockSys.ms = 100;
    mockSys.pinStates[TEST_PIN] = 0; // Event 1 (Falling)
    driver.update();

    mockSys.ms = 150;
    mockSys.pinStates[TEST_PIN] = 1; // Event 2 (Rising)
    driver.update();

    mockSys.ms = 200;
    mockSys.pinStates[TEST_PIN] = 0; // Event 3 (Falling)
    driver.update();

    TEST_ASSERT_EQUAL(3, driver.getEventCount());
    TEST_ASSERT_EQUAL(200, driver.getLastEventTime());
}

void test_polling_driver_counts_rising_edge() {
    MockSystemWrapper mockSys(10);
    mockSys.pinStates[TEST_PIN] = 0; // Start low
    PollingDriver driver(mockSys, TEST_PIN);
    driver.begin();

    mockSys.ms = 100;
    mockSys.pinStates[TEST_PIN] = 1; // Go high
    driver.update();

    TEST_ASSERT_EQUAL(1, driver.getEventCount());
    TEST_ASSERT_EQUAL(100, driver.getLastEventTime());
}

void setUp(void) {}
void tearDown(void) {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_polling_driver_initialization);
    RUN_TEST(test_polling_driver_no_change);
    RUN_TEST(test_polling_driver_single_falling_edge);
    RUN_TEST(test_polling_driver_multiple_events);
    RUN_TEST(test_polling_driver_counts_rising_edge);
    UNITY_END();
    return 0;
}
