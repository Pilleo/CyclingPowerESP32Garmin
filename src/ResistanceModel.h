#ifndef RESISTANCE_MODEL_H
#define RESISTANCE_MODEL_H

#include <cstdint>

enum class ResistanceDirection : uint8_t {
  Stopped,
  Forward,
  Backward,
  Invalid
};

struct ResistanceModelState {
  uint16_t counter = 0;
  uint8_t level = 1;
  uint32_t lastAcceptedEdgeTimeMs = 0;
  ResistanceDirection lastValidDirection = ResistanceDirection::Forward;
};

struct ResistanceEvent {
  uint32_t timestampMs;
  ResistanceDirection direction;
  bool positionEdge;
  bool minimumLimitActive;
};

namespace resistance_model_detail {
struct LevelRange {
  uint16_t minimum;
  uint16_t maximum;
  uint8_t level;
};

constexpr LevelRange LEVEL_RANGES[] = {
    {0, 29, 1},      {170, 190, 2}, {281, 308, 3}, {360, 374, 4},
    {411, 429, 5},   {461, 476, 6}, {501, 519, 7}, {531, 549, 8},
    {561, 575, 9},   {591, 601, 10},{611, 627, 11},{631, 641, 12},
    {653, 661, 13},  {666, 679, 14},{681, 690, 15},{692, 65535, 16}};

constexpr uint32_t DEBOUNCE_TIME_MS = 8;
constexpr uint32_t MOVEMENT_TIMEOUT_MS = 50;
constexpr uint32_t PAUSE_TIMEOUT_MS = 1700;

inline auto levelForCounter(const uint16_t counter,
                            const uint8_t previousLevel) -> uint8_t {
  for (const auto &range : LEVEL_RANGES) {
    if (counter >= range.minimum && counter <= range.maximum) {
      return range.level;
    }
  }
  return previousLevel;
}
} // namespace resistance_model_detail

inline auto reduceResistance(const ResistanceModelState &current,
                             const ResistanceEvent &event)
    -> ResistanceModelState {
  using namespace resistance_model_detail;
  ResistanceModelState next = current;

  if (event.minimumLimitActive &&
      event.direction != ResistanceDirection::Forward) {
    next.counter = 0;
    next.level = 1;
  }
  if (!event.positionEdge) {
    return next;
  }

  const uint32_t sampleTime =
      event.timestampMs - current.lastAcceptedEdgeTimeMs;
  if (sampleTime <= DEBOUNCE_TIME_MS) {
    return next;
  }
  next.lastAcceptedEdgeTimeMs = event.timestampMs;

  const bool consistent = sampleTime < MOVEMENT_TIMEOUT_MS;
  const bool longPause = sampleTime > PAUSE_TIMEOUT_MS;
  if (event.direction == ResistanceDirection::Forward) {
    if ((consistent && current.lastValidDirection ==
                           ResistanceDirection::Forward) ||
        longPause) {
      ++next.counter;
    }
    next.lastValidDirection = ResistanceDirection::Forward;
  } else if (event.direction == ResistanceDirection::Backward) {
    if (next.counter > 0 &&
        (longPause ||
         (consistent && current.lastValidDirection ==
                            ResistanceDirection::Backward))) {
      --next.counter;
    }
    next.lastValidDirection = ResistanceDirection::Backward;
  }

  next.level = levelForCounter(next.counter, next.level);
  return next;
}

#endif // RESISTANCE_MODEL_H
