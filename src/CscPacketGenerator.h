#ifndef CSC_PACKET_GENERATOR_H
#define CSC_PACKET_GENERATOR_H

#include "ble_constants.h"
#include "config.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

struct CscMeasurement {
  uint8_t flags;
  uint32_t cumulativeWheelRevs;
  uint16_t lastWheelEventTime;
  uint16_t cumulativeCrankRevs;
  uint16_t lastCrankEventTime;
} __attribute__((packed));

class CscPacketGenerator {
public:
  static constexpr size_t MAX_PACKET_SIZE = sizeof(CscMeasurement);

  static size_t generatePacket(uint32_t totalCrankRevs,
                               uint16_t lastCrankTime, uint8_t *buffer) {
    CscMeasurement packet{};
    packet.flags =
        CSCM_WHEEL_REV_DATA_PRESENT | CSCM_CRANK_REV_DATA_PRESENT;
    packet.cumulativeWheelRevs =
        totalCrankRevs * WHEEL_REVOLUTIONS_PER_CRANK_REVOLUTION;
    packet.lastWheelEventTime = lastCrankTime;
    packet.cumulativeCrankRevs = static_cast<uint16_t>(totalCrankRevs);
    packet.lastCrankEventTime = lastCrankTime;

    std::memcpy(buffer, &packet, MAX_PACKET_SIZE);
    return MAX_PACKET_SIZE;
  }
};

#endif // CSC_PACKET_GENERATOR_H
