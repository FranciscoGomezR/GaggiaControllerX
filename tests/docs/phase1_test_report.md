# Phase 1 Test Report — GaggiaController

| | |
|---|---|
| **Project** | GaggiaController |
| **Report date** | 2026-04-13 |
| **Phase** | Phase 1 — Pure Algorithm Tests (Host PC) |
| **Framework** | Unity v2.6.0 (ThrowTheSwitch/Unity) |
| **Compiler** | gcc (MinGW-W64 x86_64-ucrt-posix-seh) 15.2.0 |
| **Platform** | Windows x86-64 (host-side, no hardware) |
| **Build command** | `make -f tests/Makefile all` |
| **Overall result** | ✅ **PASS — 32 / 32 tests passed, 0 failures** |

---

## Summary

| Test Suite | Source Under Test | Tests | Pass | Fail | Ignored |
|------------|-------------------|------:|-----:|-----:|--------:|
| `test_numbers` | `x04_Numbers.c` | 15 | 15 | 0 | 0 |
| `test_digital_filters` | `x201_DigitalFiltersAlgorithm.c` | 8 | 8 | 0 | 0 |
| `test_pid_block` | `x205_PID_Block.c` | 9 | 9 | 0 | 0 |
| **Total** | | **32** | **32** | **0** | **0** |

---

## Suite 1 — `test_numbers` (15 tests)

**Source under test:** `ble_espresso_app/components/Utilities/x04_Numbers.c`  
**Test file:** `tests/test_numbers.c`  
**Dependencies:** `math.h` only — no stubs, no mocks

### Bug fixed during this phase

`fcn_Constrain_WithinFloats` had a copy-paste bug: the lower-bound branch returned `POSITIVE_SATURATION` instead of `NEGATIVE_SATURATION`. The Red test `test_Constrain_ValueBelowLower` caught it. Fix applied to `x04_Numbers.c` line 80 before final run.

**Before fix:**
```c
if( *Number < LowerLimit )
{
    *Number = LowerLimit;
    return POSITIVE_SATURATION;   // BUG: wrong constant
}
```
**After fix:**
```c
if( *Number < LowerLimit )
{
    *Number = LowerLimit;
    return NEGATIVE_SATURATION;   // FIXED
}
```

### Test results

| # | Test name | Function | Stimulus | Expected | Result |
|---|-----------|----------|----------|----------|--------|
| 1 | `test_Constrain_ValueWithinLimits` | `fcn_Constrain_WithinFloats` | `val=50`, limits `[0, 100]` | Returns `NO_SATURATION(0)`, val unchanged | ✅ PASS |
| 2 | `test_Constrain_ValueAboveUpper` | `fcn_Constrain_WithinFloats` | `val=150`, limits `[0, 100]` | Returns `POSITIVE_SATURATION(+1)`, val clamped to 100 | ✅ PASS |
| 3 | `test_Constrain_ValueBelowLower` | `fcn_Constrain_WithinFloats` | `val=-5`, limits `[0, 100]` | Returns `NEGATIVE_SATURATION(-1)`, val clamped to 0 | ✅ PASS *(was RED before fix)* |
| 4 | `test_Constrain_ValueAtUpperBoundary` | `fcn_Constrain_WithinFloats` | `val=100.0` (exact upper) | Returns `NO_SATURATION`, val unchanged | ✅ PASS |
| 5 | `test_Constrain_ValueAtLowerBoundary` | `fcn_Constrain_WithinFloats` | `val=0.0` (exact lower) | Returns `NO_SATURATION`, val unchanged | ✅ PASS |
| 6 | `test_ChrArrayToFloat_93_5` | `fcn_ChrArrayToFloat` | ASCII `{0x30,0x39,0x33,0x35}`, digits=3, dec=1 | Returns `93.5 ±0.01` | ✅ PASS |
| 7 | `test_ChrArrayToFloat_Zero` | `fcn_ChrArrayToFloat` | ASCII `{0x30,0x30,0x30,0x30}`, digits=3, dec=1 | Returns `0.0 ±0.01` | ✅ PASS |
| 8 | `test_ChrArrayToFloat_130_0` | `fcn_ChrArrayToFloat` | ASCII `{0x31,0x33,0x30,0x30}`, digits=3, dec=1 | Returns `130.0 ±0.01` | ✅ PASS |
| 9 | `test_FloatToChrArray_RoundTrip_93_5` | `fcn_FloatToChrArray` + `fcn_ChrArrayToFloat` | Encode `93.5f` → decode | Round-trip error `< 0.01` | ✅ PASS |
| 10 | `test_FloatToChrArray_RoundTrip_Zero` | `fcn_FloatToChrArray` + `fcn_ChrArrayToFloat` | Encode `0.0f` → decode | Round-trip error `< 0.01` | ✅ PASS |
| 11 | `test_Hysteresis_WithinBand_UpdatesToNewValue` | `fcn_AddHysteresis_WithinFloat` | `val=1.0`, band `±2.0`, new=`7.5` | `val` updated to `7.5` | ✅ PASS |
| 12 | `test_Hysteresis_OutsideBand_NoChange` | `fcn_AddHysteresis_WithinFloat` | `val=5.0`, band `±2.0` | `val` unchanged at `5.0` | ✅ PASS |
| 13 | `test_HysteresisMinusOffset_BelowLower_ZerosValue` | `fcn_AddHysteresisMinusOffset` | `val=0.5`, offsets `[1.0, 3.0]` | `val` zeroed | ✅ PASS |
| 14 | `test_HysteresisMinusOffset_InBand_SubtractsLower` | `fcn_AddHysteresisMinusOffset` | `val=2.0`, offsets `[1.0, 3.0]` | `val = 2.0 - 1.0 = 1.0` | ✅ PASS |
| 15 | `test_HysteresisMinusOffset_NegativeInput_BelowLower` | `fcn_AddHysteresisMinusOffset` | `val=-0.5`, offsets `[1.0, 3.0]` | `val` zeroed (abs logic + negate) | ✅ PASS |

---

## Suite 2 — `test_digital_filters` (8 tests)

**Source under test:** `ble_espresso_app/components/Utilities/x201_DigitalFiltersAlgorithm.c`  
**Test file:** `tests/test_digital_filters.c`  
**Dependencies:** none — pure C, state in caller-owned `lpf_rc_param_t`

### Test results

| # | Test name | Function | Stimulus | Expected | Result |
|---|-----------|----------|----------|----------|--------|
| 1 | `test_RCFilter_Fixed_Init_SetsCoefficients` | `pfcn_InitRCFilterAlgorithm` | `Fc=10 Hz`, `Ts=0.01 s` | Both coefficients `> 0`, `≤ 1`, sum `≈ 1.0` | ✅ PASS |
| 2 | `test_RCFilter_Fixed_DCConvergence` | `pfcn_RCFilterAlgorithm` (200 iters) | Constant input `5.0` | `DataOut_n ≈ 5.0 ±0.05` | ✅ PASS |
| 3 | `test_RCFilter_Fixed_ZeroInput_StaysZero` | `pfcn_RCFilterAlgorithm` (100 iters) | Input `= 0.0` always | `DataOut_n = 0.0 ±0.0001` | ✅ PASS |
| 4 | `test_RCFilter_Fixed_StepResponse_Attenuates` | `pfcn_RCFilterAlgorithm` (1 iter) | Unit step | `0 < DataOut_n < 1.0` (attenuated) | ✅ PASS |
| 5 | `test_RCFilter_Variable_CalculatesConst` | `lpf_rc_calculate_const` | `Fc=1 Hz` | `rc_constant ≈ 1/(2π) = 0.15915 ±0.001` | ✅ PASS |
| 6 | `test_RCFilter_Variable_StepResponse_63pct` | `lpf_rc_update` (~160 iters) | Unit step, `Fc=1 Hz`, `dt=0.001 s` | Output `≥ 0.632` after 1 time-constant | ✅ PASS |
| 7 | `test_RCFilter_Variable_ZeroInput_StaysZero` | `lpf_rc_update` (100 iters) | Input `= 0.0` | `DataOut_n = 0.0 ±0.0001` | ✅ PASS |
| 8 | `test_RCFilter_Fixed_vs_Variable_Consistent` | Both filter variants | Same `Fc=5 Hz`, `dt=0.01 s`, input `3.0`, 50 iters | Outputs agree within `0.1%` | ✅ PASS |

---

## Suite 3 — `test_pid_block` (9 tests)

**Source under test:** `ble_espresso_app/components/Utilities/x205_PID_Block.c`  
**Test file:** `tests/test_pid_block.c`  
**Dependencies:** `x04_Numbers.c`, `x201_DigitalFiltersAlgorithm.c` (compiled together)  
**State:** caller-owned `PID_Block_fStruct` / `PID_IMC_Block_fStruct`

### Test results

| # | Test name | Function | Stimulus | Expected | Result |
|---|-----------|----------|----------|----------|--------|
| 1 | `test_PID_Block_Ponly_OutputEqualsKpTimesError` | `fcn_update_PID_Block` | `SP=100, PV=90, Kp=2.0, Ki=Kd=0, dt=10 ms` | Output `= 20.0 ±0.1` | ✅ PASS |
| 2 | `test_PID_Block_Ionly_AccumulatesLinearly` | `fcn_update_PID_Block` | `Kp=Kd=0, Ki=1.0`, error `=10`, 5 steps `×100 ms` | `HistoryError ≈ 5.0 ±0.1` | ✅ PASS |
| 3 | `test_PID_Block_OutputSaturation_ClampsToLimit` | `fcn_update_PID_Block` | `Kp=100, SP=1000, PV=0, OutputLimit=100` | Output `≤ 100`, `OutputSaturationOut = POSITIVE_SATURATION` | ✅ PASS |
| 4 | `test_PID_Block_ResetI_HalvesIntegral` | `fcn_PID_Block_ResetI` | Accumulated `HistoryError`, `Attenuator=0.5` | `HistoryError` reduced by `50% ±1%` | ✅ PASS |
| 5 | `test_PID_Block_ResetI_ZeroAttenuator` | `fcn_PID_Block_ResetI` | Accumulated `HistoryError`, `Attenuator=0.0` | `HistoryError = 0.0 ±0.001` | ✅ PASS |
| 6 | `test_PIDimc_TypeA_HeatOnPositiveError` | `fcn_update_PIDimc_typeA` | `SP=100, PV=80, Kp=2.0, dt=10 ms` | Output `> 0` | ✅ PASS |
| 7 | `test_PIDimc_TypeA_CoolOnNegativeError` | `fcn_update_PIDimc_typeA` | `SP=80, PV=100, Kp=2.0, dt=10 ms` | Output `≤ 0` | ✅ PASS |
| 8 | `test_PIDimc_TypeA_OutputClampedToLimit` | `fcn_update_PIDimc_typeA` | `Kp=1000, SP=1000, PV=0, OutputLimit=500` | Output `≤ 500`, `OutputSaturationOut = POSITIVE_SATURATION` | ✅ PASS |
| 9 | `test_PIDimc_TypeA_vs_TypeB_DifferentDterm_OnSPstep` | `fcn_update_PIDimc_typeA` + `typeB` | Warm-up 3 ticks at `SP=93`, then SP step to `150`, `Kd=5.0`, `PV` constant | TypeA D spikes on SP step; TypeB D = 0 (PV unchanged) — outputs differ by `> 1.0` | ✅ PASS |

---

## Bug Found and Fixed

| ID | Severity | File | Line | Description | Status |
|----|----------|------|------|-------------|--------|
| `BUG-001` | HIGH | `x04_Numbers.c` | 80 | `fcn_Constrain_WithinFloats`: lower-bound branch returned `POSITIVE_SATURATION` instead of `NEGATIVE_SATURATION`. Any caller using the return value for saturation direction (e.g. PID windup clamping) would silently get the wrong direction, potentially causing runaway heating. | ✅ **Fixed** |

### Impact assessment

The `fcn_Constrain_WithinFloats` function is called in:
- `x205_PID_Block.c` — integral saturation check and output clamping (all three PID instances in `tempController.c`)
- `BLEspressoServices.c` — indirectly via PID ticks

The incorrect return value for the lower-bound case meant that `OutputSaturationOut == NEGATIVE_SATURATION` was never returned, so the anti-windup scheme's sign check could never identify a negative saturation event. Practically the boiler PID runs in positive-error territory (heating), so this was latent; the fix closes the gap before Phase 2 testing exposes it under broader conditions.

---

## Deferred Tests

The following functions are intentionally not tested in Phase 1:

| Function | Reason |
|----------|--------|
| `fcn_Constrain_WithinIntValues` | Integer variant; not exercised in any production hot path |
| `fcn_PID_Block_Dterm_LPF_Init` | `D_TERM_LP_FILTER_CTRL = NOT_ACTIVE` in all production PID instances |
| `fcn_PID_Block_Init_Dterm_LPfilter` | FIR D-filter variant; not instantiated in firmware |

---

## Environment

| Item | Value |
|------|-------|
| OS | Windows 11 x86-64 |
| Compiler | gcc (MinGW-W64 x86_64-ucrt-posix-seh, built by Brecht Sanders, r7) 15.2.0 |
| Make | GNU Make 4.4.1 |
| Unity | v2.6.0 (ThrowTheSwitch/Unity) |
| Build flags | `-Wall -Wextra -Wno-unused-parameter -Wno-old-style-declaration -Wno-missing-field-initializers -std=c99 -lm` |
| Output dir | `tests/out/` |

---

## Next Steps — Phase 2

Phase 2 adds FFF fakes and nRF SDK stub headers to test four application controllers against known issues:

| Module | Key Red tests (known issues) |
|--------|------------------------------|
| `tempController.c` | H3 (sensor failure), M1 (no integral reset on SP change) |
| `PumpController.c` | H1 (division by zero when `Prof_DeclineTmr=0`) |
| `BLEspressoServices.c` | H4 (overheat in step mode), H5 (no max brew time), M5 (I-boost stacking) |
| `StorageController.c` | M3 (no NVM data range validation) |

See [TDD_TESTPLAN.md](../../TDD_TESTPLAN.md) — Phase 2 steps 8–16 for the full implementation guide.
