#include "../../src/Cadence.h"
#include "../common/MockSystemWrapper.h"
#include <unity.h>

// Define a dummy pin for testing
const uint8_t TEST_PIN = 2;

MockSystemWrapper mockSys;
Cadence *cadenceInstance;

void setUp(void) {
  // set stuff up here
  // Re-initialize mockSys and cadenceInstance before each test
  mockSys.reset();
  cadenceInstance = new Cadence(TEST_PIN, mockSys);
}

void tearDown(void) {
  // clean stuff up here
  delete cadenceInstance;
  cadenceInstance = nullptr;
}

void test_cadence_initialization() {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, cadenceInstance->cadence());
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
  const unsigned long SIMULATION_DURATION_MS =
      10 * REVOLUTION_INTERVAL_MS; // Simulate for 10 seconds

  // Initial state: pin is HIGH
  mockSys.setPinState(TEST_PIN, true);
  mockSys.setMillis(0);

  // Call cadence once to initialize internal state with pin HIGH
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // Simulate steady pulses for 10 seconds
  for (unsigned long t = 0;
       t <= SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS; t += 10) {
    // Increment time by small steps
    mockSys.setMillis(t);

    // Simulate high state for the first half of the interval, then low for the
    // second half For a magnetic reed switch, typically it's closed (LOW) when
    // magnet is near, open (HIGH) otherwise. So, HIGH -> LOW transition means
    // magnet passed. Let's assume pin is HIGH normally, and goes LOW briefly
    // when magnet passes. We'll make it LOW for 10ms to ensure the `cadence()`
    // call detects it.
    bool pinIsLow = (t > 0 && (t % REVOLUTION_INTERVAL_MS) ==
                                  0); // At every second mark, pin goes low

    // Simulate the pulse: HIGH -> LOW transition
    if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) {
      // Just at the moment of revolution
      mockSys.setPinState(TEST_PIN, false); // Pin goes LOW
    } else {
      mockSys.setPinState(TEST_PIN, true); // Pin stays HIGH
    }

    // Call cadence to process
    cadenceInstance->poll();
    cadenceInstance->cadence();

    // After the pin goes low, set it back to high for the next cycle,
    // but only after cadence() has had a chance to read the low state.
    // The `cadence()` function checks `oldState == 1 && currentState == 0`.
    // So we need to ensure the pin is LOW only for one call, then back to HIGH.
    // This is handled by the loop incrementing `t` by 10ms and setting pinState
    // directly.
  }

  // After some time, the RPM should stabilize to 60
  // We need to call cadence() one more time after the last simulated pulse
  // to ensure the last interval is processed.
  mockSys.setMillis(SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS + 100);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // The calculation happens when `intervalTime > 330`
  // It takes some time for the rpm to be calculated and updated.
  // Let's run `cadence()` a few more times at intervals to ensure it settles.
  // The last update would have happened at SIMULATION_DURATION_MS +
  // REVOLUTION_INTERVAL_MS So, check the value at a time slightly after that.

  TEST_MESSAGE("Checking RPM after simulation");
  TEST_ASSERT_EQUAL_FLOAT(60.0f, cadenceInstance->cadence());
}

void test_cadence_steady_10rpm() {
  // Simulate 10 RPM: 1 revolution per 6 seconds (6000ms)
  const unsigned long REVOLUTION_INTERVAL_MS = 6000;
  const unsigned long SIMULATION_DURATION_MS =
      5 * REVOLUTION_INTERVAL_MS; // Simulate for 5 revolutions

  mockSys.setPinState(TEST_PIN, true);
  mockSys.setMillis(0);

  cadenceInstance->poll();
  cadenceInstance->cadence();

  for (unsigned long t = 0;
       t <= SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS; t += 10) {
    mockSys.setMillis(t);

    if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) {
      mockSys.setPinState(TEST_PIN, false); // Pin goes LOW
    } else {
      mockSys.setPinState(TEST_PIN, true); // Pin stays HIGH
    }
    cadenceInstance->poll();
    cadenceInstance->cadence();
  }

  mockSys.setMillis(SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS + 100);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  TEST_MESSAGE("Checking RPM after 10 RPM simulation");
  TEST_ASSERT_EQUAL_FLOAT(10.0f, cadenceInstance->cadence());
}

void test_cadence_steady_120rpm() {
  // Simulate 120 RPM: 1 revolution per 0.5 seconds (500ms)
  const unsigned long REVOLUTION_INTERVAL_MS = 500;
  const unsigned long SIMULATION_DURATION_MS =
      10 * REVOLUTION_INTERVAL_MS; // Simulate for 10 revolutions

  mockSys.setPinState(TEST_PIN, true);
  mockSys.setMillis(0);

  cadenceInstance->poll();
  cadenceInstance->cadence();

  for (unsigned long t = 0;
       t <= SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS; t += 10) {
    mockSys.setMillis(t);

    if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) {
      mockSys.setPinState(TEST_PIN, false); // Pin goes LOW
    } else {
      mockSys.setPinState(TEST_PIN, true); // Pin stays HIGH
    }
    cadenceInstance->poll();
    cadenceInstance->cadence();
  }

  mockSys.setMillis(SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS + 100);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  TEST_MESSAGE("Checking RPM after 120 RPM simulation");
  TEST_ASSERT_EQUAL_FLOAT(120.0f, cadenceInstance->cadence());
}

void test_cadence_sudden_stop() {
  // Simulate 60 RPM for a few seconds, then stop pulses and expect RPM to drop
  // to 0.
  const unsigned long REVOLUTION_INTERVAL_MS = 1000; // 60 RPM
  const unsigned long ACTIVE_SIMULATION_DURATION_MS =
      5 * REVOLUTION_INTERVAL_MS; // Simulate 5 revolutions
  const unsigned long STOP_DURATION_MS =
      6000; // Stop for 6 seconds (more than 5000ms threshold)

  mockSys.setPinState(TEST_PIN, true);
  mockSys.setMillis(0);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // Simulate active cycling for a few revolutions
  TEST_MESSAGE("Simulating active cycling (60 RPM) for 5 seconds.");
  for (unsigned long t = 0;
       t <= ACTIVE_SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS; t += 10) {
    mockSys.setMillis(t);
    if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) {
      mockSys.setPinState(TEST_PIN, false);
    } else {
      mockSys.setPinState(TEST_PIN, true);
    }
    cadenceInstance->poll();
    cadenceInstance->cadence();
  }

  mockSys.setMillis(ACTIVE_SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS +
                    100);
  cadenceInstance->poll();
  cadenceInstance->cadence(); // Ensure last interval is processed
  TEST_ASSERT_EQUAL_FLOAT(
      60.0f, cadenceInstance->cadence()); // Should be 60 RPM initially

  TEST_MESSAGE("Stopping pulses and waiting for RPM to drop to 0.");

  // Stop pulses (pin remains HIGH)
  mockSys.setPinState(TEST_PIN, true);

  // Advance time beyond the 5-second threshold without any pulses
  unsigned long stopTimeStart = mockSys.millis();
  for (unsigned long t = stopTimeStart; t <= stopTimeStart + STOP_DURATION_MS;
       t += 100) {
    mockSys.setMillis(t);
    cadenceInstance->poll();
    cadenceInstance->cadence();
  }

  // After stop duration, RPM should be 0
  TEST_ASSERT_EQUAL_FLOAT(0.0f, cadenceInstance->cadence());
}

void test_cadence_very_long_stop() {
  // Simulate 60 RPM for a few seconds, then stop pulses for more than 15
  // minutes and expect RPM to be -1.
  const unsigned long REVOLUTION_INTERVAL_MS = 1000; // 60 RPM
  const unsigned long ACTIVE_SIMULATION_DURATION_MS =
      5 * REVOLUTION_INTERVAL_MS; // Simulate 5 revolutions
  const unsigned long VERY_LONG_STOP_DURATION_MS =
      60000 * 16; // Stop for 16 minutes (more than 15 minutes threshold)

  mockSys.setPinState(TEST_PIN, true);
  mockSys.setMillis(0);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // Simulate active cycling for a few revolutions
  TEST_MESSAGE("Simulating active cycling (60 RPM) for 5 seconds.");
  for (unsigned long t = 0;
       t <= ACTIVE_SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS; t += 10) {
    mockSys.setMillis(t);
    if (t > 0 && (t % REVOLUTION_INTERVAL_MS) == 0) {
      mockSys.setPinState(TEST_PIN, false);
    } else {
      mockSys.setPinState(TEST_PIN, true);
    }
    cadenceInstance->poll();
    cadenceInstance->cadence();
  }

  mockSys.setMillis(ACTIVE_SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS +
                    100);
  cadenceInstance->poll();
  cadenceInstance->cadence(); // Ensure last interval is processed
  TEST_ASSERT_EQUAL_FLOAT(
      60.0f, cadenceInstance->cadence()); // Should be 60 RPM initially

  TEST_MESSAGE("Stopping pulses and waiting for RPM to drop to -1 after a very "
               "long stop.");

  // Stop pulses (pin remains HIGH)
  mockSys.setPinState(TEST_PIN, true);

  // Advance time beyond the 15-minute threshold without any pulses
  unsigned long stopTimeStart = mockSys.millis();
  for (unsigned long t = stopTimeStart;
       t <= stopTimeStart + VERY_LONG_STOP_DURATION_MS; t += 10000) {
    // Increment by 10s for faster simulation
    mockSys.setMillis(t);
    cadenceInstance->poll();
    cadenceInstance->cadence();
  }

  // After very long stop duration, RPM should be -1
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, cadenceInstance->cadence());
}

void test_cadence_inconsistent_pulses() {
  // Simulate pulses with an average of 60 RPM (1000ms interval), but with some
  // jitter (+/- 100ms)
  const unsigned long BASE_REVOLUTION_INTERVAL_MS = 1000; // Average 60 RPM
  const unsigned long JITTER_MS = 100;                    // +/- 100ms jitter
  const unsigned long NUM_REVOLUTIONS = 20;
  const unsigned long SIMULATION_DURATION_MS =
      NUM_REVOLUTIONS * BASE_REVOLUTION_INTERVAL_MS;

  mockSys.setPinState(TEST_PIN, true);
  mockSys.setMillis(0);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  unsigned long current_time = 0;
  for (unsigned int i = 0; i < NUM_REVOLUTIONS; ++i) {
    // Simulate jitter by adding/subtracting some random value (for testing,
    // we'll use a predictable pattern) For simplicity in a unit test, I'll make
    // the jitter predictable.
    unsigned long interval = BASE_REVOLUTION_INTERVAL_MS;
    if (i % 2 == 0) {
      // Even revolutions, subtract jitter
      interval -= JITTER_MS;
    } else {
      // Odd revolutions, add jitter
      interval += JITTER_MS;
    }

    current_time += interval; // Advance time for the next revolution

    // Simulate going HIGH then LOW to trigger a pulse
    mockSys.setMillis(current_time - 50); // Ensure some time where pin is HIGH
    mockSys.setPinState(TEST_PIN, true);
    cadenceInstance->poll();
    cadenceInstance->cadence();

    mockSys.setMillis(current_time); // Pin goes LOW at the revolution time
    mockSys.setPinState(TEST_PIN, false);
    cadenceInstance->poll();
    cadenceInstance->cadence();

    // Keep calling cadence to allow it to process the state changes
    mockSys.setMillis(current_time + 10);
    mockSys.setPinState(TEST_PIN, true); // Pin goes back HIGH
    cadenceInstance->poll();
    cadenceInstance->cadence();

    // Advance time in small steps for the rest of the interval
    for (unsigned long t_step = current_time + 20;
         t_step < current_time + interval; t_step += 50) {
      mockSys.setMillis(t_step);
      cadenceInstance->poll();
      cadenceInstance->cadence();
    }
  }

  // After the simulation, the RPM should be close to 60
  mockSys.setMillis(current_time + BASE_REVOLUTION_INTERVAL_MS +
                    100); // Allow some time for final calculation
  cadenceInstance->poll();
  cadenceInstance->cadence();
  cadenceInstance->poll();
  float actual_rpm = cadenceInstance->cadence();

  TEST_MESSAGE("Checking RPM after inconsistent pulses simulation");

  TEST_ASSERT_TRUE(actual_rpm >= (60.0f - 10.0f) &&
                   actual_rpm <= (60.0f + 10.0f));
  // Allow a larger tolerance for average RPM with jitter
}

void test_cadence_fractional_rpm() {
  // Simulate ~60.5 RPM with a steady interval of 992ms.

  const unsigned long REVOLUTION_INTERVAL_MS = 992;

  const unsigned long SIMULATION_DURATION_MS = 10 * REVOLUTION_INTERVAL_MS;

  mockSys.setPinState(TEST_PIN, true);

  mockSys.setMillis(0);

  cadenceInstance->poll();
  cadenceInstance->cadence();

  for (unsigned long t = 1;
       t <= SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS; ++t) {
    mockSys.setMillis(t);

    if ((t % REVOLUTION_INTERVAL_MS) == 0) {
      mockSys.setPinState(TEST_PIN, false);

      cadenceInstance->poll();
      cadenceInstance->cadence();

      mockSys.setPinState(TEST_PIN, true);
    } else {
      cadenceInstance->poll();
      cadenceInstance->cadence();
    }
  }

  mockSys.setMillis(SIMULATION_DURATION_MS + REVOLUTION_INTERVAL_MS + 100);

  cadenceInstance->poll();
  cadenceInstance->cadence();

  cadenceInstance->poll();
  float actual_rpm_val = cadenceInstance->cadence();

  TEST_ASSERT_FLOAT_WITHIN(0.1f, 60.48f, actual_rpm_val);
}

void test_cadence_debouncing() {
  // Simulate pulses arriving faster than the 330ms debouncing threshold

  // Expect only one revolution to be counted within the short interval.

  const unsigned long DEBOUNCE_THRESHOLD_MS = 330;

  const unsigned long SHORT_INTERVAL_MS =
      100; // Shorter than debounce threshold

  mockSys.setPinState(TEST_PIN, true);

  mockSys.setMillis(0);

  mockSys.setMillis(
      DEBOUNCE_THRESHOLD_MS +
      10); // Ensure initial sampleTime > 330ms for the first pulse

  cadenceInstance->poll();
  cadenceInstance->cadence(); // Initialize

  // Simulate first pulse

  mockSys.setMillis(DEBOUNCE_THRESHOLD_MS +
                    100); // Trigger first pulse after enough time

  mockSys.setPinState(TEST_PIN, false); // LOW

  cadenceInstance->poll();
  cadenceInstance->cadence();

  mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 110); // HIGH

  mockSys.setPinState(TEST_PIN, true); // HIGH

  cadenceInstance->poll();
  cadenceInstance->cadence();

  // After first pulse, total revs should be 1

  TEST_ASSERT_EQUAL(1, cadenceInstance->totalRevs());

  // Simulate a second pulse very quickly after the first (e.g., after 100ms
  // from the first detection)

  mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 +
                    SHORT_INTERVAL_MS); // Trigger second pulse

  mockSys.setPinState(TEST_PIN, false); // LOW

  cadenceInstance->poll();
  cadenceInstance->cadence();

  mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 + SHORT_INTERVAL_MS +
                    10); // HIGH

  mockSys.setPinState(TEST_PIN, true); // HIGH

  cadenceInstance->poll();
  cadenceInstance->cadence();

  // Simulate a third pulse (e.g., after another 100ms) - also should be ignored

  mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 +
                    SHORT_INTERVAL_MS * 2); // Trigger third pulse

  mockSys.setPinState(TEST_PIN, false); // LOW

  cadenceInstance->poll();
  cadenceInstance->cadence();

  mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 + SHORT_INTERVAL_MS * 2 +
                    10); // HIGH

  mockSys.setPinState(TEST_PIN, true); // HIGH

  cadenceInstance->poll();
  cadenceInstance->cadence();

  // Advance time well beyond the debounce threshold to allow a calculation to
  // happen, but no new pulses should be counted.

  mockSys.setMillis(DEBOUNCE_THRESHOLD_MS + 100 + SHORT_INTERVAL_MS * 2 +
                    DEBOUNCE_THRESHOLD_MS + 100);

  cadenceInstance->poll();
  cadenceInstance->cadence();

  TEST_MESSAGE("Checking total revolutions after debouncing simulation");

  TEST_ASSERT_EQUAL(1, cadenceInstance->totalRevs());
}

void test_cadence_total_revs() {
  // Simulate exactly 100 pulses and verify totalRevs() increments correctly.

  const unsigned long NUM_PULSES = 100;

  const unsigned long REVOLUTION_INTERVAL_MS = 1000; // 60 RPM for simplicity

  mockSys.setPinState(TEST_PIN, true);

  mockSys.setMillis(0);

  cadenceInstance->poll();
  cadenceInstance->cadence(); // Initialize

  TEST_MESSAGE("Simulating 100 pulses and checking total revolutions.");

  for (unsigned long i = 0; i < NUM_PULSES; ++i) {
    // Advance time for each pulse, ensuring it's outside the debounce threshold

    unsigned long currentPulseTime =
        (i * REVOLUTION_INTERVAL_MS) + REVOLUTION_INTERVAL_MS;

    mockSys.setMillis(currentPulseTime - 50); // Pin HIGH before LOW

    mockSys.setPinState(TEST_PIN, true);

    cadenceInstance->poll();
    cadenceInstance->cadence();

    mockSys.setMillis(currentPulseTime); // Pin LOW (trigger)

    mockSys.setPinState(TEST_PIN, false);

    cadenceInstance->poll();
    cadenceInstance->cadence();

    mockSys.setMillis(currentPulseTime + 10); // Pin HIGH again

    mockSys.setPinState(TEST_PIN, true);

    cadenceInstance->poll();
    cadenceInstance->cadence();
  }

  TEST_MESSAGE("Checking final total revolutions.");

  TEST_ASSERT_EQUAL(NUM_PULSES, cadenceInstance->totalRevs());
}

/**
 * Test 3.1.1: Timer Wraparound
 * Verifies that the cadence calculation handles the 32-bit millis() rollover
 * correctly. This simulates the condition where millis() goes from ~4.29
 * billion back to 0.
 */
void test_cadence_millis_wraparound() {
  // 1. Setup time near the end of 32-bit range (2^32 - 1500 ms)
  // Using explicit uint32_t logic simulation
  const uint32_t MAX_UINT32 = 0xFFFFFFFF;
  uint32_t startMls = MAX_UINT32 - 1500;

  mockSys.setMillis(startMls);
  mockSys.setPinState(TEST_PIN, true);
  cadenceInstance->poll();
  cadenceInstance->cadence(); // Init internal state

  // 2. Pulse 1 at startMls (Pin goes LOW)
  mockSys.setMillis(startMls);
  mockSys.setPinState(TEST_PIN, false);
  cadenceInstance->poll();
  cadenceInstance->cadence();
  mockSys.setPinState(TEST_PIN, true); // Reset to HIGH
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // 3. Pulse 2 after overflow (at time 500)
  // Physical interval: (MAX - start) + 500 + 1 = 1500 + 500 = 2000 ms
  // 2000ms interval = 30 RPM
  uint32_t nextMls = 500;
  mockSys.setMillis(nextMls);

  mockSys.setPinState(TEST_PIN, false); // LOW
  cadenceInstance->poll();
  cadenceInstance->cadence();

  mockSys.setPinState(TEST_PIN, true);
  cadenceInstance->poll();
  cadenceInstance->cadence(); // Trigger calculation

  // 4. Verify
  // Note: Due to integer math in code, 2000ms might be slightly fuzzy,
  // but calculation is 60000 / 2000 = 30.
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 30.0f, cadenceInstance->cadence());
}

/**
 * Test: GATT Timestamp Wraparound
 * Rationale: Verify that 16-bit 1/1024s timestamp wraps correctly.
 */
void test_cadence_gatt_wraparound() {
  mockSys.setMillis(0);
  mockSys.setPinState(TEST_PIN, true);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // Each rev adds time to gattLastCrankRevolutionTimestamp
  // We want to wrap 65535.
  // Time unit is 1/1024s.
  // 64 seconds = 65536 units.

  // 1. Advance 60 seconds (approx 61440 units)
  uint32_t jump = 60000;
  mockSys.setMillis(jump);
  // Trigger pulse
  mockSys.setPinState(TEST_PIN, false);
  cadenceInstance->poll();
  cadenceInstance->cadence();
  mockSys.setPinState(TEST_PIN, true);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  uint16_t ts1 = cadenceInstance->getGattLastCrankRevolutionTimestamp();
  // Expected: ~61440
  TEST_ASSERT_TRUE(ts1 > 50000);

  // 2. Advance 5 more seconds -> total 65s (Wrap should happen)
  mockSys.setMillis(jump + 5000);
  mockSys.setPinState(TEST_PIN, false);
  cadenceInstance->poll();
  cadenceInstance->cadence();
  mockSys.setPinState(TEST_PIN, true);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  uint16_t ts2 = cadenceInstance->getGattLastCrankRevolutionTimestamp();

  // ts2 should be small (< 5000) because it wrapped
  TEST_ASSERT_TRUE_MESSAGE(ts2 < 5000, "GATT timestamp should have wrapped");
  TEST_ASSERT_TRUE_MESSAGE(ts2 < ts1,
                           "Wrapped timestamp should be smaller than previous");
}
/**
 * Test 3.1.2: Coasting Decay Logic (NATURAL DECAY)
 * The previous logic artificially divided RPM by 2, 3, etc. causing sudden
 * drops. We now expect "Natural Decay": RPM = 60000 / CurrentInterval.
 */
void test_cadence_coasting_decay_logic() {
  // 1. Establish steady 60 RPM (1000ms interval)
  mockSys.setMillis(0);
  mockSys.setPinState(TEST_PIN, true);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // Pulse at 1000ms
  mockSys.setMillis(1000);
  mockSys.setPinState(TEST_PIN, false);
  cadenceInstance->poll();
  cadenceInstance->cadence();
  mockSys.setMillis(1350);
  mockSys.setPinState(TEST_PIN, true);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // Pulse at 2000ms
  mockSys.setMillis(2000);
  mockSys.setPinState(TEST_PIN, false);
  cadenceInstance->poll();
  cadenceInstance->cadence();
  mockSys.setMillis(2350);
  mockSys.setPinState(TEST_PIN, true);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  TEST_ASSERT_EQUAL_FLOAT(60.0f, cadenceInstance->cadence());

  // 2. Simulate Coasting (No pulses)

  // Case A: Interval 1500ms (1.5x gap)
  // Formula: 60000 / 1500 = 40 RPM
  mockSys.setMillis(3500);
  TEST_ASSERT_EQUAL_FLOAT(40.0f, cadenceInstance->cadence());

  // Case B: Interval 2100ms (2.1x gap)
  // PREVIOUS BUGGY LOGIC: Would divide by 2 extra -> ~14 RPM
  // NEW LOGIC: 60000 / 2100 = 28.57 RPM
  mockSys.setMillis(4100);
  cadenceInstance->poll();
  float val_2x = cadenceInstance->cadence();
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 28.57f, val_2x);

  // Case C: Interval 3100ms (3.1x gap)
  // PREVIOUS BUGGY LOGIC: Would divide by 3 extra -> ~6 RPM
  // NEW LOGIC: 60000 / 3100 = 19.35 RPM
  mockSys.setMillis(5100);
  cadenceInstance->poll();
  float val_3x = cadenceInstance->cadence();
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 19.35f, val_3x);
}

/**
 * Test 3.1.3: Startup with Magnet Present
 * Ensures that if the device boots with the magnet activating the sensor (LOW),
 * it doesn't immediately register a revolution or produce infinity RPM.
 */
void test_cadence_startup_magnet_present() {
  mockSys.setMillis(0);
  mockSys.setPinState(TEST_PIN, false); // Sensor is active (LOW) at startup

  // Re-init with this state
  if (cadenceInstance)
    delete cadenceInstance;
  cadenceInstance = new Cadence(TEST_PIN, mockSys);

  // First call
  cadenceInstance->poll();
  float initialRpm = cadenceInstance->cadence();
  TEST_ASSERT_EQUAL_FLOAT(0.0f, initialRpm);
  TEST_ASSERT_EQUAL(0, cadenceInstance->totalRevs());

  // Hold state for a while
  mockSys.setMillis(1000);
  cadenceInstance->poll();
  cadenceInstance->cadence();
  TEST_ASSERT_EQUAL_FLOAT(0.0f, cadenceInstance->cadence());

  // Release magnet (High) - Transition LOW -> HIGH
  mockSys.setMillis(1100);
  mockSys.setPinState(TEST_PIN, true);
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // No revolution counted on release (logic counts on FALLING edge usually,
  // or requires a full cycle. The code checks `old == 1 && current == 0` for
  // counting).
  TEST_ASSERT_EQUAL(0, cadenceInstance->totalRevs());

  // Trigger next valid pulse
  mockSys.setMillis(2000);
  mockSys.setPinState(TEST_PIN, false); // LOW (Pulse trigger)
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // Now we should have 1 revolution?
  // Note: First revolution usually establishes the "start" time, so RPM might
  // still be 0 until the *second* pulse defines an interval.
  TEST_ASSERT_EQUAL(1, cadenceInstance->totalRevs());
}
/**
 * Test: Short Interval Rejection (Zero Time Prevention)
 * Rationale: Ensures that pulses arriving instantly (0ms) or too quickly
 * (<330ms) are debounced and ignored, preserving the previous valid RPM state.
 * This prevents processing invalid intervals which could lead to erratic data.
 */
void test_cadence_prevents_zero_time_calculation() {
  // 1. Setup initial state (Time = 1000ms)
  mockSys.setMillis(1000);
  mockSys.setPinState(TEST_PIN, true); // High (Init)
  cadenceInstance->poll();
  cadenceInstance->cadence();

  // 2. Trigger a valid pulse (1 Revolution in 1000ms = 60 RPM)
  // Rev count becomes 1.
  // Interval (1000 - 0) > 330 -> Calculation runs.
  mockSys.setPinState(TEST_PIN, false); // Low (Active)
  cadenceInstance->poll();
  cadenceInstance->cadence();
  mockSys.setPinState(TEST_PIN, true); // High (Reset)
  cadenceInstance->poll();
  cadenceInstance->cadence();

  TEST_ASSERT_EQUAL_FLOAT(60.0f, cadenceInstance->cadence());

  // 3. Trigger IMMEDIATE second pulse (0ms elapsed since last pin change)
  // This simulates a hardware bounce or glitch.
  // Time is STILL 1000ms.
  mockSys.setMillis(1000);
  mockSys.setPinState(TEST_PIN, false);
  cadenceInstance->poll();
  float result = cadenceInstance->cadence();

  // 4. Assert Stability
  // The debounce logic (sampleTime > 330ms) should reject this 0ms pulse.
  // Therefore, the rev count stays at 1.
  // The calculation loop still sees interval=1000ms and rev=1.
  // Result should remain 60 RPM.
  // If it processed the glitch, it might see 2 revs in 1000ms (120 RPM) or
  // crash.
  TEST_ASSERT_EQUAL_FLOAT(60.0f, result);

  // Verify internal state (totalRevs should still be 1, not 2)
  TEST_ASSERT_EQUAL_MESSAGE(1, cadenceInstance->totalRevs(),
                            "0ms pulse should be ignored by debounce logic");
}
int main() {
  // Renamed setup() to main() and changed return type to int

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

  RUN_TEST(test_cadence_fractional_rpm);
  RUN_TEST(test_cadence_millis_wraparound);
  RUN_TEST(test_cadence_gatt_wraparound);
  RUN_TEST(test_cadence_coasting_decay_logic);
  RUN_TEST(test_cadence_startup_magnet_present);
  RUN_TEST(test_cadence_prevents_zero_time_calculation);

  return UNITY_END(); // Return test result
}
