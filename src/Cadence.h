#include <Arduino.h>

class Cadence
{
public:
    explicit Cadence(uint8_t pinn);

    int16_t cadence();

    static uint32_t lastTimestamp();

    static uint32_t totalRevs();

    static uint16_t getGattLastCrankRevolutionTimestamp();

private:
    const uint8_t pin;
    int16_t rpm = 0;
    int16_t debouncingCounter = 0;
    uint32_t lastIntervalTime = 0;
};
