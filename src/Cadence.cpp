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

void Cadence::onPulse(uint32_t now)
{
    // Debounce Logic: Only count if enough time passed since last registered pulse
    if (now - elapsedSampleTimeStamp > MIN_DELAY_BETWEEN_FULL_ROTATION_MS)
    {
        rev++;
        totalRev++;
        elapsedSampleTimeStamp = now;
    }
}

auto Cadence::cadence() -> float
{
    const uint32_t mls = sys.millis();

    // 1. Hardware Detection (Polling Driver)
    // We continuously track the pin state to detect the specific Falling Edge event.
    bool const currentState = sys.digitalRead(pin) != 0;

    if (oldState && !currentState)
    {
        // Falling Edge Detected (High -> Low)
        onPulse(mls);
    }
    oldState = currentState;

    // 2. Business Logic (Calculation)
    const uint32_t intervalTime = mls - elapsedTimestamp;

    if (intervalTime > MIN_DELAY_BETWEEN_FULL_ROTATION_MS)
    {
        // Snapshot volatile variable to local scope (Prep for concurrency)
        uint8_t currentRevs = rev;

        if (currentRevs == 0)
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
            if (rpm > 0 && lastIntervalTime > 0 && intervalTime > lastIntervalTime)
            {
                rpm = calculateRpmFromRevolutions(1, intervalTime);
            }
        }
        else
        {
            // Valid revolution detected
            rpm = calculateRpmFromRevolutions(currentRevs, intervalTime);

            // Reset batch (Note: In pure interrupt mode, this needs atomic decrement)
            rev = 0;

            elapsedTimestamp = mls;
            lastIntervalTime = intervalTime;
            gattLastCrankRevolutionTimestamp = (gattLastCrankRevolutionTimestamp +
                                                static_cast<uint16_t>((intervalTime / 1000.F) * 1024.F));
        }
    }

    return rpm;
}