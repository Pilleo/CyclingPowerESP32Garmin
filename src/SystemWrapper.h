#ifndef SYSTEMWRAPPER_H
#define SYSTEMWRAPPER_H

#include <Arduino.h>

class ISystemWrapper {
public:
    virtual unsigned long millis() = 0;
    virtual int digitalRead(uint8_t pin) = 0;
};

class ArduinoSystemWrapper : public ISystemWrapper {
public:
    unsigned long millis() override {
        return ::millis();
    }

    int digitalRead(uint8_t pin) override {
        return ::digitalRead(pin);
    }
};

#endif //SYSTEMWRAPPER_H
