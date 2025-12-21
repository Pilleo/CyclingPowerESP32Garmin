#ifndef CADENCE_LOGIC_H
#define CADENCE_LOGIC_H

#include <cstdint>
#include "SystemWrapper.h"
#include "drivers/ISensorDriver.h"

class CadenceLogic {
public:
    explicit CadenceLogic(ISensorDriver& driver, ISystemWrapper& sys) noexcept;

    void begin();
    void update();

    auto cadence() -> float;
    bool shouldSleep() const;

    auto lastTimestamp() const -> uint32_t;
    auto getGattLastCrankRevolutionTimestamp() const -> uint16_t;
    auto totalRevs() const -> uint32_t;

private:
    void onPulse(uint32_t timestampMs);

    ISensorDriver& _driver;
    ISystemWrapper& _sys;

    float _rpm = 0;
    uint32_t _lastIntervalTime = 0;
    uint32_t _lastKnownEventCount = 0;

    // State variables, no longer volatile
    uint32_t _elapsedTimestamp = 0;
    uint32_t _elapsedSampleTimeStamp = 0;
    uint8_t _rev = 0;
    uint32_t _totalRev = 0;
    uint16_t _gattLastCrankRevolutionTimestamp = 0;

    static constexpr uint32_t MIN_DELAY_BETWEEN_FULL_ROTATION_MS = 330;
    static constexpr uint32_t MAX_IDLE_TIMEOUT_MS = 5000;
    static constexpr uint32_t DEEP_SLEEP_TIMEOUT_MS = 60000 * 15;
};

#endif // CADENCE_LOGIC_H
