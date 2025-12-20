#ifndef SYSTEMWRAPPER_H
#define SYSTEMWRAPPER_H

#include <cstdint>

#ifndef NATIVE_TEST
#include <Arduino.h>
// #include <esp_intr_alloc.h> // Not directly needed if using Arduino's noInterrupts()
#endif

#define RISING 0x01
#define FALLING 0x02
#define CHANGE 0x03

class ISystemWrapper {
public:
    using isr_t = void (*)();

    virtual auto millis() -> uint32_t = 0;
    virtual auto digitalRead(uint8_t pin) -> int = 0;
    virtual void digitalWrite(uint8_t pin, int val) = 0;
    virtual void enterDeepSleep(uint8_t wakeupPin, int wakeupLevel) = 0;
    virtual void attachInterrupt(uint8_t pin, isr_t isr, int mode) = 0;

    // RENAMED: Critical Section Management
    virtual void disableInterrupts() = 0;
    virtual void enableInterrupts() = 0;
};

#ifndef NATIVE_TEST
class ArduinoSystemWrapper : public ISystemWrapper {
public:
    auto millis() -> uint32_t override { return static_cast<uint32_t>(::millis()); }
    auto digitalRead(const uint8_t pin) -> int override { return ::digitalRead(pin); }
    void digitalWrite(uint8_t pin, int val) override { ::digitalWrite(pin, val); }
    void enterDeepSleep(uint8_t wakeupPin, int wakeupLevel) override {
        esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(wakeupPin), wakeupLevel);
        esp_deep_sleep_start();
    }
    void attachInterrupt(uint8_t pin, isr_t isr, int mode) override {
        ::attachInterrupt(digitalPinToInterrupt(pin), isr, mode);
    }

    // NEW implementations - now call the Arduino macros
    void disableInterrupts() override { noInterrupts(); } // Calls Arduino's macro
    void enableInterrupts() override { interrupts(); }     // Calls Arduino's macro
};
#endif

#endif //SYSTEMWRAPPER_H