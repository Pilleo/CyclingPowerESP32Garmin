#ifndef CADENCE_H
#define CADENCE_H

#include <cstdint>
#include "SystemWrapper.h"

class Cadence
{
public:
    explicit Cadence(uint8_t pinn, ISystemWrapper& sys);

    float cadence();

    uint32_t lastTimestamp();

    uint32_t totalRevs();

    uint16_t getGattLastCrankRevolutionTimestamp();

private:
    ISystemWrapper& sys;
    const uint8_t pin;
    float rpm = 0;
    int16_t debouncingCounter = 0;
    uint32_t lastIntervalTime = 0;
    bool oldState;
    unsigned long elapsedTimestamp = 0;
    unsigned long elapsedSampleTimeStamp = 0;
    uint8_t rev = 0;
    uint32_t totalRev = 0;
    uint16_t gattLastCrankRevolutionTimestamp = 0;
};

#endif //CADENCE_H
