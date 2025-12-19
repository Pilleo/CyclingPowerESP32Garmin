#ifndef SYSTEMWRAPPER_H
#define SYSTEMWRAPPER_H

#include <cstdint>

#ifndef NATIVE_TEST
#include <Arduino.h>
#endif

class ISystemWrapper {
public:
    // Enforce 32-bit time to match ESP32 and native tests
    virtual auto millis() -> uint32_t = 0;
    virtual auto digitalRead(uint8_t pin) -> int = 0;
};

#ifndef NATIVE_TEST
class ArduinoSystemWrapper : public ISystemWrapper {
public:
    auto millis() -> uint32_t override {
        return static_cast<uint32_t>(::millis());
    }

    auto digitalRead(const uint8_t pin) -> int override {
        return ::digitalRead(pin);
    }
};
#endif

#endif //SYSTEMWRAPPER_H
