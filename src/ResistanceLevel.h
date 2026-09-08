#ifndef RESISTANCE_LEVEL_H
#define RESISTANCE_LEVEL_H

#include "SystemWrapper.h"
#include <cstdint>
#ifdef ESP32
#include "esp_attr.h"
#define ISR_ATTR IRAM_ATTR
#else
#define ISR_ATTR
#endif

struct ResistanceLevelPins {
  const uint8_t backwards;
  const uint8_t limit;
  const uint8_t position;
  const uint8_t forwards;
};

enum class ResistanceDirection : uint8_t {
  Stopped,
  Forward,
  Backward,
  Invalid
};

auto classifyResistanceDirection(bool forwardActive,
                                 bool backwardActive) -> ResistanceDirection;

struct ResistanceRunDiagnostics {
  ResistanceDirection direction = ResistanceDirection::Stopped;
  uint16_t startCounter = 0;
  uint16_t endCounter = 0;
  uint32_t observedEdges = 0;
  uint32_t invalidDirectionSamples = 0;
  uint32_t stoppedSamplesDuringRun = 0;
  uint32_t maxPollGapMs = 0;
  uint32_t minimumAcceptedEdgeIntervalMs = 0;
};

class ResistanceLevel {
public:
  explicit ResistanceLevel(const ResistanceLevelPins &pins,
                           ISystemWrapper &sys) noexcept;

  void begin(); // Initialize hardware state

  auto getLevel() const -> uint8_t;

  auto getPositionChangeCounter() const -> uint16_t;

  auto level() const -> uint8_t; // // Wrapper returns level
  /**
   * @brief Polling Driver.
   * Reads all resistance pins and triggers events if edges are detected.
   */
  void poll();

  auto takeCompletedRunDiagnostics(ResistanceRunDiagnostics &diagnostics)
      -> bool;

  /**
   * @brief Triggered when the position sensor detects an edge (pulse).
   * Handles debouncing, direction logic, and counter updates.
   *
   * @param timestampMs Current time in milliseconds.
   * @param isMovingForward True if the Forward pin signals movement.
   * @param isMovingBack True if the Backward pin signals movement.
   */
  void onPositionPulse(uint32_t timestampMs, bool isMovingForward,
                       bool isMovingBack) ISR_ATTR;

  /**
   * @brief Triggered when the limit switch is active and movement is not
   * forward. Resets calibration.
   */
  void onLimitReset() ISR_ATTR;

  // NEW: ISR Entry Points
  void enableInterrupt();

  void handlePositionInterrupt() ISR_ATTR;

  static void isrPosition() ISR_ATTR;

  // NEW: ISR for Limit Switch
  void handleLimitInterrupt() ISR_ATTR;

  static void isrLimit() ISR_ATTR;

  void update();

private:
  ISystemWrapper &sys;
  const uint8_t limitPin;
  const uint8_t backwardsPin;
  const uint8_t positionPin;
  const uint8_t forwardsPin;

  volatile bool oldPositionState;

  // Volatile: Accessed by future ISRs
  volatile uint8_t currentLevel;
  volatile uint32_t lastPulseTimestamp;
  volatile bool prevDirectionWasForward;
  volatile uint16_t positionChangeCounter;
  volatile bool _limitInterruptFlag = false;
  volatile bool _positionInterruptFlag = false;
  volatile uint32_t _lastPositionInterruptTime = 0;

  bool _runActive = false;
  bool _completedRunAvailable = false;
  bool _stopping = false;
  bool _hasPollTime = false;
  bool _hasObservedEdgeTime = false;
  uint32_t _lastPollTimeMs = 0;
  uint32_t _stopStartedTimeMs = 0;
  uint32_t _pendingStoppedSamples = 0;
  uint32_t _lastObservedEdgeTimeMs = 0;
  ResistanceRunDiagnostics _activeRun{};
  ResistanceRunDiagnostics _completedRun{};

  void beginDiagnosticRun(ResistanceDirection direction);
  void updateRunDiagnostics(ResistanceDirection direction, uint32_t nowMs,
                            uint32_t pollGapMs);
  void recordObservedEdge(uint32_t nowMs);
  void completeDiagnosticRun();

  void ISR_ATTR updateLevelFromCounter();

  static constexpr uint8_t MIN_LEVEL = 1;
  static constexpr unsigned long DEBOUNCE_TIME_MS = 8;
  static constexpr unsigned long MOVEMENT_TIMEOUT_MS = 50;
  static constexpr unsigned long PAUSE_TIMEOUT_MS = 1700;
  static constexpr unsigned long RUN_STOP_CONFIRMATION_MS = 100;
  // NEW: Static pointer
  static ResistanceLevel *_instance;
};

#endif // RESISTANCE_LEVEL_H
