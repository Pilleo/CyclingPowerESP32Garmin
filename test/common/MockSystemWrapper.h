#ifndef MOCK_SYSTEM_WRAPPER_H
#define MOCK_SYSTEM_WRAPPER_H

#include "../../src/SystemWrapper.h"
#include <map>

class MockSystemWrapper : public ISystemWrapper {
public:
    MockSystemWrapper() : currentTime(0), wasSleepCalled(false), sleepWakeupPin(0), sleepWakeupLevel(0) {}

    void setMillis(unsigned long newTime) {
        currentTime = newTime;
    }

    void advanceTime(unsigned long delta) {
        currentTime += delta;
    }

    void setPinState(uint8_t pin, bool state) {
        if (_interrupts.count(pin)) { // Existing ISR simulation logic
            auto& handler = _interrupts[pin];
            isr_t isr = handler.first;
            int mode = handler.second;
            bool oldState = pinStates.count(pin) ? pinStates[pin] : false; // Default to false if not set
            pinStates[pin] = state;

            bool trigger = false;
            if (mode == CHANGE && state != oldState) trigger = true;
            if (mode == RISING && !oldState && state) trigger = true;
            if (mode == FALLING && oldState && !state) trigger = true;

            if (trigger && isr) {
                isr(); // Trigger the ISR
            }
        } else {
            pinStates[pin] = state;
        }
    }

    // ISystemWrapper interface
    uint32_t millis() override {
        return currentTime;
    }

    int digitalRead(uint8_t pin) override {
        if (pinStates.find(pin) != pinStates.end()) {
            return pinStates[pin];
        }
        return 1; // Default to HIGH (pull-up resistor)
    }

    void digitalWrite(uint8_t pin, int val) override {
        // No-op for now, or store if needed for verification
    }

    void pinMode(uint8_t pin, uint8_t mode) override {
        // No-op for mock
    }

    void enterDeepSleep(uint8_t pin, int level) override {
        wasSleepCalled = true;
        sleepWakeupPin = pin;
        sleepWakeupLevel = level;
    }

    // Existing interrupt handler storage for testing
    std::map<uint8_t, std::pair<isr_t, int>> _interrupts; // Made public for easier test setup in future
    void attachInterrupt(uint8_t pin, isr_t isr, int mode) override {
        _interrupts[pin] = {isr, mode};
    }

    // RENAMED: No-op for tests
    void disableInterrupts() override {}
    void enableInterrupts() override {}

    void reset() {
        currentTime = 0;
        pinStates.clear();
        wasSleepCalled = false;
        sleepWakeupPin = 0;
        sleepWakeupLevel = 0;
        _interrupts.clear(); // Clear registered ISRs on reset
    }

    bool wasSleepCalled;
    uint8_t sleepWakeupPin;
    int sleepWakeupLevel;

private:
    unsigned long currentTime;
    std::map<uint8_t, bool> pinStates;
};

#endif // MOCK_SYSTEM_WRAPPER_H