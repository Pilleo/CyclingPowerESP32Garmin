#ifndef CYCLING_TELEMETRY_H
#define CYCLING_TELEMETRY_H

#include <cstdint>

struct CyclingTelemetry {
  uint16_t powerWatts;
  uint32_t crankRevolutions;
  uint16_t crankEventTime1024;
};

#endif // CYCLING_TELEMETRY_H
