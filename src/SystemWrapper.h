#ifndef SYSTEMWRAPPER_H
#define SYSTEMWRAPPER_H

#include <cstdint>

#ifndef NATIVE_TEST
#include <Arduino.h>
#endif

class ISystemWrapper {
public:

    virtual auto millis() -> unsigned long = 0;
    virtual auto digitalRead(uint8_t pin) -> int = 0;
};

#ifndef NATIVE_TEST
class ArduinoSystemWrapper : public ISystemWrapper {
public:
    auto millis() -> unsigned long override {
        return ::millis();
    }

    auto digitalRead(const uint8_t pin) -> int override {
        return ::digitalRead(pin);
    }
};
#endif

#endif //SYSTEMWRAPPER_H
