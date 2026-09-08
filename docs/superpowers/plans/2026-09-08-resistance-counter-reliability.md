# Resistance Counter Reliability Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Determine why occasional full-range resistance movements are undercounted, eliminate direction-state count loss, and decide from hardware evidence whether polling is sufficient.

**Architecture:** Keep pin reads and edge detection in the polling adapter, move counter decisions into a pure reducer, and collect bounded run-level diagnostics without logging inside the fast polling path. Do not add interrupts or ESP32 PCNT unless the diagnostics prove polling misses edges and the user approves a separate follow-up.

**Tech Stack:** ESP32, PlatformIO, Arduino, Unity, polling GPIO input.

**Spec:** `docs/superpowers/plans/2026-09-08-incremental-firmware-refactor.md`

## Global Constraints

- Continue from commit `2d88d9a` on `codex/incremental-firmware-refactor`.
- Keep `USE_INTERRUPTS == 0`; do not modify or enable the interrupt path.
- Do not modify `Power.cpp`, its lookup table, resistance thresholds, pin assignments, BLE services, or cadence behavior.
- Do not print from the per-poll or per-edge hot path.
- Use fixed-size state only; no allocation, exceptions, RTTI, or `std::function`.
- Stop at every hardware gate and wait for the user result.

---

### Task 1: Add bounded resistance-run diagnostics — [100%]

**Files:**
- Modify: `src/ResistanceLevel.h`
- Modify: `src/ResistanceLevel.cpp`
- Modify: `src/BikeComputer.h`
- Test: `test/test_resistance_diagnostics/test_resistance_diagnostics.cpp`

**Interfaces:**

```cpp
enum class ResistanceDirection : uint8_t {
  Stopped,
  Forward,
  Backward,
  Invalid,
};

struct ResistanceRunDiagnostics {
  ResistanceDirection direction;
  uint16_t startCounter;
  uint16_t endCounter;
  uint32_t observedEdges;
  uint32_t invalidDirectionSamples;
  uint32_t stoppedSamplesDuringRun;
  uint32_t maxPollGapMs;
  uint32_t minimumAcceptedEdgeIntervalMs;
};

auto classifyResistanceDirection(bool forward, bool backward)
    -> ResistanceDirection;

class ResistanceLevel {
public:
  auto takeCompletedRunDiagnostics(ResistanceRunDiagnostics& output) -> bool;
};
```

- [x] **Step 1: Write failing tests**

Assert all four direction classifications. Simulate a forward run with known poll gaps, accepted edges, one invalid sample, and one transient stopped sample; after 100 ms of continuous `Stopped`, assert one completed report with exact counters and metrics. Assert taking it a second time returns `false`.

- [x] **Step 2: Verify RED**

```bash
pio test -e native -f test_resistance_diagnostics
```

Expected: compilation fails because the diagnostics interfaces do not exist.

- [x] **Step 3: Implement fixed-size diagnostics**

Classify direction from the two pins. Start a run on `Forward` or `Backward`; accumulate metrics in memory; finalize only after direction remains `Stopped` for 100 ms. Retain the report until `takeCompletedRunDiagnostics` copies and clears it. Never change counting behavior in this task.

- [x] **Step 4: Print one summary after each completed run**

In `BikeComputer::update()`, print only after `takeCompletedRunDiagnostics` returns true:

```text
RESISTANCE RUN: dir=F start=0 end=679 edges=679 invalid=0 stopped=0 maxPollGapMs=4 minEdgeMs=12
```

- [x] **Step 5: Verify and commit**

```bash
pio test -e native
pio run -e lolin32_lite
pio check -e lolin32_lite --skip-packages
git diff --check
git add src/ResistanceLevel.h src/ResistanceLevel.cpp src/BikeComputer.h test/test_resistance_diagnostics/test_resistance_diagnostics.cpp
git commit -m "diagnostic: measure resistance polling reliability"
```

### Hardware Gate A: Locate the count loss — [100%]

Upload and perform at least five complete minimum-to-maximum-to-minimum movements while Garmin is connected. Preserve every `RESISTANCE RUN` line.

Classify the result:

- `invalid > 0` or `stopped > 0` during an undercounted run: direction instability is confirmed.
- `maxPollGapMs >= minimumAcceptedEdgeIntervalMs` during an undercounted run: polling can miss edges and is the leading cause.
- Neither condition: repeat once with Garmin disconnected. If only connected runs fail, BLE-induced loop latency is confirmed; otherwise stop and capture the position/direction pins with a logic analyzer before changing software.

Do not continue until one category has evidence.

Evidence captured on hardware: full and short runs had a 1 ms maximum poll gap
with 23--27 ms minimum accepted edge intervals, so polling had ample margin.
Direction pins reported no invalid or transient stopped samples. One-step
backward commands intentionally consisted of a backward overshoot followed by
a short forward compensation run. USB attachment materially disturbed trainer
behavior, so further acceptance is by normal training with USB disconnected.

### Task 2: Extract and correct the pure resistance reducer — [100%]

**Files:**
- Create: `src/ResistanceModel.h`
- Modify: `src/ResistanceLevel.h`
- Modify: `src/ResistanceLevel.cpp`
- Test: `test/test_resistance_model/test_resistance_model.cpp`

**Interfaces:**

```cpp
struct ResistanceModelState {
  uint16_t counter;
  uint8_t level;
  uint32_t lastAcceptedEdgeTimeMs;
  ResistanceDirection lastValidDirection;
};

struct ResistanceEvent {
  uint32_t timestampMs;
  ResistanceDirection direction;
  bool positionEdge;
  bool minimumLimitActive;
};

auto reduceResistance(const ResistanceModelState& state,
                      const ResistanceEvent& event)
    -> ResistanceModelState;
```

- [x] **Step 1: Write characterization tests**

Cover every level minimum and maximum, every gap between ranges, the 8/50/1700 ms timing boundaries, forward/backward changes, minimum-limit reset, `millis()` rollover, and current `uint16_t` overflow behavior.

- [x] **Step 2: Write the failing direction-dropout regression**

Apply a valid forward edge, an `Invalid` edge, then another valid forward edge at consistent intervals. Expect both forward edges to count. Repeat with `Stopped`. The current implementation must fail because it overwrites its remembered direction on invalid/stopped events.

- [x] **Step 3: Verify RED**

```bash
pio test -e native -f test_resistance_model
```

- [x] **Step 4: Implement the reducer**

Move debounce, direction hysteresis, counter changes, level lookup, gap latching, and limit reset into the pure function. Update `lastValidDirection` only for `Forward` or `Backward`; `Stopped` and `Invalid` cannot modify the counter or valid-direction history.

- [x] **Step 5: Make polling a thin adapter**

`ResistanceLevel::poll()` reads pins, detects the position edge, constructs one `ResistanceEvent`, applies `reduceResistance`, and updates diagnostics. Preserve the existing public `level()` and `getPositionChangeCounter()` methods.

- [x] **Step 6: Verify and commit**

```bash
pio test -e native
pio run -e lolin32_lite
pio check -e lolin32_lite --skip-packages
git diff --check
git add src/ResistanceModel.h src/ResistanceLevel.h src/ResistanceLevel.cpp test/test_resistance_model/test_resistance_model.cpp test/test_resistance/test_resistance.cpp
git commit -m "refactor: isolate resistance state transitions"
```

### Hardware Gate B: Validate the reducer — [0%]

Repeat five full movements with Garmin connected. Accept only if:

- Minimum reset still produces counter `0` and level `1`.
- Forward and backward level transitions match the existing thresholds.
- No new jumps or oscillation appear.
- Direction-dropout runs no longer lose the next valid pulse.
- Cadence, power, BLE reconnection, sleep, and wake remain unchanged.

If undercount remains and diagnostics prove long poll gaps, stop. Do not adjust thresholds or debounce values.

The user elected to validate this gate during normal training with USB
disconnected and will return if resistance behavior becomes materially worse.

### Task 3: Close the evidence loop — [0%]

**Files:**
- Modify: `docs/superpowers/plans/2026-09-08-incremental-firmware-refactor.md`
- Modify: `agents.md`

- [ ] **Step 1: Record confirmed hardware results**

Document the failing and successful run summaries, confirmed root cause category, Garmin connection state, and whether the reducer eliminated any count loss.

- [ ] **Step 2: Choose the bounded outcome from evidence**

- If five connected full-range cycles pass, retain polling and mark resistance refactoring complete.
- If poll gaps overlap edge intervals and undercount persists, create a separate ESP32 PCNT feasibility plan. The PCNT plan must prototype hardware counting on a spare branch and include its own trainer gate; do not enable GPIO ISRs.
- If the electrical signals are unstable, stop software changes and document the required hardware investigation.

- [ ] **Step 3: Final verification and commit**

```bash
pio pkg list -e lolin32_lite
pio test -e native
pio run -e lolin32_lite
pio check -e lolin32_lite --skip-packages
git diff --check
git status --short
git add agents.md docs/superpowers/plans/2026-09-08-incremental-firmware-refactor.md docs/superpowers/plans/2026-09-08-resistance-counter-reliability.md
git commit -m "docs: record resistance counting evidence"
```

## Acceptance Criteria

- The level-range table and power table remain unchanged.
- Diagnostics identify direction instability, polling latency, or the need for electrical capture without adding hot-path serial output.
- `Stopped` and `Invalid` direction samples cannot corrupt the last valid direction.
- The reducer is pure, fixed-size, rollover-safe, and exhaustively tested at all range/timing boundaries.
- Five consecutive connected full-range hardware cycles pass, or the work stops with evidence for a separate PCNT/hardware investigation.
