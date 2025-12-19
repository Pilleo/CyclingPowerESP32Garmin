#ifndef MOCK_SYSTEM_WRAPPER_H
#define MOCK_SYSTEM_WRAPPER_H

#include "SystemWrapper.h" // Use the header from src
#include <map>
#include <cstdint>

class MockSystemWrapper : public ISystemWrapper {
public:
    MockSystemWrapper() : currentTime(0), wasSleepCalled(false), sleepWakeupPin(0), sleepWakeupLevel(0) {}

    void setMillis(unsigned long newTime) {
        currentTime = newTime;
    }

    void setPinState(uint8_t pin, bool state) {
        pinStates[pin] = state;
    }

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

    void enterDeepSleep(uint8_t wakeupPin, int wakeupLevel) override {
        wasSleepCalled = true;
        sleepWakeupPin = wakeupPin;
        sleepWakeupLevel = wakeupLevel;
    }

    bool wasSleepCalled;
    uint8_t sleepWakeupPin;
    int sleepWakeupLevel;

private:
    unsigned long currentTime;
    std::map<uint8_t, bool> pinStates;
};

#endif // MOCK_SYSTEM_WRAPPER_H
