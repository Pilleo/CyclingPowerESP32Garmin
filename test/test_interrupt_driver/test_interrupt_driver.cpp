#include <unity.h>
#include "drivers/InterruptDriver.h"
#include "SystemWrapper.h"
#include <vector>

class MockSystemWrapper : public ISystemWrapper {
public:
    uint32_t ms = 0;
    void (*_isr)() = nullptr;
    bool interrupts_enabled = true;
    void pinMode(uint8_t pin, uint8_t mode) override {}
    int digitalRead(uint8_t pin) override { return 0; }
    void digitalWrite(uint8_t pin, int val) override {}
    uint32_t millis() override { return ms; }
    void attachInterrupt(uint8_t pin, void (*isr)(), int mode) override { _isr = isr; }
    void enableInterrupts() override { interrupts_enabled = true; }
    void disableInterrupts() override { interrupts_enabled = false; }
    void enterDeepSleep(uint8_t wakeupPin, int mode) override {}

    void triggerInterrupt() {
        if (_isr && interrupts_enabled) {
            _isr();
        }
    }
};

const uint8_t TEST_PIN = 3;

void test_interrupt_driver_initialization() {
    MockSystemWrapper mockSys;
    InterruptDriver driver(mockSys, TEST_PIN);
    driver.begin();
    TEST_ASSERT_EQUAL(0, driver.getEventCount());
    TEST_ASSERT_EQUAL(0, driver.getLastEventTime());
    TEST_ASSERT_NOT_NULL(mockSys._isr);
}

void test_interrupt_driver_single_event() {
    MockSystemWrapper mockSys;
    InterruptDriver driver(mockSys, TEST_PIN);
    driver.begin();

    mockSys.ms = 100;
    mockSys.triggerInterrupt();

    TEST_ASSERT_EQUAL(1, driver.getEventCount());
    TEST_ASSERT_EQUAL(100, driver.getLastEventTime());
}

void test_interrupt_driver_debounce() {
    MockSystemWrapper mockSys;
    InterruptDriver driver(mockSys, TEST_PIN);
    driver.begin();

    mockSys.ms = 100;
    mockSys.triggerInterrupt();

    mockSys.ms = 110; // Should be ignored (debounce is 50ms)
    mockSys.triggerInterrupt();

    mockSys.ms = 160; // Should be counted
    mockSys.triggerInterrupt();

    TEST_ASSERT_EQUAL(2, driver.getEventCount());
    TEST_ASSERT_EQUAL(160, driver.getLastEventTime());
}

void test_interrupt_driver_get_count_is_atomic() {
    MockSystemWrapper mockSys;
    InterruptDriver driver(mockSys, TEST_PIN);
    driver.begin();

    mockSys.ms = 100;
    mockSys.triggerInterrupt();

    // Simulate reading the count while interrupts are disabled
    driver.getEventCount();
    TEST_ASSERT_TRUE(mockSys.interrupts_enabled); // Should be re-enabled

    mockSys.disableInterrupts();
    driver.getEventCount();
    TEST_ASSERT_TRUE(mockSys.interrupts_enabled);
}

void setUp(void) {}
void tearDown(void) {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_interrupt_driver_initialization);
    RUN_TEST(test_interrupt_driver_single_event);
    RUN_TEST(test_interrupt_driver_debounce);
    RUN_TEST(test_interrupt_driver_get_count_is_atomic);
    UNITY_END();
    return 0;
}
