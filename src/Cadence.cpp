#include "Cadence.h"

auto Cadence::getGattLastCrankRevolutionTimestamp() const -> uint16_t
{
    return (gattLastCrankRevolutionTimestamp);
}

auto Cadence::totalRevs() const -> uint32_t
{
    return totalRev;
}

auto Cadence::lastTimestamp() const -> uint32_t
{
    return elapsedTimestamp;
}

Cadence::Cadence(const uint8_t pinn, ISystemWrapper& sys) : sys(sys), pin(pinn)
{
    // Initialize state to avoid startup glitches
    oldState = (sys.digitalRead(pin) != 0);
    
    elapsedTimestamp = 0;
    elapsedSampleTimeStamp = 0;
    rev = 0;
    totalRev = 0;
    gattLastCrankRevolutionTimestamp = 0;
}

static auto calculateRpmFromRevolutions(const uint8_t revolutions, const uint32_t revolutionsTime) -> float
{
    if (revolutionsTime == 0) {
        return 0.0F;
    }
    // 60,000 ms in a minute
    const float instantaneousRpm = 60000.0F / (static_cast<float>(revolutionsTime) / static_cast<float>(revolutions));
    return instantaneousRpm;
}


auto Cadence::cadence() -> float
{
    const uint32_t mls = sys.millis();
    const uint32_t sampleTime = mls - elapsedSampleTimeStamp;

    if (sampleTime > MIN_DELAY_BETWEEN_FULL_ROTATION_MS)
    {
        bool const currentState = sys.digitalRead(pin) != 0;
        if (oldState == true && currentState == false)
        {
            rev++;
            totalRev++;
            elapsedSampleTimeStamp = mls;
        }
        oldState = currentState;
    }

    const uint32_t intervalTime = mls - elapsedTimestamp;

    if (intervalTime > MIN_DELAY_BETWEEN_FULL_ROTATION_MS)
    {
        if (rev == 0)
        {
            // Timeout: 5 seconds with no pedaling = 0 RPM
            if (intervalTime > MAX_IDLE_TIMEOUT_MS)
            {
                rpm = 0.0F;
                if (intervalTime > DEEP_SLEEP_TIMEOUT_MS)
                {
                    rpm = -1.0F;
                }
                return rpm;
            }

            // Coasting Logic (Natural Decay)
            // If we have valid history, and the current gap is longer than the last,
            // calculate the "Virtual RPM" based on this growing gap.
            if (rpm > 0 && lastIntervalTime > 0 && intervalTime > lastIntervalTime)
            {
                // Simple Natural Decay: RPM = 60000 / CurrentWaitingDuration
                rpm = calculateRpmFromRevolutions(1, intervalTime);
            }
        }
        else
        {
            // Valid revolution detected
            rpm = calculateRpmFromRevolutions(rev, intervalTime);
            rev = 0;
            elapsedTimestamp = mls;
            lastIntervalTime = intervalTime;
            gattLastCrankRevolutionTimestamp = (gattLastCrankRevolutionTimestamp +
                                                static_cast<uint16_t>((intervalTime / 1000.F) * 1024.F));
        }
    }

    return rpm;
}
