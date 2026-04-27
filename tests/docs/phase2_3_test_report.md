# Phase 2/3 Test Report

**Date:** 2026-04-14  
**Framework:** Unity v2.6.0 + FFF  
**Toolchain:** gcc (MinGW-W64) 15.2.0 · GNU Make 4.4.1  
**Build command:** `make -f tests/Makefile all`  
**Project root:** `C:\WS\NRF\GaggiaController`  
**Related report:** [phase3_test_report.md](phase3_test_report.md)

---

## Overview

Phase 2/3 completes the application-layer issues deferred from Phase 2. All fixes
are exercised on the host PC (Windows) via the existing Unity + FFF Makefile suite.
No firmware or on-target changes are required to run these tests.

| Issue | Module | Description | Tests |
|-------|--------|-------------|-------|
| H2 | `ProfileValidator.c` (new) | BLE input validation — clamp out-of-range float writes | 14 |
| H5 | `BLEspressoServices.c` | Maximum brew time limit (120 s auto-stop) | 3 |
| M5 | `BLEspressoServices.c` | I-boost gain stacking on rapid brew cycling | 3 |
| M3 | `StorageController.c` | NVM float validation after deserialization | 4 |
| **Total** | | | **24** |

---

## Test Results

### Cumulative Makefile suite (`make -f tests/Makefile all`)

| Phase | Suite | Tests | Passed | Failed |
|-------|-------|-------|--------|--------|
| Phase 1 | Unity | 32 | 32 | 0 |
| Phase 2 | Unity + FFF | 34 | 34 | 0 |
| **Phase 2/3** | **Unity + FFF** | **22** | **22** | **0** |
| **Total** | | **88** | **88** | **0** |

> The 22 Phase 2/3 tests break down as: 14 (H2) + 3 (H5) + 3 (M5) + 4 (M3 update) — see detail below. The 4 M3 tests replace or augment the 1 M3 placeholder from Phase 2, so the net new test functions added are +22 minus the renamed test = 21 new + 1 updated.

---

## Issue H2 — BLE Input Validation

**File:** `ble_espresso_app/components/Application/ProfileValidator.c` (new)  
**Test file:** `tests/test_profile_validator.c`

### Root cause

`bluetooth_drv.c` forwarded every BLE GATT write directly into `blEspressoProfile`
without any range check. A malformed write containing NaN, Inf, or an out-of-range
float would be accepted silently and passed to the PID controller and SSR drivers on
the next service tick.

### Fix

Extracted a pure-logic validator module:

```c
// ProfileValidator.h
profileValidation_status_t fcn_ValidateAndClampProfile(bleSpressoUserdata_struct *profile);
bool fcn_ValidateFloat_InRange(float *value, float min, float max, float safeDefault);
```

`bluetooth_drv.c` calls `fcn_ValidateFloat_InRange()` after each GATT write.
`StorageController.c` calls `fcn_ValidateAndClampProfile()` after NVM deserialization
(M3 fix — see below). No SDK or hardware dependencies — fully testable on host.

### Test results

| # | Test name | Assertion |  Result |
|---|-----------|-----------|---------|
| 1 | `test_H2_SingleField_InRange_Valid` | Returns `true`, value unchanged | PASS |
| 2 | `test_H2_SingleField_OutOfRange_Clamped` | Returns `false`, value = default | PASS |
| 3 | `test_H2_SingleField_AtLowerBoundary_Valid` | Boundary accepted as valid | PASS |
| 4 | `test_H2_SingleField_AtUpperBoundary_Valid` | Boundary accepted as valid | PASS |
| 5 | `test_H2_ValidProfile_ReturnsValid` | All fields in range → `PROFILE_VALID` | PASS |
| 6 | `test_H2_TempTarget_NaN_ClampedToDefault` | NaN → 93.0 °C, `PROFILE_CLAMPED` | PASS |
| 7 | `test_H2_TempTarget_Inf_ClampedToDefault` | Inf → 93.0 °C, `PROFILE_CLAMPED` | PASS |
| 8 | `test_H2_TempTarget_Negative_ClampedToDefault` | −50 °C → 93.0 °C | PASS |
| 9 | `test_H2_TempTarget_TooHigh_ClampedToDefault` | 200 °C → 93.0 °C | PASS |
| 10 | `test_H2_PidPterm_Negative_Clamped` | −1.0 → 9.5 (default Kp) | PASS |
| 11 | `test_H2_PidIterm_TooHigh_Clamped` | 50 → 0.3 (default Ki) | PASS |
| 12 | `test_H2_ProfTimerZero_IsValid` | 0.0 s is a valid pre-infuse time | PASS |
| 13 | `test_H2_ProfDeclineTmr_Negative_Clamped` | −5 s → 10.0 s (default) | PASS |
| 14 | `test_H2_MultipleFieldsInvalid_AllClamped` | All three bad fields clamped; valid field unchanged | PASS |

---

## Issue H5 — Maximum Brew Time

**File:** `ble_espresso_app/components/Application/BLEspressoServices.c`  
**Test file:** `tests/test_blespresso_services.c` — tests 7–9

### Root cause

Neither `cl_Mode_1` (classic) nor `prof_Mode` (profile) had a duration limit.
A mechanically stuck switch would run the pump indefinitely, risking over-extraction
and hardware damage.

### Fix

```c
#define MAX_BREW_TICKS  (120000 / SERVICE_BASE_T_MS)   // 1200 ticks = 120 s

// Top of case cl_Mode_1 and prof_Mode:
if ((serviceTick - classicData.svcStartT) >= MAX_BREW_TICKS) {
    fcn_pumpSSR_pwrUpdate(PUMP_PWR_OFF);
    fcn_multiplyI_ParamToCtrl_Temp(..., 2.0f);   // enter phase2 recovery
    classicData.b_boostI_phase2 = true;
    fcn_SolenoidSSR_Off();
    espressoService_Status.sRunning = cl_idle;
    break;
}
```

`test_set_serviceTick()` / `test_get_serviceTick()` accessors (guarded by `#ifdef TEST`)
allow tests to jump serviceTick forward without calling the function 1200 times.

### Test results

| # | Test name | Assertion | Result |
|---|-----------|-----------|--------|
| 7 | `test_H5_ClassicBrew_AutoStops_After120s` | At tick 1200: pump=0, solenoid off | PASS |
| 8 | `test_H5_ClassicBrew_StillRunning_Before120s` | At tick 1199: solenoid NOT called | PASS |
| 9 | `test_H5_ProfileBrew_AutoStops_After120s` | Profile mode: pump=0, solenoid off | PASS |

---

## Issue M5 — I-Boost Gain Stacking

**File:** `ble_espresso_app/components/Application/BLEspressoServices.c`  
**Test file:** `tests/test_blespresso_services.c` — tests 10–12

### Root cause

When a brew cycle ended (switch off or timeout), `fcn_multiplyI_ParamToCtrl_Temp(..., 2.0f)`
was called to enter phase2 recovery (×2 I-gain). If the user started a new brew before
the boiler recovered, `b_boostI_phase2` was still `true` and `fcn_loadIboost_ParamToCtrl_Temp()`
applied another boost on top of the residual ×2 gain — effectively ×4 I-gain, causing
integral wind-up.

### Fix

```c
// In cl_idle (and prof_idle), before fcn_loadIboost_ParamToCtrl_Temp():
if (classicData.b_boostI_phase2) {
    fcn_multiplyI_ParamToCtrl_Temp(..., 1.0f);   // revert to ×1 first
    classicData.b_boostI_phase2 = false;
    classicData.b_normalI = true;
}
```

### Test results

| # | Test name | Assertion | Result |
|---|-----------|-----------|--------|
| 10 | `test_M5_RapidCycling_IboostNotStacked` | Revert (1.0f) called before loadIboost on rapid restart | PASS |
| 11 | `test_M5_NormalCycling_IboostAppliedOnce` | When phase2 already clear: loadIboost called once, no extra revert | PASS |
| 12 | `test_M5_ProfileMode_RapidCycling_NoCrash` | ON → OFF → ON in profile mode completes without crash | PASS |

---

## Issue M3 Fix — NVM Range Clamping

**File:** `ble_espresso_app/components/Application/StorageController.c`  
**Test file:** `tests/test_storage_controller.c` — tests 6–9 (total file: 13 tests)

### Root cause

`stgCtrl_ReadUserData()` deserialised all float fields from NVM flash without any
range check. A single bit-flip (flash wear, power loss during write) could produce
NaN or an extreme out-of-range value that would propagate directly to the PID
controller on the next service tick.

### Fix

```c
// stgCtrl_ReadUserData(), after dataStatus = STORAGE_USERDATA_LOADED:
fcn_ValidateAndClampProfile(ptr_rxData);
```

Test 6 was previously `test_M3_OutOfRange_Temp_Loaded_Without_Validation` (expected
999.0 °C to survive — demonstrating the bug). It is now renamed and asserts 93.0 °C,
demonstrating the fix.

### Test results

| # | Test name | Assertion | Result |
|---|-----------|-----------|--------|
| 6 | `test_M3_OutOfRange_Temp_Clamped_To_Default` | 999 °C in NVM → rx.temp_Target = 93.0 | PASS |
| 7 | `test_M3_NaN_PidPterm_Clamped` | NaN at NVM byte offset 0x28 → Pid_P_term = 9.5 | PASS |
| 8 | `test_M3_NegativeTimer_Clamped` | −5.0 s at offset 0x14 → prof_preInfuseTmr = 3.0 | PASS |
| 9 | `test_M3_AllFieldsValid_NoChange` | All fields in-range → no field modified | PASS |

---

## Files created or modified

### New files

| File | Purpose |
|------|---------|
| `ble_espresso_app/components/Application/ProfileValidator.h` | H2 — public API for validator |
| `ble_espresso_app/components/Application/ProfileValidator.c` | H2 — pure-logic validation, no HW deps |
| `tests/test_profile_validator.c` | 14 H2 tests (unit + integration) |

### Modified files

| File | Change |
|------|--------|
| `BLEspressoServices.c` | H5: `MAX_BREW_TICKS` guard in `cl_Mode_1` and `prof_Mode`; M5: phase2-revert before I-boost in `cl_idle` and `prof_idle`; `#ifdef TEST` serviceTick accessors |
| `StorageController.c` | M3: `#include "ProfileValidator.h"` + `fcn_ValidateAndClampProfile()` call after NVM read |
| `bluetooth_drv.c` | H2: `#include "ProfileValidator.h"` + `fcn_ValidateFloat_InRange()` after each GATT write |
| `tests/test_blespresso_services.c` | +6 tests (H5 × 3, M5 × 3); `main()` updated to 12 tests |
| `tests/test_storage_controller.c` | Test 6 renamed + assertion updated; +3 new M3 tests; `main()` updated to 13 tests |
| `tests/Makefile` | Added `SRC_VALIDATOR`, `phase2_3` target, updated `test_storage_controller.exe` deps, added `phase2_3` to `all` |

---

## Verification

```powershell
# Run Phase 1 + Phase 2 + Phase 2/3
make -f tests/Makefile all

# Run Phase 2/3 only
make -f tests/Makefile phase2_3
```

Expected output (Phase 2/3 only):

```
=== Running Phase 2/3 Tests ===
14 Tests 0 Failures 0 Ignored   OK   [test_profile_validator]
=== Phase 2/3 Complete ===
```

Full suite:

```
15 Tests 0 Failures 0 Ignored   OK   [test_numbers]
 8 Tests 0 Failures 0 Ignored   OK   [test_digital_filters]
 9 Tests 0 Failures 0 Ignored   OK   [test_pid_block]
11 Tests 0 Failures 0 Ignored   OK   [test_temp_controller]
 7 Tests 0 Failures 0 Ignored   OK   [test_pump_controller]
12 Tests 0 Failures 0 Ignored   OK   [test_blespresso_services]
13 Tests 0 Failures 0 Ignored   OK   [test_storage_controller]
14 Tests 0 Failures 0 Ignored   OK   [test_profile_validator]
                                      ─────────────────────
                              88 Tests 0 Failures 0 Ignored
```

