#include <Arduino.h>
#include <Cadence.h>

bool oldState;
unsigned long elapsedTimestamp = 0;
unsigned long elapsedSampleTimeStamp = 0;
uint8_t rev = 0;
uint32_t totalRev = 0;
static uint16_t gattLastCrankRevolutionTimestamp = 0;

uint16_t Cadence::getGattLastCrankRevolutionTimestamp()
{
    return (gattLastCrankRevolutionTimestamp);
}

uint32_t Cadence::totalRevs()
{
    return totalRev;
}

uint32_t Cadence::lastTimestamp()
{
    return elapsedTimestamp;
}

Cadence::Cadence(const uint8_t pinn) : pin(pinn)
{
    pinMode(pin, INPUT_PULLUP); // set pin to input
}

unsigned long calculateRpmFromRevolutions(const uint8_t revolutions, const unsigned long revolutionsTime)
{
    const unsigned long instantaneousRpm = 60000 / (revolutionsTime / revolutions);
    return instantaneousRpm;
}

auto Cadence::cadence() -> int16_t
{
    const unsigned long mls = millis();
    const unsigned long sampleTime = mls - elapsedSampleTimeStamp;

    if (sampleTime > 330)
    {
        bool currentState = digitalRead(pin);

        if (oldState == 1 && currentState == 0)
        {
            rev++;
            totalRev++;
            elapsedSampleTimeStamp = mls;
        }
        oldState = currentState;
    }

    const unsigned long intervalTime = mls - elapsedTimestamp;

    if (intervalTime > 330)
    {
        if (rev == 0)
        {
            if (intervalTime > 5000)
            {
                rpm = 0;
                if (intervalTime > 60000 * 15)
                {
                    rpm = -1;
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
                                                uint16_t(intervalTime / 1000.f * 1024.f)) %
                                               65536;
        }
    }

    return rpm;
}
