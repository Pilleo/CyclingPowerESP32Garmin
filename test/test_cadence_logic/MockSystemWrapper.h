#ifndef MOCK_SYSTEM_WRAPPER_H
#define MOCK_SYSTEM_WRAPPER_H

#include "../../src/SystemWrapper.h"

class MockSystemWrapper : public ISystemWrapper {
public:
    MockSystemWrapper() : currentTime(0), pinState(true), wasSleepCalled(false), sleepWakeupPin(0), sleepWakeupLevel(0) {}

    void setMillis(unsigned long newTime) {
        currentTime = newTime;
    }

    void setPinState(bool state) {
        pinState = state;
    }

    // ISystemWrapper interface
    uint32_t millis() override {
        return currentTime;
    }

    int digitalRead(uint8_t pin) override {
        return pinState;
    }

    void digitalWrite(uint8_t pin, int val) override {
        // No-op for now, or store if needed for verification
    }

    void enterDeepSleep(uint8_t wakeupPin, int wakeupLevel) override {
        wasSleepCalled = true;
        sleepWakeupPin = wakeupPin;
        sleepWakeupLevel = wakeupLevel;
    }

    void attachInterrupt(uint8_t pin, isr_t isr, int mode) override {
        // No-op for now, can be expanded to store ISRs for testing
    }

    bool wasSleepCalled;
    uint8_t sleepWakeupPin;
    int sleepWakeupLevel;

private: // Declare member variables here
    unsigned long currentTime;
    bool pinState;
};

#endif // MOCK_SYSTEM_WRAPPER_H
