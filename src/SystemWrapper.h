#ifndef SYSTEMWRAPPER_H
#define SYSTEMWRAPPER_H

#include <cstdint>

#ifndef NATIVE_TEST
#include <Arduino.h>
#endif

class ISystemWrapper {
public:
    virtual unsigned long millis() = 0;
    virtual int digitalRead(uint8_t pin) = 0;
};

#ifndef NATIVE_TEST
class ArduinoSystemWrapper : public ISystemWrapper {
public:
    unsigned long millis() override {
        return ::millis();
    }

    int digitalRead(uint8_t pin) override {
        return ::digitalRead(pin);
    }
};
#endif

#endif //SYSTEMWRAPPER_H
