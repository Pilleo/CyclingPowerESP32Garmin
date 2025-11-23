#include <unity.h>
#include "Cadence.h"
#include "MockSystemWrapper.h"

// Define a dummy pin for testing
const uint8_t TEST_PIN = 2;

MockSystemWrapper mockSys;
Cadence *cadenceInstance;

void setUp(void) {
    // set stuff up here
    // Re-initialize mockSys and cadenceInstance before each test
    mockSys = MockSystemWrapper(); // Reset mock system wrapper
    cadenceInstance = new Cadence(TEST_PIN, mockSys);
}

void tearDown(void) {
    // clean stuff up here
    delete cadenceInstance;
    cadenceInstance = nullptr;
}

void test_cadence_initialization() {
    TEST_ASSERT_EQUAL(0, cadenceInstance->cadence());
    TEST_ASSERT_EQUAL(0, cadenceInstance->getGattLastCrankRevolutionTimestamp());
    TEST_ASSERT_EQUAL(0, cadenceInstance->lastTimestamp());
    TEST_ASSERT_EQUAL(0, cadenceInstance->totalRevs());
}

void test_cadence_steady_60rpm() {
    // Simulate 60 RPM: 1 revolution per second (1000ms)
    // The system should detect a high->low transition as a pulse.
    // Cadence calculation interval is 330ms, and it checks for intervalTime > 330
    // so we need to ensure enough calls to cadence()
    const unsigned long REVOLUTION_INTERVAL_MS = 1000; // 1 revolution per second
    const unsigned long SIMULATION_DURATION_MS = 10 * REVOLUTION_INTERVAL_MS; // Simulate for 10 seconds

    // Initial state: pin is HIGH
    mockSys.setPinState(true);
    mockSys.setMillis(0);

    // Call cadence once to initialize internal state with pin HIGH
    cadenceInstance->cadence();


    // Simulate steady pulses for 10 seconds
    for (unsigned long t = 0; t <= SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS ; t += 10) { // Increment time by small steps
        mockSys.setMillis(t);

        // Simulate high state for the first half of the interval, then low for the second half
        // For a magnetic reed switch, typically it's closed (LOW) when magnet is near, open (HIGH) otherwise.
        // So, HIGH -> LOW transition means magnet passed.
        // Let's assume pin is HIGH normally, and goes LOW briefly when magnet passes.
        // We'll make it LOW for 10ms to ensure the `cadence()` call detects it.
        bool pinIsLow = (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0); // At every second mark, pin goes low

        // Simulate the pulse: HIGH -> LOW transition
        if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) { // Just at the moment of revolution
            mockSys.setPinState(false); // Pin goes LOW
        } else {
            mockSys.setPinState(true); // Pin stays HIGH
        }

        // Call cadence to process
        cadenceInstance->cadence();

        // After the pin goes low, set it back to high for the next cycle,
        // but only after cadence() has had a chance to read the low state.
        // The `cadence()` function checks `oldState == 1 && currentState == 0`.
        // So we need to ensure the pin is LOW only for one call, then back to HIGH.
        // This is handled by the loop incrementing `t` by 10ms and setting pinState directly.
    }


    // After some time, the RPM should stabilize to 60
    // We need to call cadence() one more time after the last simulated pulse
    // to ensure the last interval is processed.
    mockSys.setMillis(SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS + 100);
    cadenceInstance->cadence();

    // The calculation happens when `intervalTime > 330`
    // It takes some time for the rpm to be calculated and updated.
    // Let's run `cadence()` a few more times at intervals to ensure it settles.
    // The last update would have happened at SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS
    // So, check the value at a time slightly after that.

    TEST_MESSAGE("Checking RPM after simulation");
    TEST_ASSERT_EQUAL(60, cadenceInstance->cadence());
}

void test_cadence_steady_10rpm() {
    // Simulate 10 RPM: 1 revolution per 6 seconds (6000ms)
    const unsigned long REVOLUTION_INTERVAL_MS = 6000;
    const unsigned long SIMULATION_DURATION_MS = 5 * REVOLUTION_INTERVAL_MS; // Simulate for 5 revolutions

    mockSys.setPinState(true);
    mockSys.setMillis(0);

    cadenceInstance->cadence();

    for (unsigned long t = 0; t <= SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS ; t += 10) {
        mockSys.setMillis(t);

        if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) {
            mockSys.setPinState(false); // Pin goes LOW
        } else {
            mockSys.setPinState(true); // Pin stays HIGH
        }
        cadenceInstance->cadence();
    }

    mockSys.setMillis(SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS + 100);
    cadenceInstance->cadence();

    TEST_MESSAGE("Checking RPM after 10 RPM simulation");
    TEST_ASSERT_EQUAL(10, cadenceInstance->cadence());
}

void test_cadence_steady_120rpm() {
    // Simulate 120 RPM: 1 revolution per 0.5 seconds (500ms)
    const unsigned long REVOLUTION_INTERVAL_MS = 500;
    const unsigned long SIMULATION_DURATION_MS = 10 * REVOLUTION_INTERVAL_MS; // Simulate for 10 revolutions

    mockSys.setPinState(true);
    mockSys.setMillis(0);

    cadenceInstance->cadence();

    for (unsigned long t = 0; t <= SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS ; t += 10) {
        mockSys.setMillis(t);

        if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) {
            mockSys.setPinState(false); // Pin goes LOW
        } else {
            mockSys.setPinState(true); // Pin stays HIGH
        }
        cadenceInstance->cadence();
    }

    mockSys.setMillis(SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS + 100);
    cadenceInstance->cadence();

    TEST_MESSAGE("Checking RPM after 120 RPM simulation");
    TEST_ASSERT_EQUAL(120, cadenceInstance->cadence());
}

void test_cadence_sudden_stop() {
    // Simulate 60 RPM for a few seconds, then stop pulses and expect RPM to drop to 0.
    const unsigned long REVOLUTION_INTERVAL_MS = 1000; // 60 RPM
    const unsigned long ACTIVE_SIMULATION_DURATION_MS = 5 * REVOLUTION_INTERVAL_MS; // Simulate 5 revolutions
    const unsigned long STOP_DURATION_MS = 6000; // Stop for 6 seconds (more than 5000ms threshold)

    mockSys.setPinState(true);
    mockSys.setMillis(0);
    cadenceInstance->cadence();

    // Simulate active cycling for a few revolutions
    TEST_MESSAGE("Simulating active cycling (60 RPM) for 5 seconds.");
    for (unsigned long t = 0; t <= ACTIVE_SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS ; t += 10) {
        mockSys.setMillis(t);
        if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) {
            mockSys.setPinState(false);
        } else {
            mockSys.setPinState(true);
        }
        cadenceInstance->cadence();
    }

    mockSys.setMillis(ACTIVE_SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS + 100);
    cadenceInstance->cadence(); // Ensure last interval is processed
    TEST_ASSERT_EQUAL(60, cadenceInstance->cadence()); // Should be 60 RPM initially

    TEST_MESSAGE("Stopping pulses and waiting for RPM to drop to 0.");

    // Stop pulses (pin remains HIGH)
    mockSys.setPinState(true);

    // Advance time beyond the 5-second threshold without any pulses
    unsigned long stopTimeStart = mockSys.millis();
    for (unsigned long t = stopTimeStart; t <= stopTimeStart + STOP_DURATION_MS; t += 100) {
        mockSys.setMillis(t);
        cadenceInstance->cadence();
    }

    // After stop duration, RPM should be 0
    TEST_ASSERT_EQUAL(0, cadenceInstance->cadence());
}

void test_cadence_very_long_stop() {
    // Simulate 60 RPM for a few seconds, then stop pulses for more than 15 minutes and expect RPM to be -1.
    const unsigned long REVOLUTION_INTERVAL_MS = 1000; // 60 RPM
    const unsigned long ACTIVE_SIMULATION_DURATION_MS = 5 * REVOLUTION_INTERVAL_MS; // Simulate 5 revolutions
    const unsigned long VERY_LONG_STOP_DURATION_MS = 60000 * 16; // Stop for 16 minutes (more than 15 minutes threshold)

    mockSys.setPinState(true);
    mockSys.setMillis(0);
    cadenceInstance->cadence();

    // Simulate active cycling for a few revolutions
    TEST_MESSAGE("Simulating active cycling (60 RPM) for 5 seconds.");
    for (unsigned long t = 0; t <= ACTIVE_SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS ; t += 10) {
        mockSys.setMillis(t);
        if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) {
            mockSys.setPinState(false);
        } else {
            mockSys.setPinState(true);
        }
        cadenceInstance->cadence();
    }

    mockSys.setMillis(ACTIVE_SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS + 100);
    cadenceInstance->cadence(); // Ensure last interval is processed
    TEST_ASSERT_EQUAL(60, cadenceInstance->cadence()); // Should be 60 RPM initially

    TEST_MESSAGE("Stopping pulses and waiting for RPM to drop to -1 after a very long stop.");

    // Stop pulses (pin remains HIGH)
    mockSys.setPinState(true);

    // Advance time beyond the 15-minute threshold without any pulses
    unsigned long stopTimeStart = mockSys.millis();
    for (unsigned long t = stopTimeStart; t <= stopTimeStart + VERY_LONG_STOP_DURATION_MS; t += 10000) { // Increment by 10s for faster simulation
        mockSys.setMillis(t);
        cadenceInstance->cadence();
    }

    // After very long stop duration, RPM should be -1
    TEST_ASSERT_EQUAL(-1, cadenceInstance->cadence());
}

void test_cadence_inconsistent_pulses() {
    // Simulate pulses with an average of 60 RPM (1000ms interval), but with some jitter (+/- 100ms)
    const unsigned long BASE_REVOLUTION_INTERVAL_MS = 1000; // Average 60 RPM
    const unsigned long JITTER_MS = 100; // +/- 100ms jitter
    const unsigned long NUM_REVOLUTIONS = 20;
    const unsigned long SIMULATION_DURATION_MS = NUM_REVOLUTIONS * BASE_REVOLUTION_INTERVAL_MS;

    mockSys.setPinState(true);
    mockSys.setMillis(0);
    cadenceInstance->cadence();

    unsigned long current_time = 0;
    for (unsigned int i = 0; i < NUM_REVOLUTIONS; ++i) {
        // Simulate jitter by adding/subtracting some random value (for testing, we'll use a predictable pattern)
        // For simplicity in a unit test, I'll make the jitter predictable.
        unsigned long interval = BASE_REVOLUTION_INTERVAL_MS;
        if (i % 2 == 0) { // Even revolutions, subtract jitter
            interval -= JITTER_MS;
        } else { // Odd revolutions, add jitter
            interval += JITTER_MS;
        }

        current_time += interval; // Advance time for the next revolution

        // Simulate going HIGH then LOW to trigger a pulse
        mockSys.setMillis(current_time - 50); // Ensure some time where pin is HIGH
        mockSys.setPinState(true);
        cadenceInstance->cadence();

        mockSys.setMillis(current_time); // Pin goes LOW at the revolution time
        mockSys.setPinState(false);
        cadenceInstance->cadence();

        // Keep calling cadence to allow it to process the state changes
        mockSys.setMillis(current_time + 10);
        mockSys.setPinState(true); // Pin goes back HIGH
        cadenceInstance->cadence();

        // Advance time in small steps for the rest of the interval
        for (unsigned long t_step = current_time + 20; t_step < current_time + interval; t_step += 50) {
            mockSys.setMillis(t_step);
            cadenceInstance->cadence();
        }
    }

    // After the simulation, the RPM should be close to 60
    mockSys.setMillis(current_time + BASE_REVOLUTION_INTERVAL_MS + 100); // Allow some time for final calculation
    cadenceInstance->cadence();

    int16_t actual_rpm = cadenceInstance->cadence();
    TEST_MESSAGE("Checking RPM after inconsistent pulses simulation");
    TEST_ASSERT_TRUE(actual_rpm >= (60 - 10) && actual_rpm <= (60 + 10)); // Allow a larger tolerance for average RPM with jitter
}

void test_cadence_debouncing() {
    // Simulate pulses arriving faster than the 330ms debouncing threshold
    // Expect only one revolution to be counted within the short interval.
    const unsigned long DEBOUNCE_THRESHOLD_MS = 330;
    const unsigned long SHORT_INTERVAL_MS = 100; // Shorter than debounce threshold

    mockSys.setPinState(true);
    mockSys.setMillis(0);
    mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 10); // Ensure initial sampleTime > 330ms for the first pulse
    cadenceInstance->cadence(); // Initialize

    // Simulate first pulse
    mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100); // Trigger first pulse after enough time
    mockSys.setPinState(false); // LOW
    cadenceInstance->cadence();
    mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 110); // HIGH
    mockSys.setPinState(true); // HIGH
    cadenceInstance->cadence();

    // After first pulse, total revs should be 1
    TEST_ASSERT_EQUAL(1, cadenceInstance->totalRevs());

    // Simulate a second pulse very quickly after the first (e.g., after 100ms from the first detection)
    // The elapsedSampleTimeStamp was just updated to (DEBOUNCE_THRESHOLD_MS + 100).
    // So, sampleTime will be (DEBOUNCE_THRESHOLD_MS + 100 + SHORT_INTERVAL_MS) - (DEBOUNCE_THRESHOLD_MS + 100) = SHORT_INTERVAL_MS (100ms)
    // This should be less than 330ms, so it should be ignored.
    mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 + SHORT_INTERVAL_MS); // Trigger second pulse
    mockSys.setPinState(false); // LOW
    cadenceInstance->cadence();
    mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 + SHORT_INTERVAL_MS + 10); // HIGH
    mockSys.setPinState(true); // HIGH
    cadenceInstance->cadence();

    // Simulate a third pulse (e.g., after another 100ms) - also should be ignored
    mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 + SHORT_INTERVAL_MS * 2); // Trigger third pulse
    mockSys.setPinState(false); // LOW
    cadenceInstance->cadence();
    mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 + SHORT_INTERVAL_MS * 2 + 10); // HIGH
    mockSys.setPinState(true); // HIGH
    cadenceInstance->cadence();

    // Advance time well beyond the debounce threshold to allow a calculation to happen, but no new pulses should be counted.
    mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 + SHORT_INTERVAL_MS * 2 + DEBOUNCE_THRESHOLD_MS + 100);
    cadenceInstance->cadence();

    TEST_MESSAGE("Checking total revolutions after debouncing simulation");
    // Only the first pulse should have counted.
    
        TEST_ASSERT_EQUAL(1, cadenceInstance->totalRevs());
    }
    
    void test_cadence_total_revs() {
        // Simulate exactly 100 pulses and verify totalRevs() increments correctly.
        const unsigned long NUM_PULSES = 100;
        const unsigned long REVOLUTION_INTERVAL_MS = 1000; // 60 RPM for simplicity
    
        mockSys.setPinState(true);
        mockSys.setMillis(0);
        cadenceInstance->cadence(); // Initialize
    
        TEST_MESSAGE("Simulating 100 pulses and checking total revolutions.");
    
        for (unsigned long i = 0; i < NUM_PULSES; ++i) {
            // Advance time for each pulse, ensuring it's outside the debounce threshold
            unsigned long currentPulseTime = (i * REVOLUTION_INTERVAL_MS) + REVOLUTION_INTERVAL_MS;
            
            mockSys.setMillis(currentPulseTime - 50); // Pin HIGH before LOW
            mockSys.setPinState(true);
            cadenceInstance->cadence();
    
            mockSys.setMillis(currentPulseTime); // Pin LOW (trigger)
            mockSys.setPinState(false);
            cadenceInstance->cadence();
    
            mockSys.setMillis(currentPulseTime + 10); // Pin HIGH again
            mockSys.setPinState(true);
            cadenceInstance->cadence();
        }
    
        TEST_MESSAGE("Checking final total revolutions.");
        TEST_ASSERT_EQUAL(NUM_PULSES, cadenceInstance->totalRevs());
    }
    
    int main() { // Renamed setup() to main() and changed return type to int
        UNITY_BEGIN();
    
        // Run tests
        RUN_TEST(test_cadence_initialization);
        RUN_TEST(test_cadence_steady_60rpm);
        RUN_TEST(test_cadence_steady_10rpm);
        RUN_TEST(test_cadence_steady_120rpm);
        RUN_TEST(test_cadence_sudden_stop);
        RUN_TEST(test_cadence_very_long_stop);
        RUN_TEST(test_cadence_inconsistent_pulses);
        RUN_TEST(test_cadence_debouncing);
        RUN_TEST(test_cadence_total_revs);
    
        return UNITY_END(); // Return test result
    }
    