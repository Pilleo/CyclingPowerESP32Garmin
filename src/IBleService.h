#ifndef IBLESERVICE_H
#define IBLESERVICE_H

#include <cstdint>
#include "CyclingTelemetry.h"

class IBleService {
public:
    virtual ~IBleService() = default;

    virtual void start() = 0;

    virtual void updateData(const CyclingTelemetry& telemetry) = 0;

    virtual auto isConnected() -> bool = 0;
};

#endif //IBLESERVICE_H
