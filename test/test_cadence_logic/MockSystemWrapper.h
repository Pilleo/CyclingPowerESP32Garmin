#ifndef MOCK_SYSTEM_WRAPPER_H
#define MOCK_SYSTEM_WRAPPER_H

#include "SystemWrapper.h"

class MockSystemWrapper : public ISystemWrapper {
public:
    MockSystemWrapper() : currentTime(0), pinState(true) {}

    void setMillis(unsigned long newTime) {
        currentTime = newTime;
    }

    void setPinState(bool state) {
        pinState = state;
    }

    // ISystemWrapper interface
    unsigned long millis() override {
        return currentTime;
    }

    int digitalRead(uint8_t pin) override {
        return pinState;
    }

private: // Declare member variables here
    unsigned long currentTime;
    bool pinState;
};

#endif // MOCK_SYSTEM_WRAPPER_H
