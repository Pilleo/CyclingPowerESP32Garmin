#ifndef SYSTEMWRAPPER_H
#define SYSTEMWRAPPER_H

#include <cstdint> // For uint8_t

// No #include <Arduino.h> needed for native test environment

class ISystemWrapper {
public:
    virtual unsigned long millis() = 0;
    virtual int digitalRead(uint8_t pin) = 0;
};

// ArduinoSystemWrapper is not needed for native tests
// class ArduinoSystemWrapper : public ISystemWrapper {
// public:
//     unsigned long millis() override {
//         return ::millis();
//     }

//     int digitalRead(uint8_t pin) override {
//         return ::digitalRead(pin);
//     }
// };

#endif //SYSTEMWRAPPER_H
