#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <cstdint>
#include "SystemWrapper.h"

class StatusLed {
    ISystemWrapper &_sys;
    uint8_t _pin;
    uint32_t _lastBlinkTime = 0;

    static constexpr int DEFAULT_BLINK_INTERVAL = 1000;
    static constexpr int MILLISECONDS_IN_MINUTE = 60000;

public:
    StatusLed(ISystemWrapper &sys, const uint8_t pin) noexcept : _sys(sys), _pin(pin) {
    }

    void setup() {
        _sys.pinMode(_pin, OUTPUT);
        _sys.digitalWrite(_pin, 0); // Initialize LOW
    }

    void update(const float cadence) {
        int blinkInterval = DEFAULT_BLINK_INTERVAL;
        if (cadence > 0) {
            blinkInterval = static_cast<int>(MILLISECONDS_IN_MINUTE / cadence);
        }

        const auto now = _sys.millis();
        if (now - _lastBlinkTime >= blinkInterval) {
            _sys.digitalWrite(_pin, 0); // LOW (Off)
            _lastBlinkTime = now;
        } else if (now - _lastBlinkTime >= 1) {
            _sys.digitalWrite(_pin, 1); // HIGH (On)
        }
    }
};

#endif // STATUS_LED_H
