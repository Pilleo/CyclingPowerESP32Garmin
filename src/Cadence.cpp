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

Cadence::Cadence(const uint8_t pinn, ISystemWrapper& sys) : pin(pinn), sys(sys)
{
    // pinMode(pin, INPUT_PULLUP); // This should be handled by the system wrapper
    oldState = (sys.digitalRead(pin) != 0);
    elapsedTimestamp = 0;
    elapsedSampleTimeStamp = 0;
    rev = 0;
    totalRev = 0;
    gattLastCrankRevolutionTimestamp = 0;
}

static auto calculateRpmFromRevolutions(const uint8_t revolutions, const unsigned long revolutionsTime) -> float
{
    if (revolutionsTime == 0) {
        return 0.0F;
    }
    const float instantaneousRpm = 60000.0F / (static_cast<float>(revolutionsTime) / static_cast<float>(revolutions));
    return instantaneousRpm;
}

auto Cadence::cadence() -> float
{
    const unsigned long mls = sys.millis();
    const unsigned long sampleTime = mls - elapsedSampleTimeStamp;

    constexpr int minDelayBetweenFullRotation = 330;
    if (sampleTime > minDelayBetweenFullRotation)
    {
        bool const currentState = sys.digitalRead(pin) != 0;

        if (static_cast<int>(oldState) == 1 && static_cast<int>(currentState) == 0)
        {
            rev++;
            totalRev++;
            elapsedSampleTimeStamp = mls;
        }
        oldState = currentState;
    }

    const unsigned long intervalTime = mls - elapsedTimestamp;

    if (intervalTime > minDelayBetweenFullRotation)
    {
        if (rev == 0)
        {
            if (intervalTime > 5000)
            {
                rpm = 0.0F;
                if (intervalTime > 60000 * 15)
                {
                    rpm = -1.0F;
                }
                return rpm;
            }

            if (rpm > 0 && intervalTime > lastIntervalTime) // rpm forecast
            {

                if (intervalTime > lastIntervalTime * 2)
                {
                    rpm = calculateRpmFromRevolutions(1, intervalTime * 2);
                }
                else if (intervalTime > lastIntervalTime * 3)
                {
                    rpm = calculateRpmFromRevolutions(1, intervalTime * 3);
                }
                else if (intervalTime > lastIntervalTime * 4)
                {
                    rpm = calculateRpmFromRevolutions(1, intervalTime * 4);
                }
                else if (intervalTime > lastIntervalTime * 5)
                {
                    rpm = calculateRpmFromRevolutions(1, intervalTime * 5);
                }
                else if (intervalTime > lastIntervalTime * 6)
                {
                    rpm = calculateRpmFromRevolutions(1, intervalTime * 6);
                }
                else
                {
                    rpm = calculateRpmFromRevolutions(1, intervalTime);
                }
            }
        }
        else
        {
            rpm = calculateRpmFromRevolutions(rev, intervalTime);
            rev = 0;
            elapsedTimestamp = mls;
            lastIntervalTime = intervalTime;
            gattLastCrankRevolutionTimestamp = (gattLastCrankRevolutionTimestamp +
                                                // static_cast<uint16_t>(intervalTime * 1.024);
                                                static_cast<uint16_t>((intervalTime / 1000.F) * 1024.F)) %
                                               65536;
        }
    }

    return rpm;
}
