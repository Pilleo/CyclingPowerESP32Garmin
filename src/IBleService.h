#ifndef IBLESERVICE_H
#define IBLESERVICE_H

#include <cstdint>
#include "Cadence.h"

class IBleService {
public:
    virtual void setup() = 0;
    virtual void update(uint16_t currentPower, const Cadence &cadence) = 0;
};

#endif //IBLESERVICE_H
