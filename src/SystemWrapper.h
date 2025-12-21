#ifndef SYSTEMWRAPPER_H
#define SYSTEMWRAPPER_H

#include <cstdint> // Required for uint8_t, uint32_t regardless of platform

#ifndef NATIVE_TEST
#include <Arduino.h>
// #include <esp_intr_alloc.h> // Not directly needed if using Arduino's noInterrupts()
#else
// Define Arduino constants for Native Tests
#ifndef INPUT
#define INPUT 0x0
#endif
#ifndef OUTPUT
#define OUTPUT 0x1
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 0x2
#endif
#ifndef LOW
#define LOW 0x0
#endif
#ifndef HIGH
#define HIGH 0x1
#endif
#endif

#define RISING 0x01
#define FALLING 0x02
#define CHANGE 0x03

class ISystemWrapper {
public:
    ISystemWrapper() = default;
    virtual ~ISystemWrapper() = default;
    ISystemWrapper(const ISystemWrapper&) = delete;
    auto operator=(const ISystemWrapper&) -> ISystemWrapper& = delete;
    ISystemWrapper(ISystemWrapper&&) = delete;
    auto operator=(ISystemWrapper&&) -> ISystemWrapper& = delete;

    using isr_t = void (*)();

    virtual auto millis() -> uint32_t = 0;
    virtual auto digitalRead(uint8_t pin) -> int = 0;
    virtual void digitalWrite(uint8_t pin, int val) = 0;
    virtual void pinMode(uint8_t pin, uint8_t mode) = 0; // Added pinMode
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
    void digitalWrite(const uint8_t pin, const int val) override { ::digitalWrite(pin, val); }
    void pinMode(const uint8_t pin, const uint8_t mode) override { ::pinMode(pin, mode); } // Added pinMode implementation
    void enterDeepSleep(uint8_t wakeupPin, const int wakeupLevel) override {
        esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(wakeupPin), wakeupLevel);
        esp_deep_sleep_start();
    }
    void attachInterrupt(const uint8_t pin, const isr_t isr, const int mode) override {
        ::attachInterrupt(digitalPinToInterrupt(pin), isr, mode);
    }

    // NEW implementations - now call the Arduino macros
    void disableInterrupts() override { noInterrupts(); } // Calls Arduino's macro
    void enableInterrupts() override { interrupts(); }     // Calls Arduino's macro
};
#endif

#endif //SYSTEMWRAPPER_H