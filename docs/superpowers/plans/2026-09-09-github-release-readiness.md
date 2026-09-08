# CyclingPowerGarmin GitHub Release Readiness Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish the hardware-verified Garmin speed fix from `master` with accurate documentation, automated regression checks, and a reproducible release record.

**Architecture:** Preserve the trainer firmware behavior proven on the Fenix 7 and make the repository describe and continuously verify that exact behavior. Keep release preparation separate from later BLE restructuring and speed calibration so publication does not introduce another hardware-sensitive redesign.

**Tech Stack:** ESP32, PlatformIO, Arduino, NimBLE-Arduino 2.5.x, Unity, GitHub Actions.

**Spec:** `docs/superpowers/plans/2026-09-08-garmin-speed-recovery.md` and `docs/superpowers/plans/2026-09-08-incremental-firmware-refactor.md`

## Global Constraints

- Work directly on `master`, as explicitly requested by the user.
- Preserve the hardware-verified CPS Control Point implementation from commit `c61fbf1`.
- Do not change `src/Power.cpp`, its lookup table, resistance thresholds, pin assignments, BLE packet bytes, or wheel-to-crank ratio `3`.
- Keep `USE_INTERRUPTS == 0`; describe interrupts as experimental and unsupported in production.
- Keep `h2zero/NimBLE-Arduino@^2.5.0`; verified resolution remains 2.5.1.
- Do not add mock firmware or diagnostic PlatformIO environments to production.
- Do not rewrite or squash the existing local history unless the user explicitly requests it.
- Do not push or create a GitHub release without explicit user authorization.

---

### Task 1: Strengthen the Garmin regression contract — [100%]

**Files:**
- Modify: `test/test_cps_control_point/test_cps_control_point.cpp`
- Modify: `test/test_ble_protocol/test_ble_protocol.cpp`
- Verify unchanged: `src/CpsControlPoint.h`
- Verify unchanged: `src/BLECyclingPowerService.cpp`

**Interfaces:**
- Consumes: `CpsControlPoint::write(...)`, `CpsControlPoint::apply(...)`, and `BLECyclingPowerService::updateData(const CyclingTelemetry&)`.
- Produces: executable regression evidence for the characteristic whose absence caused Garmin speed to display `--`.

- [x] **Step 1: Add a unit test proving rejected requests preserve an established offset**

Create a test that first accepts `{0x01, 0x64, 0x00, 0x00, 0x00}` at base wheel count `30`, then submits malformed opcode `0x01` and unsupported opcode `0x03`. Assert both failures leave `apply(33) == 103`.

- [x] **Step 2: Temporarily remove the production CPS Control Point wiring and verify the protocol test fails**

Use a reversible local patch that removes creation of characteristic `2A66`, then run:

```bash
pio test -e native -f test_ble_protocol
```

Expected: `test_cps_control_point_sets_cumulative_wheel_value` fails because characteristic `2A66` is absent. Restore the production code immediately afterward; do not commit the temporary mutation.

- [x] **Step 3: Run focused Control Point tests**

```bash
pio test -e native -f test_cps_control_point -f test_ble_protocol
```

Expected: all Control Point and BLE protocol tests pass, including exact properties `WRITE | INDICATE`, response `20 01 01`, and subsequent CPS wheel count `103`.

- [x] **Step 4: Commit the regression-test improvement**

```bash
git add test/test_cps_control_point/test_cps_control_point.cpp
git commit -m "test: preserve Garmin speed compatibility contract"
```

### Task 2: Correct the public documentation — [100%]

**Files:**
- Modify: `README.md`
- Modify: `docs/superpowers/plans/2026-09-08-garmin-speed-recovery.md`
- Modify: `agents.md`

**Interfaces:**
- Consumes: hardware evidence from Fenix 7 firmware 26.09 and commit `c61fbf1`.
- Produces: accurate build, pairing, compatibility, and troubleshooting instructions for GitHub users.

- [x] **Step 1: Replace the obsolete Garmin compatibility section**

Document these exact findings:

- CPS wheel and crank data without CPS Control Point produced power and cadence but speed `--`.
- Adding CPS Control Point characteristic `2A66` restored live speed, saved speed, distance, and stop/run behavior.
- The fix was reproduced by fresh pairing and verified on the real trainer with Fenix 7 firmware 26.09.
- CSC remains exposed for compatible receivers, but Garmin may classify the combined device as one power meter.

- [x] **Step 2: Document the Bluetooth requirement precisely**

Explain that when CPS Wheel Revolution Data is supported, the CPS Set Cumulative Value procedure is conditionally required. Avoid claiming knowledge of Garmin’s undocumented internal validation logic; state only that controlled A/B testing isolated `2A66` as the compatibility difference.

- [x] **Step 3: Correct production-mode claims**

State that polling is the only hardware-validated production mode. Interrupt code remains experimental, is disabled with `USE_INTERRUPTS == 0`, and should not be enabled without separate trainer validation.

- [x] **Step 4: Add a concise installation and pairing checklist**

Include:

```bash
pio test -e native
pio run -e lolin32_lite
pio run -e lolin32_lite -t upload
```

Then instruct users to remove and re-add `CX6` after a firmware upgrade that changes the GATT layout, configure a 2100 mm circumference if appropriate, and test trainer movement with USB disconnected.

- [x] **Step 5: Document the virtual-speed formula and limitations**

Record `speed_kmh = cadence_rpm × 3 × circumference_m × 0.06`, giving approximately 22.68 km/h at 60 RPM and 2100 mm. Clarify that this is virtual speed intended primarily for recording and auto-pause; exact agreement with the trainer requires later calibration.

- [x] **Step 6: Commit documentation**

```bash
git add README.md agents.md docs/superpowers/plans/2026-09-08-garmin-speed-recovery.md
git commit -m "docs: publish Garmin speed recovery setup"
```

### Task 3: Add continuous integration — [100%]

**Files:**
- Create: `.github/workflows/platformio.yml`

**Interfaces:**
- Consumes: PlatformIO environments `native` and `lolin32_lite` from `platformio.ini`.
- Produces: required automated native-test and firmware-build checks for pushes and pull requests.

- [x] **Step 1: Add the GitHub Actions workflow**

Create a workflow named `PlatformIO` triggered by `push` and `pull_request`. Use `actions/checkout@v4`, `actions/setup-python@v5` with Python 3.12, cache `~/.platformio`, install PlatformIO with `python -m pip install --upgrade platformio`, then run:

```bash
pio test -e native
pio run -e lolin32_lite
pio check -e lolin32_lite --skip-packages
```

- [x] **Step 2: Validate workflow syntax locally**

Parse `.github/workflows/platformio.yml` with the available YAML parser. Assert the top-level keys include `name`, `on`, and `jobs`, and that the job contains all three PlatformIO commands.

- [x] **Step 3: Re-run the same commands locally**

```bash
pio test -e native
pio run -e lolin32_lite
pio check -e lolin32_lite --skip-packages
```

Expected: 93 or more native tests pass, firmware builds without warnings, and static analysis reports zero high/medium findings.

- [x] **Step 4: Commit CI**

```bash
git add .github/workflows/platformio.yml
git commit -m "ci: verify native tests and ESP32 firmware"
```

### Task 4: Prepare the release record — [100%]

**Files:**
- Create: `CHANGELOG.md`
- Modify: `.gitignore`

**Interfaces:**
- Consumes: verified commits and hardware results.
- Produces: a user-facing release summary and a clean source tree.

- [x] **Step 1: Add an Unreleased changelog entry**

Under `## [Unreleased]`, record:

- Restored Garmin speed by adding conforming CPS Control Point support.
- Added combined CPS/CSC publication from one telemetry snapshot.
- Preserved fractional cadence in power calculation.
- Added typed cadence and resistance state models while retaining polling.
- Added packet, rollover, resistance, Control Point, and integration tests.

- [x] **Step 2: Ignore local worktree storage**

Add `/.worktrees/` to `.gitignore` so future isolated worktrees do not appear as untracked repository content. Do not remove or modify any existing external diagnostic worktrees in `/tmp`.

- [x] **Step 3: Audit tracked content for publication hazards**

Run:

```bash
git ls-files
git grep -nE '(BEGIN (RSA|OPENSSH|EC) PRIVATE KEY|api[_-]?key|password|secret|token)' -- ':!docs/superpowers/plans/*'
git diff --check
git status --short
```

Expected: no credentials or generated build artifacts are tracked; only intentional release-preparation files are modified before commit.

- [x] **Step 4: Commit release metadata**

```bash
git add CHANGELOG.md .gitignore
git commit -m "chore: prepare public firmware release"
```

### Task 5: Final release gate — [100%]

**Files:** No source changes expected.

**Interfaces:**
- Consumes: Tasks 1–4 and the completed trainer hardware acceptance.
- Produces: evidence that local `master` is ready to push.

- [x] **Step 1: Verify dependency resolution**

```bash
pio pkg list -e lolin32_lite
```

Expected: `NimBLE-Arduino @ 2.5.1` resolved from `h2zero/NimBLE-Arduino@^2.5.0`.

- [x] **Step 2: Run complete software verification**

```bash
pio test -e native
pio run -e lolin32_lite
pio check -e lolin32_lite --skip-packages
git diff --check
```

Expected: all native tests pass, the ESP32 release build succeeds without warnings, and static analysis reports zero high/medium findings.

- [x] **Step 3: Confirm immutable behavior-sensitive files**

Compare `src/Power.cpp`, `src/config.h`, packet generators, and resistance thresholds against commit `c61fbf1`. Expected changes are documentation or tests only; no power-table, pin, ratio, timing, packet, or resistance changes are permitted.

- [x] **Step 4: Confirm repository state and history**

```bash
git status --short --branch
git log --oneline --decorate origin/master..master
```

Expected: clean `master`, containing the hardware-verified fix and release-preparation commits, with no mock firmware tracked.

- [x] **Step 5: Stop for publication authorization**

Report the exact commits, verification totals, and hardware evidence. Do not run `git push`, create a tag, or create a GitHub release until the user explicitly requests publication.

## Deferred follow-up plans

These are intentionally outside this release gate:

- Split CPS, CSC, and Battery implementation behind the existing coordinator.
- Replace `_deviceConnected` with a saturating connection count and test two-client behavior.
- Implement full Control Point procedure-state and CCCD error handling if required by additional receivers.
- Compile resistance diagnostics completely out of non-diagnostic builds.
- Calibrate virtual speed against trainer measurements using fixed-point or piecewise-linear mapping.

None of these deferred items is required to publish the currently hardware-verified Garmin fix.
