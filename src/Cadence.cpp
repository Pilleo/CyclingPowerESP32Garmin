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

    // 1. Hardware Detection (Still Polling for now)
    bool const currentState = sys.digitalRead(pin) != 0;
    if (oldState && !currentState) {
        onPulse(mls);
    }
    oldState = currentState;

    // 2. Atomic Snapshot
    // We strictly assume 'rev' and 'elapsedSampleTimeStamp' are volatile
    // and could change at ANY moment if we were using interrupts.
    uint32_t snapshotTimestamp;
    uint8_t snapshotRev;

    // START CRITICAL SECTION
    sys.disableInterrupts();

    snapshotRev = rev;

    // We only reset if we are actually going to process them
    // But we need to check the interval first.
    // Actually, 'elapsedTimestamp' is the "Start of Interval".
    // 'rev' is the count SINCE 'elapsedTimestamp'.

    // We can't do complex math inside critical section.
    // But we can't reset 'rev' outside without risking race condition.
    // Strategy: Read ONLY.

    snapshotTimestamp = elapsedTimestamp;

    sys.enableInterrupts();
    // END CRITICAL SECTION

    const uint32_t intervalTime = mls - snapshotTimestamp;

    if (intervalTime > MIN_DELAY_BETWEEN_FULL_ROTATION_MS)
    {
        if (snapshotRev == 0)
        {
            // ... Idle logic (Same as before) ...
            if (intervalTime > MAX_IDLE_TIMEOUT_MS) {
                rpm = 0.0F;
                if (intervalTime > DEEP_SLEEP_TIMEOUT_MS) rpm = -1.0F;
                return rpm;
            }
            if (rpm > 0 && lastIntervalTime > 0 && intervalTime > lastIntervalTime) {
                rpm = calculateRpmFromRevolutions(1, intervalTime);
            }
        }
        else
        {
            // Valid revolution detected
            rpm = calculateRpmFromRevolutions(snapshotRev, intervalTime);

            // ATOMIC RESET
            // We processed 'snapshotRev'. We need to subtract that from 'rev'
            // or reset 'rev' to 0 IF it hasn't changed.
            // Simplest safe approach: Reset everything and start new interval.

            sys.disableInterrupts();
            rev = 0;
            elapsedTimestamp = mls; // Start new interval now
            // Note: In extremely high freq interrupts, we might lose a pulse that happened
            // between the first sys.interrupts() and this sys.noInterrupts().
            // For Cadence (max 2Hz), this window is negligible.
            sys.enableInterrupts();

            lastIntervalTime = intervalTime;
            gattLastCrankRevolutionTimestamp = (gattLastCrankRevolutionTimestamp +
                                                static_cast<uint16_t>((intervalTime / 1000.F) * 1024.F));
        }
    }

    return rpm;
}