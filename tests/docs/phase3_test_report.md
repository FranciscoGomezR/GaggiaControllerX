# Phase 3 Test Report

**Date:** 2026-04-14  
**Framework:** Ceedling 1.0.1 + CMock (auto-generated mocks)  
**Toolchain:** gcc (MinGW-W64) 15.2.0 · Ruby 3.2.11 · GNU Make 4.4.1  
**Build command:** `ceedling test:all` (run from `tests_ceedling/`)  
**Config file:** `tests_ceedling/project.yml`  
**Project root:** `C:\WS\NRF\GaggiaController`  
**Related report:** [phase2_3_test_report.md](phase2_3_test_report.md)

---

## Overview

Phase 3 migrates the Phase 1 and Phase 2/3 tests to the **Ceedling + CMock** build
system. The production code is unchanged. The goals are:

| Goal | Achieved |
|------|----------|
| Auto-generated mocks from header files — no manual `FAKE_*_FUNC()` declarations | Yes |
| Strict call-ordering via `_Expect()` — out-of-order calls fail immediately | Yes |
| Unexpected call detection — any unmocked call is an instant failure | Yes |
| Automatic test-runner discovery — no hand-maintained `main()` with `RUN_TEST()` | Yes |
| Built-in gcov coverage target (`ceedling gcov:all`) | Yes |

Both `tests/` (Makefile + FFF) and `tests_ceedling/` (Ceedling + CMock) coexist.
The Makefile suite is the fast sanity check; Ceedling is the authoritative gate.

---

## Test Results

### Overall summary

```
TESTED:  36
PASSED:  36
FAILED:   0
IGNORED:  0
```

### Per-file results

| Test file | Module under test | Tests | Passed | Failed | Mock used |
|-----------|------------------|-------|--------|--------|-----------|
| `test_ProfileValidator.c` | `ProfileValidator.c` | 14 | 14 | 0 | None (pure logic) |
| `test_x04_Numbers.c` | `x04_Numbers.c` | 15 | 15 | 0 | None (pure logic) |
| `test_PumpController.c` | `PumpController.c` | 7 | 7 | 0 | `mock_solidStateRelay_Controller` |
| **Total** | | **36** | **36** | **0** | |

---

## `test_ProfileValidator.c` — H2 BLE Validation (14 tests)

**Source under test:** `ble_espresso_app/components/Application/ProfileValidator.c`  
**Mocks:** None — pure logic, no hardware dependencies.

This file is a direct port of `tests/test_profile_validator.c`. The only Ceedling
differences are: no `main()` function (auto-discovered by the runner generator),
and `UNITY_INCLUDE_FLOAT` enabled via `project.yml :unity: :defines:`.

| # | Test name | Assertion | Result |
|---|-----------|-----------|--------|
| 1 | `test_H2_SingleField_InRange_Valid` | In-range value: returns `true`, value unchanged | PASS |
| 2 | `test_H2_SingleField_OutOfRange_Clamped` | Out-of-range value: returns `false`, clamped to default | PASS |
| 3 | `test_H2_SingleField_AtLowerBoundary_Valid` | Exact lower boundary accepted | PASS |
| 4 | `test_H2_SingleField_AtUpperBoundary_Valid` | Exact upper boundary accepted | PASS |
| 5 | `test_H2_ValidProfile_ReturnsValid` | All fields valid → `PROFILE_VALID`, no change | PASS |
| 6 | `test_H2_TempTarget_NaN_ClampedToDefault` | NaN → 93.0 °C, `PROFILE_CLAMPED` | PASS |
| 7 | `test_H2_TempTarget_Inf_ClampedToDefault` | Inf → 93.0 °C, `PROFILE_CLAMPED` | PASS |
| 8 | `test_H2_TempTarget_Negative_ClampedToDefault` | −50 °C → 93.0 °C | PASS |
| 9 | `test_H2_TempTarget_TooHigh_ClampedToDefault` | 200 °C → 93.0 °C | PASS |
| 10 | `test_H2_PidPterm_Negative_Clamped` | −1.0 → 9.5 (default Kp) | PASS |
| 11 | `test_H2_PidIterm_TooHigh_Clamped` | 50.0 → 0.3 (default Ki) | PASS |
| 12 | `test_H2_ProfTimerZero_IsValid` | 0.0 s pre-infuse time is valid | PASS |
| 13 | `test_H2_ProfDeclineTmr_Negative_Clamped` | −5 s → 10.0 s default | PASS |
| 14 | `test_H2_MultipleFieldsInvalid_AllClamped` | Three bad fields all clamped; valid field unchanged | PASS |

---

## `test_x04_Numbers.c` — Math Utility Functions (15 tests)

**Source under test:** `ble_espresso_app/components/Utilities/x04_Numbers.c`  
**Mocks:** None — pure arithmetic, no hardware dependencies.

This is a Ceedling port of the Phase 1 `tests/test_numbers.c`. Test `test_Constrain_ValueBelowLower`
verifies the Phase 1 BUG-001 fix is still present (lower-branch of `fcn_Constrain_WithinFloats`
returns `NEGATIVE_SATURATION`).

| # | Test name | Assertion | Result |
|---|-----------|-----------|--------|
| 1 | `test_Constrain_ValueWithinLimits` | No saturation, value unchanged | PASS |
| 2 | `test_Constrain_ValueAboveUpper` | Clamped to upper, returns `POSITIVE_SATURATION` | PASS |
| 3 | `test_Constrain_ValueBelowLower` | Clamped to lower, returns `NEGATIVE_SATURATION` *(BUG-001 fix)* | PASS |
| 4 | `test_Constrain_ValueAtUpperBoundary` | Boundary = no saturation | PASS |
| 5 | `test_Constrain_ValueAtLowerBoundary` | Boundary = no saturation | PASS |
| 6 | `test_ChrArrayToFloat_93_5` | `{0x30,0x39,0x33,0x35}` → 93.5 | PASS |
| 7 | `test_ChrArrayToFloat_Zero` | `{0x30,0x30,0x30,0x30}` → 0.0 | PASS |
| 8 | `test_ChrArrayToFloat_130_0` | `{0x31,0x33,0x30,0x30}` → 130.0 | PASS |
| 9 | `test_FloatToChrArray_RoundTrip_93_5` | Encode 93.5 → decode → 93.5 | PASS |
| 10 | `test_FloatToChrArray_RoundTrip_Zero` | Encode 0.0 → decode → 0.0 | PASS |
| 11 | `test_Hysteresis_WithinBand_UpdatesToNewValue` | Value inside dead-band → updated to new value | PASS |
| 12 | `test_Hysteresis_OutsideBand_NoChange` | Value outside dead-band → unchanged | PASS |
| 13 | `test_HysteresisMinusOffset_BelowLower_ZerosValue` | Below lower offset → zeroed | PASS |
| 14 | `test_HysteresisMinusOffset_InBand_SubtractsLower` | In-band → subtract lower offset | PASS |
| 15 | `test_HysteresisMinusOffset_NegativeInput_BelowLower` | Negative input below lower → zeroed | PASS |

---

## `test_PumpController.c` — Pump State Machine + CMock Demo (7 tests)

**Source under test:** `ble_espresso_app/components/Application/PumpController.c`  
**Mocks:** `mock_solidStateRelay_Controller` — **auto-generated** by CMock from
`ble_espresso_app/components/Peripherals/include/solidStateRelay_Controller.h`.

This file demonstrates three key CMock capabilities vs. the Phase 2 FFF approach:

### CMock features demonstrated

#### 1 — Auto-generated mock
```c
// Phase 2 (FFF) — manual declaration in fakes.h:
FAKE_VOID_FUNC(fcn_SolenoidSSR_On);
FAKE_VOID_FUNC(fcn_SolenoidSSR_Off);
FAKE_VALUE_FUNC(int, get_SolenoidSSR_State);
FAKE_VOID_FUNC(fcn_pumpSSR_pwrUpdate, uint16_t);

// Phase 3 (CMock) — one include, everything generated:
#include "mock_solidStateRelay_Controller.h"
```
CMock parses `solidStateRelay_Controller.h` and generates `_Expect()`,
`_IgnoreAndReturn()`, `_StubWithCallback()`, etc. for every function declared in it.

#### 2 — Strict call-ordering (`test_PC_StartBrew_ThenDriver_CallsSolenoidOn`)
```c
// Declare exactly ONE expected call before the code runs:
fcn_SolenoidSSR_On_Expect();
fcn_PumpStateDriver();
// tearDown() automatically verifies all pending expectations.
// 0 calls or 2+ calls → immediate test failure.
```

#### 3 — Callback argument capture (`test_PC_CancelBrew_StopsPump`)
```c
static uint16_t s_last_pump_pwr = 0xFFFF;
static void pump_pwr_cb(uint16_t pwr, int num_calls) { s_last_pump_pwr = pwr; }

fcn_pumpSSR_pwrUpdate_StubWithCallback(pump_pwr_cb);
// ... run 50 driver ticks ...
TEST_ASSERT_EQUAL_UINT16(0, s_last_pump_pwr);   // last update must be 0
```
Replaces the Phase 2 pattern of reading `fake.arg0_history[fake.call_count - 1]`.

### Test results

| # | Test name | CMock technique | Assertion | Result |
|---|-----------|-----------------|-----------|--------|
| 1 | `test_PC_Init_Returns_OK` | `_IgnoreAndReturn` (setUp) | Returns `PUMPCTRL_INIT_OK` | PASS |
| 2 | `test_PC_Init_StateDriver_Returns_IDLE` | `_IgnoreAndReturn` (setUp) | Returns `PUMPCTRL_IDLE` after init | PASS |
| 3 | `test_PC_LoadNewParams_Valid_Returns_OK` | `_IgnoreAndReturn` (setUp) | Returns `PUMPCTRL_LOAD_OK` | PASS |
| 4 | `test_H1_ZeroDeclineTime_NoCrash_Returns_OK` | `_IgnoreAndReturn` (setUp) | Zero decline time → no crash, `LOAD_OK` | PASS |
| 5 | `test_H1_AllZeroTimes_NoCrash` | `_IgnoreAndReturn` (setUp) | All times zero → no crash, `LOAD_OK` | PASS |
| 6 | `test_PC_StartBrew_ThenDriver_CallsSolenoidOn` | `_Expect()` strict ordering | `fcn_SolenoidSSR_On` called exactly once | PASS |
| 7 | `test_PC_CancelBrew_StopsPump` | `_StubWithCallback()` | Final pump power update = 0 | PASS |

---

## Ceedling Configuration Notes

Key `project.yml` settings that differ from the generated default:

| Setting | Value | Reason |
|---------|-------|--------|
| `:use_test_preprocessor` | `:mocks` | CMock must pre-process headers to generate mocks |
| `:unity: :defines: - UNITY_INCLUDE_FLOAT` | enabled | Default config disables float assertions |
| `:unity: :defines: - UNITY_EXCLUDE_FLOAT` | removed | Generated `project.yml` had this enabled — removed |
| `:cmock: :enforce_strict_ordering` | `TRUE` | Out-of-order `_Expect()` calls fail immediately |
| `:cmock: :treat_externs` | `:include` | Mocks extern-declared functions |
| `:cmock: :plugins` | `[:ignore, :callback, :return_thru_ptr]` | Enable `_Ignore()`, `_StubWithCallback()`, pointer returns |
| `:paths: :include` | `../tests/stubs` **first** | Stub headers shadow nRF SDK headers |
| `:defines: :test` | `TEST`, `HOST`, `NRF_LOG_ENABLED=0`, `SERVICE_*_EN=1` | Match Makefile Phase 2 flags |
| `:test_threads` | `1` | Avoid race conditions on shared static variables in state machines |

---

## Project structure

```
tests_ceedling\
├── project.yml               ← Ceedling configuration
├── ceedling.cmd              ← Launcher: ruby vendor\ceedling\bin\ceedling %*
├── test\
│   ├── test_ProfileValidator.c   ← H2 (pure logic, no mocks)
│   ├── test_x04_Numbers.c        ← BUG-001 + math utils (pure logic, no mocks)
│   └── test_PumpController.c     ← CMock demo: auto-mock, _Expect, _StubWithCallback
├── vendor\
│   └── ceedling\             ← local copy (--local flag at init)
│       ├── vendor\
│       │   ├── unity\        ← Unity source
│       │   └── cmock\        ← CMock source + parser
│       └── plugins\          ← gcov, module_generator, report_tests_pretty_stdout
└── build\                    ← auto-generated by ceedling
    ├── test\
    │   ├── out\              ← compiled .o and .out executables
    │   └── results\          ← .pass / .fail result files
    └── artifacts\
        └── gcov\             ← HTML coverage reports (ceedling gcov:all)
```

---

## How to run

```powershell
# Add Ruby to PATH (only needed if not already set)
$env:PATH += ";C:\Ruby32-x64\bin"

cd C:\WS\NRF\GaggiaController\tests_ceedling

# Run all tests
.\ceedling.cmd test:all

# Run a single test file
.\ceedling.cmd test:test_PumpController

# Run with verbose output
.\ceedling.cmd test:all --verbosity=obnoxious

# Generate gcov coverage report
.\ceedling.cmd gcov:all
# HTML report → build\artifacts\gcov\

# Clean all generated artefacts
.\ceedling.cmd clobber
```

### Expected `test:all` output

```
Ceedling set up completed in ~600 ms

Preprocessing for Mocks
  Preprocessing test_PumpController::solidStateRelay_Controller.h...

Mocking
  Generating mock for test_PumpController::solidStateRelay_Controller.h...

Test Runners
  Generating runner for test_ProfileValidator.c...
  Generating runner for test_PumpController.c...
  Generating runner for test_x04_Numbers.c...

Building Objects / Linking / Executing
  Running test_ProfileValidator.out...
  Running test_PumpController.out...
  Running test_x04_Numbers.out...

--------------------
OVERALL TEST SUMMARY
--------------------
TESTED:  36
PASSED:  36
FAILED:   0
IGNORED:  0
```

---

## Phase 2 vs Phase 3 comparison

| Aspect | Phase 2 (FFF + Makefile) | Phase 3 (CMock + Ceedling) |
|--------|--------------------------|---------------------------|
| Mock creation | Manual `FAKE_VOID_FUNC()` in `fakes.h` | Auto-generated from `#include "mock_<header>.h"` |
| Call verification | Post-hoc: `fake.call_count`, `arg0_history[]` | Pre-declared: `_Expect()` with tearDown auto-verify |
| Unexpected call | Silent unless test explicitly checks | Immediate test failure |
| Test discovery | Hand-written `main()` with `RUN_TEST()` | Automatic (Ceedling scans for `void test_*()`) |
| Build | Makefile, manual `-I` flags | Ceedling manages paths, compilation, linking |
| Coverage | Manual `gcov` invocation | `ceedling gcov:all` built-in |
| Speed (full suite) | ~2 s (88 tests, no recompile) | ~8 s (36 tests, includes mock generation) |
| Regression suite | Primary | Secondary (Ceedling is authoritative) |

---

## Cumulative test totals (all phases)

| Suite | Phase 1 | Phase 2 | Phase 2/3 | Phase 3 | Total |
|-------|---------|---------|-----------|---------|-------|
| Makefile (Unity + FFF) | 32 | 34 | 22 | — | **88** |
| Ceedling (CMock) | — | — | — | 36 | **36** |

> The 36 Ceedling tests are not additive — they duplicate intent from Phase 1 and
> Phase 2/3 under a stricter framework. Running both suites gives complementary
> confidence: Makefile catches regressions quickly; Ceedling enforces strict
> driver-call contracts and generates coverage data.
