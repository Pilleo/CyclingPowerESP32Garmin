#ifndef BLE_PACKET_GENERATOR_H
#define BLE_PACKET_GENERATOR_H

#include <cstdint>
#include <cstring> // For memcpy

// Define the flags based on ble_constants.h
// (We redefine or include them here to keep this file standalone-testable)
#define CPM_FLAG_WHEEL_REV_DATA_PRESENT (1 << 4)
#define CPM_FLAG_CRANK_REV_DATA_PRESENT (1 << 5)

struct CPSMeasurement_t {
    uint16_t flags;
    int16_t instantaneousPower;
    // Optional fields exist, but we must pack them dynamically based on flags
    // to strictly adhere to BLE specs (which are not simple structs).
    // However, for this specific project, the structure seems fixed:
    uint32_t cumulativeWheelRevs;
    uint16_t lastWheelEventTime; // 1/2048 s
    uint16_t cumulativeCrankRevs;
    uint16_t lastCrankEventTime;// 1/1024 s
} __attribute__((packed));

class BlePacketGenerator {
public:
    /**
     * @brief Packs power and cadence data into the BLE byte structure.
     * 
     * @param powerWatts Instantaneous Power
     * @param totalCrankRevs Accumulated crank revolutions
     * @param lastCrankTime Timestamp of last crank event (1/1024s)
     * @param buffer Output buffer (must be at least 14 bytes)
     * @return size_t Number of bytes written
     */
    static auto generatePacket(uint16_t powerWatts,
                                 uint32_t totalCrankRevs, 
                                 uint16_t lastCrankTime, 
                                 uint8_t* buffer) -> size_t
    {
        // 1. Prepare the Fixed Structure
        // Note: The original code combined Wheel and Crank logic.
        // Assuming we keep the existing logic:
        // Wheel Revs = Crank Revs * 31
        // Wheel Time = Crank Time * 2 (?)
        
        CPSMeasurement_t packet{};
        packet.flags = CPM_FLAG_WHEEL_REV_DATA_PRESENT | CPM_FLAG_CRANK_REV_DATA_PRESENT;
        packet.instantaneousPower = powerWatts;
        
        packet.cumulativeWheelRevs = totalCrankRevs * 31; 
        packet.lastWheelEventTime = (lastCrankTime * 2); // % 65536 is automatic for uint16
        
        packet.cumulativeCrankRevs = totalCrankRevs;
        packet.lastCrankEventTime = lastCrankTime;

        size_t size = sizeof(CPSMeasurement_t);
        // 2. Copy to buffer
        // This ensures Endianness is handled by the compiler (ESP32 is Little Endian, BLE is Little Endian)
        memcpy(buffer, &packet, size);

        return size;
    }
};

#endif