#include "Cadence.h"

Cadence* Cadence::_instance = nullptr;

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

void Cadence::enableInterrupt() {
    _instance = this;
    // We prepare the bridge, but we DO NOT call sys.attachInterrupt() yet.
    // That is the final step in the future.
}

// Static ISR: The Bridge
void Cadence::isr() {
    if (_instance != nullptr) {
        _instance->handleInterrupt();
    }
}

// Instance ISR: The Worker
void Cadence::handleInterrupt() {
    // Get time immediately
    uint32_t const now = sys.millis();
    onPulse(now);
}

Cadence::Cadence(const uint8_t pinn, ISystemWrapper& sys) noexcept
    : sys(sys),
      pin(pinn),
      oldState(false),
      elapsedTimestamp(0),
      elapsedSampleTimeStamp(0),
      rev(0),
      totalRev(0),
      gattLastCrankRevolutionTimestamp(0)
{
}

void Cadence::begin() {
    oldState = (sys.digitalRead(pin) != 0);
}

namespace {
    auto calculateRpmFromRevolutions(const uint8_t revolutions, const uint32_t revolutionsTime) -> float
    {
        if (revolutionsTime == 0) {
            return 0.0F;
        }
        // 60,000 ms in a minute
        const float instantaneousRpm = 60000.0F / (static_cast<float>(revolutionsTime) / static_cast<float>(revolutions));
        return instantaneousRpm;
    }

    constexpr float MILLISECONDS_IN_SECOND = 1000.0F;
    constexpr float GATT_TIME_UNITS_PER_SECOND = 1024.0F;
} // namespace

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
// ... onPulse ...

void Cadence::poll()
{
    // 1. Hardware Detection (Polling Driver)
    bool const currentState = sys.digitalRead(pin) != 0;
    if (oldState && !currentState) {
        onPulse(sys.millis());
    }
    oldState = currentState;
}

auto Cadence::cadence() -> float
{
    // NO HARDWARE READING HERE!
    // Only pure logic based on volatile state.

    const uint32_t mls = sys.millis();

    // 2. Atomic Snapshot
    sys.disableInterrupts();
    const uint8_t snapshotRev = rev;
    const uint32_t snapshotTimestamp = elapsedTimestamp;
    sys.enableInterrupts();

    const uint32_t intervalTime = mls - snapshotTimestamp;

    if (intervalTime > MIN_DELAY_BETWEEN_FULL_ROTATION_MS)
    {
        if (snapshotRev == 0)
        {
            if (intervalTime > MAX_IDLE_TIMEOUT_MS) {
                rpm = 0.0F;
                if (intervalTime > DEEP_SLEEP_TIMEOUT_MS) {
                    rpm = -1.0F;
                }
                return rpm;
            }
            if (rpm > 0 && lastIntervalTime > 0 && intervalTime > lastIntervalTime) {
                rpm = calculateRpmFromRevolutions(1, intervalTime);
            }
        }
        else
        {
            rpm = calculateRpmFromRevolutions(snapshotRev, intervalTime);

            sys.disableInterrupts();
            rev = 0;
            elapsedTimestamp = mls;
            sys.enableInterrupts();

            lastIntervalTime = intervalTime;
            gattLastCrankRevolutionTimestamp = (gattLastCrankRevolutionTimestamp +
                                                static_cast<uint16_t>((static_cast<float>(intervalTime) / MILLISECONDS_IN_SECOND) * GATT_TIME_UNITS_PER_SECOND));
        }
    }

    return rpm;
}
