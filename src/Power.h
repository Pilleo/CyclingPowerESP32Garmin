#ifndef POWER_H
#define POWER_H

#include <cstdint>

class Power {
public:
    /**
     * @brief Calculates power based on resistance level and cadence using a lookup table.
     *
     * @param resistanceLevel Current resistance level (1-16)
     * @param cadence Current cadence in RPM
     * @return uint16_t Power in Watts
     */
    static auto power(uint8_t resistanceLevel, float cadence) -> uint16_t;
};

#endif // POWER_H