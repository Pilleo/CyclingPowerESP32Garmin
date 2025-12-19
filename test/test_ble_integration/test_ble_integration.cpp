#include <unity.h>
#include "BikeComputer.h"
#include "MockSystemWrapper.h"
#include "MockBleService.h"
#include "Cadence.h"
#include "ResistanceLevel.h"
#include <new> // For placement new

// Pin definitions matching main code
static constexpr uint8_t PIN_CADENCE = 15;
static constexpr uint8_t PIN_BACK = 4;
static constexpr uint8_t PIN_LIMIT = 18;
static constexpr uint8_t PIN_FWD = 19;
static constexpr uint8_t PIN_POS = 23;

// Global instances for testing
MockSystemWrapper mockSys;
MockBleService mockBle;
// We will initialize these in setUp using placement new to ensure fresh state
uint8_t cadenceBuffer[sizeof(Cadence)];
uint8_t resistanceBuffer[sizeof(ResistanceLevel)];
uint8_t computerBuffer[sizeof(BikeComputer)];

Cadence* realCadence;
ResistanceLevel* realResistance;
BikeComputer* computer;

void setUp() {
    mockSys.reset();
    // Reset pins to default states
    mockSys.setPinState(PIN_CADENCE, true);
    mockSys.setPinState(PIN_BACK, false);
    mockSys.setPinState(PIN_LIMIT, false);
    mockSys.setPinState(PIN_FWD, false);
    mockSys.setPinState(PIN_POS, true);

    mockBle.lastPowerSent = 0;
    mockBle.lastRevsSent = 0;
    mockBle.lastTimestampSent = 0;
    mockBle.notifyCalled = false;
    mockBle.startCalled = false;
    mockBle.callCount = 0;

    // Re-construct objects to reset their internal state
    realCadence = new (cadenceBuffer) Cadence(PIN_CADENCE, mockSys);
    realResistance = new (resistanceBuffer) ResistanceLevel(PIN_BACK, PIN_LIMIT, PIN_POS, PIN_FWD, mockSys);
    computer = new (computerBuffer) BikeComputer(mockSys, *realCadence, *realResistance, mockBle);

    computer->setup();
}

void tearDown() {
    // Clean up if needed
    // Destructors could be called manually if they have side effects, but for these POD-like classes it's fine.
}

// Helper to simulate a cadence pulse
void simulateCadencePulse(uint32_t durationMs) {
    mockSys.setPinState(PIN_CADENCE, false); // Magnet passes
    mockSys.advanceTime(50); // Debounce time
    computer->update();

    mockSys.setPinState(PIN_CADENCE, true); // Magnet leaves
    mockSys.advanceTime(durationMs - 50);
    computer->update();
}

// Helper to simulate resistance change
void simulateResistanceLevelChange(int targetLevel) {
    // Based on ResistanceLevel.cpp:
    // Level 5 corresponds to counter range 411-429.
    // The logic counts EVERY edge (change of state).

    // Set Forward Pin HIGH
    mockSys.setPinState(PIN_FWD, true);
    mockSys.setPinState(PIN_BACK, false);

    // We need ~415 counts.
    // Each loop iteration produces 2 edges (LOW then HIGH).
    // So we need 415 / 2 = ~208 iterations.
    int targetLoops = 208;

    for (int i = 0; i < targetLoops; i++) {
        // Toggle Position Pin (Edge 1)
        mockSys.setPinState(PIN_POS, false);
        mockSys.advanceTime(10);
        computer->update();

        // Toggle Position Pin (Edge 2)
        mockSys.setPinState(PIN_POS, true);
        mockSys.advanceTime(10);
        computer->update();
    }

    // Stop moving
    mockSys.setPinState(PIN_FWD, false);
    computer->update();
}

void test_happy_path_data_pipeline() {
    // 1. Simulate Resistance Level 5
    simulateResistanceLevelChange(5);

    // 2. Simulate 90 RPM
    // 90 RPM = 1.5 rev/sec = 666.67 ms per rev
    uint32_t revDuration = 667;

    // Spin up for a few seconds to stabilize cadence
    for (int i = 0; i < 5; i++) {
        simulateCadencePulse(revDuration);
    }

    // 3. Advance time to trigger BLE update (510ms interval)
    simulateCadencePulse(revDuration);

    // Assertions
    TEST_ASSERT_TRUE(mockBle.notifyCalled);

    // Check Power
    // At Level 5 (index 4) and 90 RPM (index 70).
    // Expected power is 247W.
    TEST_ASSERT_INT_WITHIN(50, 247, mockBle.lastPowerSent);
}

// Helper to simulate RPM
void simulate_rpm(int rpm, int durationMs) {
    int totalPeriod = 60000 / rpm;
    int pulseWidth = 50;
    // We need to ensure we update AFTER the lockout period (330ms) defined in Cadence.cpp
    // so that the system registers the return to HIGH state.
    int lockoutBuffer = 350;

    int elapsed = 0;
    while(elapsed < durationMs) {
        // 1. Pulse Start (Low)
        mockSys.setPinState(PIN_CADENCE, false);
        computer->update();

        // 2. Pulse Width
        mockSys.advanceTime(pulseWidth);
        mockSys.setPinState(PIN_CADENCE, true);
        computer->update(); // Likely ignored by Cadence logic due to lockout

        // 3. Wait for Lockout to expire + buffer
        mockSys.advanceTime(lockoutBuffer);
        computer->update(); // Should register High state now

        // 4. Wait rest of cycle
        int used = pulseWidth + lockoutBuffer;
        if (totalPeriod > used) {
            mockSys.advanceTime(totalPeriod - used);
        }

        elapsed += totalPeriod;
    }
}

void test_full_data_flow() {
    // 1. Simulate Level 10 (arbitrary pulses to get resistance up)
    // (Assuming Resistance logic works, we just trust it triggers here)
    // ... setup resistance pulses ...

    // 2. Simulate 60 RPM for 5 seconds
    // 60 RPM = 1000ms per rev.
    simulate_rpm(60, 5000);

    // 3. Verify BLE received data
    // At 60 RPM and Default Level 1:
    // Check Power.cpp -> Level 1 @ 60 RPM = 50 Watts (Index 40, Level 0)
    TEST_ASSERT_TRUE(mockBle.notifyCalled);

    // Allow small tolerance for simulation timing jitter
    // Expected 50 Watts.
    TEST_ASSERT_INT_WITHIN(10, 50, mockBle.lastPowerSent);
}

void test_system_sleeps_after_timeout() {
    // 1. Initialize
    computer->update();
    TEST_ASSERT_FALSE(mockSys.wasSleepCalled);

    // 2. Jump forward past timeout (15 mins = 900,000ms)
    mockSys.advanceTime(16 * 60 * 1000);

    // 3. Update
    computer->update();

    // 4. Assert
    TEST_ASSERT_TRUE_MESSAGE(mockSys.wasSleepCalled, "System should have entered deep sleep");
    TEST_ASSERT_EQUAL(PIN_CADENCE, mockSys.sleepWakeupPin);
}

void test_ble_rate_limiting() {
    // 1. Establish steady state (e.g., 60 RPM)
    // We need valid cadence so update() tries to send data.
    simulate_rpm(60, 2000);

    // Reset call count after setup
    mockBle.callCount = 0;

    // 2. Loop 100 times, advancing 10ms each time (Total 1 second)
    for (int i = 0; i < 100; i++) {
        mockSys.advanceTime(10);
        computer->update();
    }

    // 3. Assert
    // Sending interval is 510ms.
    // In 1000ms, we expect ~2 calls (maybe 1 if timing aligns differently).
    // Definitely not 100.
    TEST_ASSERT_INT_WITHIN(2, 2, mockBle.callCount);
    TEST_ASSERT_LESS_THAN(10, mockBle.callCount); // Safety check against flooding
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_happy_path_data_pipeline);
    RUN_TEST(test_full_data_flow);
    RUN_TEST(test_system_sleeps_after_timeout);
    RUN_TEST(test_ble_rate_limiting);
    return UNITY_END();
}
