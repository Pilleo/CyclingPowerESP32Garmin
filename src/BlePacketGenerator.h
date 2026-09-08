#ifndef BLE_PACKET_GENERATOR_H
#define BLE_PACKET_GENERATOR_H

#include <cstdint>
#include <cstring> // For memcpy
#include "ble_constants.h"
#include "config.h"

// Define the flags based on ble_constants.h
// These flags indicate which optional fields are present in the packet.
#define CPM_FLAG_WHEEL_REV_DATA_PRESENT CPM_WHEEL_REV_DATA_PRESENT
#define CPM_FLAG_CRANK_REV_DATA_PRESENT CPM_CRANK_REV_DATA_PRESENT

// This struct represents the Cycling Power Measurement characteristic.
// It is packed to ensure there is no padding between members, matching the BLE specification.
struct CyclingPowerMeasurement {
    uint16_t flags;
    int16_t instantaneousPower;
    // The following fields are optional and their presence is indicated by the flags.
    uint32_t cumulativeWheelRevs;
    uint16_t lastWheelEventTime; // Unit is 1/2048 second
    uint16_t cumulativeCrankRevs;
    uint16_t lastCrankEventTime; // Unit is 1/1024 second
} __attribute__((packed));

class BlePacketGenerator {
public:
    // The size of the buffer required to hold the generated packet.
    static constexpr size_t MAX_PACKET_SIZE = sizeof(CyclingPowerMeasurement);
    // This factor is used to simulate wheel revolutions from crank revolutions.
    // A ratio of 3 approximates a standard gear ratio (e.g. 50/17).
    // At 30 RPM, this gives ~11.3 km/h on a standard wheel.
    static constexpr uint8_t WHEEL_TO_CRANK_REVOLUTION_RATIO =
        WHEEL_REVOLUTIONS_PER_CRANK_REVOLUTION;
    // This factor is used to convert crank event time to wheel event time, which has a different resolution.

    /**
     * @brief Packs power and cadence data into a BLE packet.
     * @param powerWatts The instantaneous power in watts.
     * @param totalCrankRevs The total number of crank revolutions.
     * @param lastCrankTime The time of the last crank event in 1/1024s resolution.
     * @param buffer The output buffer for the generated packet. Must be at least MAX_PACKET_SIZE bytes.
     * @return The number of bytes written to the buffer.
     */
    static size_t generatePacket(const uint16_t powerWatts,
                                 const uint32_t totalCrankRevs,
                                 const uint16_t lastCrankTime,
                                 uint8_t *buffer) {
        CyclingPowerMeasurement packet{};
        packet.flags = CPM_FLAG_WHEEL_REV_DATA_PRESENT | CPM_FLAG_CRANK_REV_DATA_PRESENT;
        packet.instantaneousPower = powerWatts;

        // Simulate wheel data from crank data, as some applications require it.
        packet.cumulativeWheelRevs = totalCrankRevs * WHEEL_TO_CRANK_REVOLUTION_RATIO;
        packet.lastWheelEventTime =
            static_cast<uint16_t>(lastCrankTime * 2U);

        packet.cumulativeCrankRevs = static_cast<uint16_t>(totalCrankRevs);
        packet.lastCrankEventTime = lastCrankTime;

        // Copy the packet to the buffer. This is safe because the struct is packed and the buffer is large enough.
        // The ESP32 is little-endian, which matches the BLE specification, so no byte swapping is needed.
        memcpy(buffer, &packet, MAX_PACKET_SIZE);

        return MAX_PACKET_SIZE;
    }
};

#endif // BLE_PACKET_GENERATOR_H
