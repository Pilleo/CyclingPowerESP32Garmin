#ifndef POWER_H
#define POWER_H

#include <cstdint>

struct PowerCalculationInput {
    uint8_t resistanceLevel;
    float cadence;
};

class Power {
public:
    /**
     * @brief Calculates power based on resistance level and cadence using a lookup table.
     *
     * @param input Struct containing resistance level (1-16) and cadence in RPM
     * @return uint16_t Power in Watts
     */
    static auto calculate(PowerCalculationInput input) -> uint16_t;
};

#endif // POWER_H
