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

auto Cadence::calculateCoastingRpm(const uint32_t intervalTime) -> float 
{
    // Fix: Check largest multipliers first to avoid shadowing
    // e.g., if > 6x, it is also > 2x. Checking 2x first hides the 6x case.
    
    if (intervalTime > lastIntervalTime * 6)
    {
        return calculateRpmFromRevolutions(1, intervalTime * 6);
    }
    if (intervalTime > lastIntervalTime * 5)
    {
        return calculateRpmFromRevolutions(1, intervalTime * 5);
    }
    if (intervalTime > lastIntervalTime * 4)
    {
        return calculateRpmFromRevolutions(1, intervalTime * 4);
    }
    if (intervalTime > lastIntervalTime * 3)
    {
        return calculateRpmFromRevolutions(1, intervalTime * 3);
    }
    if (intervalTime > lastIntervalTime * 2)
    {
        return calculateRpmFromRevolutions(1, intervalTime * 2);
    }
    
    // Default decay
    return calculateRpmFromRevolutions(1, intervalTime);
}

auto Cadence::cadence() -> float
{
    const uint32_t mls = sys.millis();
    
    // Unsigned arithmetic handles overflow correctly if types match
    const uint32_t sampleTime = mls - elapsedSampleTimeStamp;

    if (sampleTime > MIN_DELAY_BETWEEN_FULL_ROTATION_MS)
    {
        bool const currentState = sys.digitalRead(pin) != 0;

        // Falling Edge Detection (High -> Low)
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
            // Timeout logic
            if (intervalTime > MAX_IDLE_TIMEOUT_MS)
            {
                rpm = 0.0F;
                if (intervalTime > DEEP_SLEEP_TIMEOUT_MS)
                {
                    rpm = -1.0F;
                }
                return rpm;
            }

            // Coasting / Forecasting logic
            if (rpm > 0 && lastIntervalTime > 0 && intervalTime > lastIntervalTime) 
            {
                rpm = calculateCoastingRpm(intervalTime);
            }
        }
        else
        {
            // Valid revolution(s) detected
            rpm = calculateRpmFromRevolutions(rev, intervalTime);
            rev = 0;
            elapsedTimestamp = mls;
            lastIntervalTime = intervalTime;
            
            // GATT Timestamp: 1/1024th of a second
            // Modulo 65536 is automatic for uint16_t, but explicit is fine
            gattLastCrankRevolutionTimestamp = (gattLastCrankRevolutionTimestamp +
                                                static_cast<uint16_t>((intervalTime / 1000.F) * 1024.F));
        }
    }

    return rpm;
}
