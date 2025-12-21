#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <cstdint>
#include "SystemWrapper.h"

class StatusLed {
    ISystemWrapper& _sys;
    uint8_t _pin;
    uint32_t _lastBlinkTime = 0;

    static constexpr int DEFAULT_BLINK_INTERVAL = 1000;
    static constexpr int MILLISECONDS_IN_MINUTE = 60000;

public:
    StatusLed(ISystemWrapper& sys, uint8_t pin) noexcept : _sys(sys), _pin(pin) {}

    void setup() {
        _sys.pinMode(_pin, OUTPUT);
        _sys.digitalWrite(_pin, 0); // Initialize LOW
    }

    void update(float cadence) {
        int blinkInterval = DEFAULT_BLINK_INTERVAL;
        if (cadence > 0) {
            blinkInterval = static_cast<int>(MILLISECONDS_IN_MINUTE / cadence);
        }

        long const now = _sys.millis();

        if (now - _lastBlinkTime >= blinkInterval) {
            _sys.digitalWrite(_pin, 0); // LOW (Off)
            _lastBlinkTime = now;
        } else if (now - _lastBlinkTime >= 1) { // Short pulse logic from original code?
            // Original logic:
            // if (start - lastBlink >= timeFrameForBlink) { LOW; last = start; }
            // else if (start - lastBlink >= 1) { HIGH; }

            // This means it stays HIGH for almost the entire duration, and dips LOW for 1ms?
            // Or stays LOW for 1ms?

            // Let's re-read original logic:
            // if (start - lastBlink >= timeFrameForBlink) { digitalWrite(27, LOW); lastBlink = start; }
            // else if (start - lastBlink >= 1) { digitalWrite(27, HIGH); }

            // At t=0 (reset): lastBlink=0. start=0.
            // t=1: start-last >= 1 -> HIGH.
            // ...
            // t=interval: start-last >= interval -> LOW. lastBlink=interval.
            // t=interval+1: start-last >= 1 -> HIGH.

            // So it is HIGH most of the time, and blinks LOW for <1ms (or one loop cycle).
            // This is an "Active Low" blink or just a very short blip.
            // Assuming the LED is ON when HIGH.

            _sys.digitalWrite(_pin, 1); // HIGH (On)
        }
    }
};

#endif // STATUS_LED_H