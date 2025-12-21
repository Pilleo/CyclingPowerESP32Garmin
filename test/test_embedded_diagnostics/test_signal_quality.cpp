#include <Arduino.h>
#include <unity.h>
#include <vector>
#include <numeric>
#include <algorithm>

// ==========================================
// CONFIGURATION
// ==========================================
// Pins to test (Match your cycling_power_garmin.cpp)
const uint8_t PIN_CADENCE = 15;
const uint8_t PIN_RES_POS = 23;

// How long to record while you spin (seconds)
const uint32_t RECORDING_TIME_MS = 5000; 

// Buffer to store timestamps (Max edges to record per test)
const size_t MAX_SAMPLES = 2000; 

// ==========================================
// DATA CAPTURE
// ==========================================
volatile uint32_t edgeTimestamps[MAX_SAMPLES];
volatile size_t edgeCount = 0;

// Raw ISR: Captures every single transition (bounce included)
void IRAM_ATTR rawSignalISR() {
    if (edgeCount < MAX_SAMPLES) {
        edgeTimestamps[edgeCount++] = micros();
    }
}

// Helper to reset and prepare for a new test
void start_capture(uint8_t pin) {
    edgeCount = 0;
    // Reset buffer
    memset((void*)edgeTimestamps, 0, sizeof(edgeTimestamps));
    
    // Setup Pin
    pinMode(pin, INPUT_PULLUP);
    
    // Attach Raw Interrupt (CHANGE to catch both Press and Release bounces)
    attachInterrupt(digitalPinToInterrupt(pin), rawSignalISR, CHANGE);
}

void stop_capture(uint8_t pin) {
    detachInterrupt(digitalPinToInterrupt(pin));
}

// ==========================================
// ANALYSIS LOGIC
// ==========================================
struct SignalStats {
    uint32_t minStableInterval; // Shortest valid pulse
    uint32_t maxBounceDuration; // Longest noise burst
    uint32_t totalEdges;
    uint32_t validPulses;
};

void analyze_and_print(const char* sensorName) {
    if (edgeCount < 2) {
        UnityPrint("  -> NO DATA: Did you spin the ");
        UnityPrint(sensorName);
        UnityPrint("?");
        UNITY_PRINT_EOL();
        return;
    }

    std::vector<uint32_t> deltas;
    deltas.reserve(edgeCount);

    // 1. Calculate Deltas (Time between edges)
    for (size_t i = 1; i < edgeCount; i++) {
        uint32_t diff = edgeTimestamps[i] - edgeTimestamps[i-1];
        deltas.push_back(diff);
    }

    uint32_t maxBounce = 0;
    uint32_t minStable = 99999999;
    
    // 2. Classify "Bounce" vs "Signal"
    // Assumption: Anything < 50ms (50,000us) is suspicious for a human cadence/knob.
    // However, for Cadence @ 120RPM, a half-rotation is ~250ms.
    // For Resistance, fast turns might be 20-30ms.
    // Let's define "Bounce" as high-freq chatter < 10ms (10,000us).
    
    const uint32_t BOUNCE_THRESHOLD_US = 15000; // 15ms

    for (uint32_t d : deltas) {
        if (d < BOUNCE_THRESHOLD_US) {
            if (d > maxBounce) maxBounce = d;
        } else {
            if (d < minStable) minStable = d;
        }
    }

    // 3. Report
    UnityPrint("-----------------------------------------"); UNITY_PRINT_EOL();
    UnityPrint("ANALYSIS FOR: "); UnityPrint(sensorName); UNITY_PRINT_EOL();
    UnityPrint("  Total Raw Edges: "); UnityPrintNumber(edgeCount); UNITY_PRINT_EOL();
    
    UnityPrint("  Max Bounce Duration: "); 
    UnityPrintNumber(maxBounce / 1000); 
    UnityPrint(" ms (Raw: "); UnityPrintNumber(maxBounce); UnityPrint(" us)");
    UNITY_PRINT_EOL();

    UnityPrint("  Min Stable Interval: "); 
    UnityPrintNumber(minStable / 1000); 
    UnityPrint(" ms");
    UNITY_PRINT_EOL();

    // 4. Recommendation
    UnityPrint("  RECOMMENDATION: ");
    if (maxBounce == 0) {
        UnityPrint("Signal is clean! Debounce 5-10ms is safe.");
    } else {
        uint32_t safeDebounce = (maxBounce / 1000) + 5; // Add 5ms buffer
        UnityPrint("Set DEBOUNCE_TIME_MS to at least ");
        UnityPrintNumber(safeDebounce);
        UnityPrint(" ms.");
    }
    UNITY_PRINT_EOL();
    UnityPrint("-----------------------------------------"); UNITY_PRINT_EOL();
}

// ==========================================
// TESTS
// ==========================================

void test_cadence_signal_quality() {
    UnityPrint(">>> PREPARE TO SPIN PEDALS <<<"); UNITY_PRINT_EOL();
    UnityPrint("Recording Cadence Pin (15) for 5 seconds..."); UNITY_PRINT_EOL();
    
    start_capture(PIN_CADENCE);
    delay(RECORDING_TIME_MS);
    stop_capture(PIN_CADENCE);
    
    analyze_and_print("CADENCE SENSOR");
}

void test_resistance_signal_quality() {
    UnityPrint(">>> PREPARE TO TURN RESISTANCE KNOB <<<"); UNITY_PRINT_EOL();
    UnityPrint("Recording Position Pin (23) for 5 seconds..."); UNITY_PRINT_EOL();
    
    start_capture(PIN_RES_POS);
    delay(RECORDING_TIME_MS);
    stop_capture(PIN_RES_POS);
    
    analyze_and_print("RESISTANCE POSITION");
}

void setup() {
    delay(2000); // Wait for Serial to connect
    UNITY_BEGIN();
    RUN_TEST(test_cadence_signal_quality);
    RUN_TEST(test_resistance_signal_quality);
    UNITY_END();
}

void loop() {}