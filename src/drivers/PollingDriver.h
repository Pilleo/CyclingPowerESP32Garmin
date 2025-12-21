#ifndef POLLINGDRIVER_H
#define POLLINGDRIVER_H

#include "ISensorDriver.h"
#include "../SystemWrapper.h"

class PollingDriver : public ISensorDriver {
public:
    PollingDriver(ISystemWrapper& sys, uint8_t pin);

    void begin() override;
    void update() override;
    uint32_t getEventCount() const override;
    uint32_t getLastEventTime() const override;

private:
    ISystemWrapper& _sys;
    uint8_t _pin;
    bool _lastState;
    uint32_t _eventCount;
    uint32_t _lastEventTime;
};

#endif // POLLINGDRIVER_H
