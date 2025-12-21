#ifndef INTERRUPTDRIVER_H
#define INTERRUPTDRIVER_H

#include "ISensorDriver.h"
#include "../SystemWrapper.h"

#ifdef ESP32
#include "esp_attr.h"
#define ISR_ATTR IRAM_ATTR
#else
#define ISR_ATTR
#endif

class InterruptDriver : public ISensorDriver {
public:
    InterruptDriver(ISystemWrapper& sys, uint8_t pin);

    void begin() override;
    void update() override;
    uint32_t getEventCount() const override;
    uint32_t getLastEventTime() const override;

    void handleInterrupt();

private:
    ISystemWrapper& _sys;
    uint8_t _pin;
    volatile uint32_t _eventCount;
    volatile uint32_t _lastEventTime;
    volatile uint32_t _lastDebounceTime;

    static void isr();
    static InterruptDriver* _instance;

    static constexpr uint32_t DEBOUNCE_TIME_MS = 50;
};

#endif // INTERRUPTDRIVER_H
