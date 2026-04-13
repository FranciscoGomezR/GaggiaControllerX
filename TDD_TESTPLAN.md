# TDD Test Plan — GaggiaController (Phase 1 + Phase 2)

> **Companion to:** [TEST_PLAN.md](TEST_PLAN.md) — this document implements the TDD (Test-Driven Development) workflow for Phase 1 and Phase 2 tests defined there. Known issues from TEST_PLAN.md become Red-Green-Refactor targets.

## TL;DR

Set up a host-side (Windows gcc) unit testing infrastructure using **Unity** + **FFF** to apply TDD retroactively to the GaggiaController embedded C project. Phase 1 tests pure-logic modules (PID, filters, math utilities) with zero dependencies. Phase 2 tests four application controllers — `tempController`, `PumpController`, `BLEspressoServices`, and `StorageController` — using FFF fakes for hardware drivers and nRF SDK stub headers. The Red-Green-Refactor workflow is applied to existing code: write a failing test exposing a known issue → fix the code → refactor.

---

## Modules Under Test

Seven source modules are covered across Phase 1 and Phase 2. The table below gives a **TL;DR on what is tested and how** for each module, followed by a per-function breakdown.

| Module | Phase | Test file | Functions tested | How |
|--------|-------|-----------|-----------------|-----|
| `x04_Numbers.c` | 1 | `test_numbers.c` | 5 of 6 | Compiled directly on host gcc. Stateless — call function, assert return value and/or mutated argument. No mocks needed. |
| `x201_DigitalFiltersAlgorithm.c` | 1 | `test_digital_filters.c` | 4 of 4 | Compiled on host gcc. State lives in caller-owned `lpf_rc_param_t` struct passed by pointer. Tests drive loops of N iterations and assert convergence / step-response math. |
| `x205_PID_Block.c` | 1 | `test_pid_block.c` | 4 of 6 | Compiled on host gcc. State lives in caller-owned `PID_Block_fStruct` / `PID_IMC_Block_fStruct`. Tests feed known SP/PV/time sequences and assert output values and internal fields (`HistoryError`, `WindupClampStatus`). |
| `tempController.c` | 2 | `test_temp_controller.c` | 4 of 8 | Hardware calls faked with FFF (`f_getBoilerTemperature`, `fcn_boilerSSR_pwrUpdate`, timer functions). `milisTicks` exposed via `#ifdef TEST` accessor to inject time. Tests populate `blEspressoProfile` and call public API, asserting returned status codes and captured fake call args. |
| `PumpController.c` | 2 | `test_pump_controller.c` | 5 of 5 | SSR/solenoid calls faked with FFF. State machine stepped by calling `fcn_PumpStateDriver()` N times. Tests assert exact state transitions by hashing the sequence of `fcn_pumpSSR_pwrUpdate` call args captured in FFF's `arg_history`. |
| `BLEspressoServices.c` | 2 | `test_blespresso_services.c` | 3 modes | All sub-controllers faked (SSR, pump, tempCtrl, AC inputs). Tests set FFF return values to simulate switch states and temperature readings, call the service tick once, and assert which fake functions were called with which arguments. |
| `StorageController.c` | 2 | `test_storage_controller.c` | 5 of 6 | `spi_NVMemoryRead` and `spi_NVMemoryWritePage` replaced by FFF custom-fake callbacks that inject or capture the 65-byte NVM page in a `static uint8_t g_nvm_page[65]` buffer. Tests encode known float values into the buffer, call public API, and assert deserialized struct fields. Write tests capture `g_written_buf` and decode it back. |

---

### Module 1 — `x04_Numbers.c`
**Path:** `ble_espresso_app/components/Utilities/x04_Numbers.c`
**Dependencies:** none (pure C, `math.h` only)
**How tested:** call → assert, no mocks, no state setup beyond local variables
**Known bug exposed by tests:** `fcn_Constrain_WithinFloats` — the lower-bound branch incorrectly returns `POSITIVE_SATURATION` instead of `NEGATIVE_SATURATION` (both `if` arms return the same constant). The test `test_Constrain_ValueBelowLower` will catch this.

| Function | Signature | Tested | What the test does |
|----------|-----------|--------|--------------------|
| `fcn_Constrain_WithinFloats` | `int8_t (float* Number, float UpperLimit, float LowerLimit)` | ✅ | Passes values below, within, and above range; asserts returned flag (-1/0/+1) and mutated Number |
| `fcn_Constrain_WithinIntValues` | `void (long*, long, long)` | — | Integer variant; not used in hot paths. Deferred. |
| `fcn_AddHysteresis_WithinFloat` | `void (float* Number, float NumberWithoutHyst, float OffsetLimit)` | ✅ | Oscillates input ±OffsetLimit and asserts Number stays stable (no chatter) |
| `fcn_AddHysteresisMinusOffset` | `void (float* Number, float NumberWithoutHyst, float OffsetUpper, float OffsetLower)` | ✅ | Passes negative inputs; asserts sign-flip logic and dead-band removal |
| `fcn_ChrArrayToFloat` | `float (char* array, char noDigits, char noDecimals)` | ✅ | Passes encoded ASCII byte arrays for known values (93.5, 0.0, 130.0); asserts result ±0.01 |
| `fcn_FloatToChrArray` | `void (float, uint8_t*, char, char)` | ✅ | Encodes a float, feeds output to `fcn_ChrArrayToFloat`, asserts round-trip error < 0.01 |

---

### Module 2 — `x201_DigitalFiltersAlgorithm.c`
**Path:** `ble_espresso_app/components/Utilities/x201_DigitalFiltersAlgorithm.c`
**Dependencies:** none (pure C)
**State:** all state in caller-owned `lpf_rc_param_t` — fields `DataOut_n`, `DataOut_n_1`, `FilterRCCoefficients[2]`, `rc_constant`
**How tested:** allocate `lpf_rc_param_t` on the stack per test, init, drive a loop, assert output

| Function | Signature | Tested | What the test does |
|----------|-----------|--------|--------------------|
| `pfcn_InitRCFilterAlgorithm` | `void (lpf_rc_param_t*, float Fc_Hz, float Ts_s)` | ✅ | Calls with Fc=10Hz, Ts=0.01s; asserts both coefficients are non-zero and their sum ≈ 1.0 |
| `pfcn_RCFilterAlgorithm` | `void (lpf_rc_param_t*, float DataIn)` | ✅ | Feeds constant 5.0 for 200 iterations; asserts `DataOut_n` converges to 5.0 ±0.005 |
| `lpf_rc_calculate_const` | `void (lpf_rc_param_t*, float Fc_Hz)` | ✅ | Called implicitly by `lpf_rc_update`; verifies `rc_constant = 1/(2π·Fc)` ±0.001 |
| `lpf_rc_update` | `float (lpf_rc_param_t*, float DataIn, float t_s)` | ✅ | Feeds unit step at dt=0.001s, Fc=1Hz; asserts output reaches ≥63.2% of input by iteration 1000 (1 time constant) |

---

### Module 3 — `x205_PID_Block.c`
**Path:** `ble_espresso_app/components/Utilities/x205_PID_Block.c`
**Dependencies:** `x04_Numbers.c` and `x201_DigitalFiltersAlgorithm.c` (both pure C — compiled together)
**State:** all state in `PID_Block_fStruct` or `PID_IMC_Block_fStruct` allocated by the caller
**How tested:** zero-initialize struct, configure gains and enable flags, feed SP/PV/timeMilis sequences, assert `Output`, `HistoryError`, `WindupClampStatus`

| Function | Signature | Tested | What the test does |
|----------|-----------|--------|--------------------|
| `fcn_update_PID_Block` | `float (float Input, float SP, uint32_t tMs, PID_Block_fStruct*)` | ✅ | Five tests: P-only output = Kp×e; I linear over N steps; anti-windup clamps `HistoryError`; output saturates at `OutputLimit`; TypeB D-term doesn't spike on SP step |
| `fcn_PID_Block_ResetI` | `void (PID_Block_fStruct*, float Attenuator)` | ✅ | Accumulates integral, then calls reset with 0.5 and 0.0; asserts `HistoryError` is halved / zeroed |
| `fcn_update_PIDimc_typeA` | `float (PID_IMC_Block_fStruct*)` | ✅ | SP>PV → output > 0; SP<PV → output ≤ 0; large error → output clamped to `OutputLimit` |
| `fcn_update_PIDimc_typeB` | `float (PID_IMC_Block_fStruct*)` | ✅ | Run typeA and typeB with identical inputs + SP step; assert typeB output differs (D on PV, not error) |
| `fcn_PID_Block_Dterm_LPF_Init` | `void (PID_Block_fStruct*)` | — | LPF path inactive in production (`D_TERM_LP_FILTER_CTRL=NOT_ACTIVE`). Deferred. |
| `fcn_PID_Block_Init_Dterm_LPfilter` | `void (PID_Block_fStruct*, float*, uint8_t, uint8_t)` | — | FIR variant, not used. Deferred. |

---

### Module 4 — `tempController.c`
**Path:** `ble_espresso_app/components/Application/tempController.c`
**Dependencies faked:** `f_getBoilerTemperature` (SPI RTD), `fcn_boilerSSR_pwrUpdate` (SSR gate), nRF timer functions
**Key challenge:** `milisTicks` is `static volatile uint32_t` — the PID's only time source. A `#ifdef TEST` accessor must be added so tests can advance time without a real HW timer.
**How tested:** `blEspressoProfile` declared globally in test file; FFF fakes set `.return_val` before each call; public API called; returned status code and fake `.arg_history` asserted

| Function | Tested | What the test does |
|----------|--------|--------------------|
| `fcn_loadPID_ParamToCtrl_Temp` | ✅ | Sets `blEspressoProfile.Pid_P_term=9.5, Ki=0.1`; calls function; asserts `TEMPCTRL_LOAD_OK`. Separately, sets all gains to 0.0 and verifies corresponding term flags become `NOT_ACTIVE`. |
| `fcn_loadIboost_ParamToCtrl_Temp` | ✅ | Sets `Pid_Iboost_term=0.65`; calls function; asserts internal `sctrl_profile_main.Ki = 0.65` via a subsequent PID tick with known error. |
| `fcn_loaddSetPoint_ParamToCtrl_Temp` | ✅ / **M1 Red** | Steam SP switch: asserts `temp_Target = sp_StemTemp`. Then accumulates integral via ticks, switches SP, and asserts `HistoryError = 0` — this **fails** until integral reset is added. |
| `fcn_updateTemperatureController` | ✅ / **H3 Red** | Normal: fake returns 70°C, SP=93°C → `fcn_boilerSSR_pwrUpdate` called with arg > 0. Failure: fake returns 0.0°C (open sensor) or 400.0°C (short) → asserts power arg = 0 — **fails** until sensor guard added. |
| `fcn_initCntrl_Temp` | — | Calls HW timer init deep in call stack. Deferred to Phase 4. |
| `fcn_startTempCtrlSamplingTmr` / `fcn_stopTempCtrlSamplingTmr` | — | Pure HW timer enable/disable. Deferred to Phase 4. |
| `isr_HwTmr3_Period_EventHandler` | — | ISR; verified by Phase 4 on-target timer test. |

---

### Module 5 — `PumpController.c`
**Path:** `ble_espresso_app/components/Application/PumpController.c`
**Dependencies faked:** `fcn_SolenoidSSR_On/Off`, `fcn_pumpSSR_pwrUpdate`, `get_SolenoidSSR_State`
**State:** `Struct_PumpParam sPumpParam` — contains the state machine current state, all ramp slopes, and counters. It is declared `public` (no `static`), so tests can inspect it directly.
**How tested:** load parameters, call `fcn_StartBrew()`, then call `fcn_PumpStateDriver()` in a loop while asserting the sequence of `fcn_pumpSSR_pwrUpdate` arguments captured in `fake.arg0_history[]`

| Function | Tested | What the test does |
|----------|--------|--------------------|
| `fcn_initPumpController` | ✅ | Calls init; asserts `PUMPCTRL_INIT_OK` and `sPumpParam.smPump.sRunning == s0_Idle`. |
| `fcn_LoadNewPumpParameters` | ✅ / **H1 Red** | Normal load: asserts `PUMPCTRL_LOAD_OK` and slopes are non-zero. Zero-time test: sets `Prof_DeclineTmr=0.0` → private `fcn_LoadPumpParam` divides by zero → **hard fault** without the guard. Red test catches this; fix adds a zero-check before slope division. |
| `fcn_PumpStateDriver` | ✅ | Solenoid fake returns `SSR_STATE_ENGAGE` immediately. Drives 200 ticks; asserts `fcn_pumpSSR_pwrUpdate` call count > 0 and power values increase monotonically then decrease. |
| `fcn_StartBrew` | ✅ | Calls `fcn_StartBrew()`; asserts `sPumpParam.smPump.sRunning == s00_CloseSolenoid`. |
| `fcn_CancelBrew` | ✅ | Calls `fcn_StartBrew()`, drives a few ticks, then `fcn_CancelBrew()`; asserts `sPumpParam.smPump.sRunning == s67_RampdownToStop` and power reaches 0 after additional ticks. |

---

### Module 6 — `BLEspressoServices.c`
**Path:** `ble_espresso_app/components/Application/BLEspressoServices.c`
**Dependencies faked:** all SSR functions, `fcn_GetInputStatus_Brew/Steam`, `fcn_loaddSetPoint_ParamToCtrl_Temp`, `fcn_loadIboost_ParamToCtrl_Temp`, `fcn_StartBrew`, `fcn_CancelBrew`, `fcn_PumpStateDriver`, `fcn_updateTemperatureController`, `ble_update_boilerWaterTemp`
**How tested:** set FFF `.return_val` to simulate switch states and temperatures; call `fcn_svc_EspressoApp()` or `fcn_svc_StepFunctionApp()` once per simulated scheduler tick; assert which fakes were called and with which arguments

| Logic block | Tested | What the test does |
|-------------|--------|--------------------|
| Classic mode — brew ON | ✅ | Brew fake returns `AC_SWITCH_ASSERTED`; one tick; asserts `fcn_pumpSSR_pwrUpdate` called with 1000 AND `fcn_SolenoidSSR_On` called once. |
| Classic mode — brew OFF | ✅ | Brew fake returns `AC_SWITCH_DEASSERTED` after ON; asserts pump arg=0 and `fcn_SolenoidSSR_Off` called. |
| Classic mode — steam switch | ✅ | Steam fake asserted; asserts `fcn_loaddSetPoint_ParamToCtrl_Temp` called with `SETPOINT_STEAM`. |
| Profile mode — brew start | ✅ | Brew fake asserted in profile mode; asserts `fcn_StartBrew()` called once and `fcn_PumpStateDriver` called on subsequent ticks. |
| Step function — 100% heater | ✅ | Machine in tune mode; asserts `fcn_boilerSSR_pwrUpdate` called with 1000. |
| Step function — overheat **H4 Red** | ✅ | `blEspressoProfile.temp_Boiler` set to 160.0°C; asserts `fcn_boilerSSR_pwrUpdate` called with 0 — **fails** until cutoff added. |
| Profile mode — max brew time **H5 Red** | ✅ | Brew fake held asserted; advances simulated tick counter beyond 300s; asserts pump and solenoid are stopped — **fails** until timeout added. |
| Rapid I-boost cycling **M5 Red** | ✅ | Brew toggled ON/OFF 5 times in 2s; asserts `fcn_loadIboost_ParamToCtrl_Temp` called at most once — **fails** until cooldown guard added. |

---

### Module 7 — `StorageController.c`
**Path:** `ble_espresso_app/components/Application/StorageController.c`
**Dependencies faked:** `spi_NVMemoryRead`, `spi_NVMemoryWritePage`, `spim_initNVmemory`
**Note on `nrf_fstorage`:** `StorageController.c` includes `nrf_fstorage.h` and `nrf_fstorage_sd.h` but calls none of their API — flash is accessed entirely through `spi_Devices`. Both headers are stubbed empty.
**How tested:** FFF custom-fake callbacks replace `spi_NVMemoryRead` / `spi_NVMemoryWritePage` with functions that `memcpy` from/to a local `uint8_t g_nvm_page[65]` buffer. Tests pre-load `g_nvm_page` with IEEE 754-encoded floats, call the public API, and assert the deserialized struct. Write tests capture what was written into `g_written_buf[65]` and decode it back.

```c
// Test fixture pattern (in setUp / per-test)
static uint8_t g_nvm_page[65];
static uint8_t g_written_buf[65];

void fake_NVMRead(uint32_t pg, uint8_t off, uint32_t n, uint8_t* buf) {
    memcpy(buf, g_nvm_page + off, n);
}
void fake_NVMWrite(uint32_t pg, uint8_t off, uint32_t n, uint8_t* buf) {
    memcpy(g_written_buf, buf, n);
}
// In setUp():
spi_NVMemoryRead_fake.custom_fake    = fake_NVMRead;
spi_NVMemoryWritePage_fake.custom_fake = fake_NVMWrite;
```

The three **private** helpers (`parsingBytesToFloat`, `parsingBytesTo32bitVar`, `encodeFloatToBytes`) are not static — they can be forward-declared in the test file and tested directly to verify float serialization math independently of the public API.

| Function | Tested | What the test does |
|----------|--------|--------------------|
| `stgCtrl_Init` | ✅ | `spim_initNVmemory` fake returns `NVM_INIT_OK`; asserts `stgCtrl_Init()` returns same value. |
| `stgCtrl_ChkForUserData` | ✅ | Injects valid key `{0xAA,0x00,0xAA,0x00}` at offset 4 → asserts `STORAGE_USERDATA_LOADED`. Injects `{0xFF,0xFF,0xFF,0xFF}` → asserts `STORAGE_USERDATA_EMPTY`. |
| `stgCtrl_ReadUserData` | ✅ / **M3 Red** | Normal: encodes `temp_Target=93.0f`, `Pid_P_term=5.5f` into `g_nvm_page`; asserts struct fields match ±0.001. Invalid-key: leaves struct unchanged. Out-of-range: injects `temp_Target=999.0f` with valid key → function returns `LOADED` without clamping — documents current broken behavior; **fix adds range validation**. |
| `stgCtrl_StoreShotProfileData` | ✅ | Pre-loads NVM with valid data; sets new profile in struct; calls function; decodes `g_written_buf` and asserts: (a) profile floats match ±0.001, (b) PID section bytes unchanged, (c) shot write-cycle counter incremented by 1. Also tests first-write path (key=0xFF…FF → key written to buffer). |
| `stgCtrl_StoreControllerData` | ✅ | Same pattern as above but for PID section: asserts PID floats encoded correctly, profile section bytes unchanged, controller write-cycle counter incremented. |
| `stgCtrl_PrintUserData` | — | Only calls `NRF_LOG_DEBUG` macros (no-ops in test stubs). Nothing observable to assert. Deferred. |
| `parsingBytesToFloat` (private) | ✅ | Forward-declared in test; encode known float (3.14159f) to bytes via `encodeFloatToBytes`, decode back, assert round-trip error < 0.0001. |
| `encodeFloatToBytes` (private) | ✅ | Encode 93.0f; assert byte pattern matches expected IEEE 754 little-endian representation. |

---

## Phase 1: Pure Algorithm Tests (Host PC, Unity)

### Step 1 — Create test directory structure

Create the `tests/` folder at project root with sub-folders for frameworks and test files:

```
tests/
├── Makefile
├── unity/              ← Unity framework (copy or git submodule from ThrowTheSwitch/Unity)
│   ├── unity.c
│   ├── unity.h
│   └── unity_internals.h
├── test_runners/       ← Auto-generated or hand-written test runners
├── test_numbers.c
├── test_digital_filters.c
└── test_pid_block.c
```

### Step 2 — Add Unity framework

Download Unity from https://github.com/ThrowTheSwitch/Unity (tag v2.6.0 or latest).
Copy `src/unity.c`, `src/unity.h`, `src/unity_internals.h` into `tests/unity/`.

### Step 3 — Create Makefile for Phase 1

Build system: plain Makefile using `gcc` (MinGW on Windows or WSL).

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c99
CFLAGS += -I tests/unity
CFLAGS += -I ble_espresso_app/components/Utilities
CFLAGS += -I ble_espresso_app/components/Utilities/include
LDFLAGS = -lm

SRC_UNITY = tests/unity/unity.c
SRC_NUMBERS = ble_espresso_app/components/Utilities/x04_Numbers.c
SRC_FILTERS = ble_espresso_app/components/Utilities/x201_DigitalFiltersAlgorithm.c
SRC_PID = ble_espresso_app/components/Utilities/x205_PID_Block.c

all: test_numbers test_digital_filters test_pid_block
	./test_numbers.exe
	./test_digital_filters.exe
	./test_pid_block.exe

test_numbers: tests/test_numbers.c $(SRC_NUMBERS) $(SRC_UNITY)
	$(CC) $(CFLAGS) $^ -o $@.exe $(LDFLAGS)

test_digital_filters: tests/test_digital_filters.c $(SRC_FILTERS) $(SRC_UNITY)
	$(CC) $(CFLAGS) $^ -o $@.exe $(LDFLAGS)

test_pid_block: tests/test_pid_block.c $(SRC_PID) $(SRC_FILTERS) $(SRC_NUMBERS) $(SRC_UNITY)
	$(CC) $(CFLAGS) $^ -o $@.exe $(LDFLAGS)

clean:
	del /Q *.exe 2>nul || true
```

### Step 4 — Write `test_numbers.c` (TDD for x04_Numbers)

**Module:** `x04_Numbers.c` — float clamping, serialization, hysteresis
**Functions to test:**

| Test Name | Function | Stimulus | Expected |
|-----------|----------|----------|----------|
| `test_Constrain_ValueWithinLimits` | `fcn_Constrain_WithinFloats` | Number=50, limits [0,100] | Returns 0 (NO_SATURATION), Number unchanged |
| `test_Constrain_ValueBelowLower` | `fcn_Constrain_WithinFloats` | Number=-5, limits [0,100] | Returns -1 (NEG_SAT), Number=0 |
| `test_Constrain_ValueAboveUpper` | `fcn_Constrain_WithinFloats` | Number=150, limits [0,100] | Returns +1 (POS_SAT), Number=100 |
| `test_Constrain_ValueAtExactBoundary` | `fcn_Constrain_WithinFloats` | Number=0.0 and 100.0 | Returns 0, no clamp |
| `test_ChrArrayToFloat_PositiveValue` | `fcn_ChrArrayToFloat` | "93.5" (3 digits, 1 decimal) | Returns 93.5 ±0.01 |
| `test_FloatToChrArray_RoundTrip` | Both functions | Float 93.5 → array → float | Matches ±0.01 |
| `test_FloatToChrArray_Zero` | `fcn_FloatToChrArray` | 0.0 | Array = "00.0" or equivalent |
| `test_Hysteresis_SymmetricOffset` | `fcn_AddHysteresis_WithinFloat` | Oscillating input around threshold | Output stable (no chatter) |
| `test_Hysteresis_AsymmetricBounds` | `fcn_AddHysteresisMinusOffset` | Edge inputs | Correct asymmetric hysteresis |

> **TEST_PLAN.md cross-ref:** Phase 1 → `test_numbers.c` verification criteria table

### Step 5 — Write `test_digital_filters.c` (TDD for x201_DigitalFiltersAlgorithm)

**Module:** `x201_DigitalFiltersAlgorithm.c` — RC low-pass filter
**Functions to test:**

| Test Name | Function | Stimulus | Expected |
|-----------|----------|----------|----------|
| `test_RCFilter_Init_SetsCoefficients` | `pfcn_InitRCFilterAlgorithm` | Fc=10Hz, Ts=0.01s | Coefficients computed (non-zero, ≤1.0) |
| `test_RCFilter_StepResponse_63pct` | `lpf_rc_update` (loop) | Unit step, Fc=1Hz, dt=0.001s | Output reaches 63.2% of input within 1/Fc ±5% |
| `test_RCFilter_DCConvergence` | `pfcn_RCFilterAlgorithm` (loop) | Constant=5.0, 200 iterations | Settles to 5.0 ±0.1% |
| `test_RCFilter_ZeroInput_StaysZero` | `lpf_rc_update` | Input=0 always | Output stays 0 |
| `test_RCFilterConst_vs_Variable` | Both filter functions | Same input sequence | Outputs match when dt is constant |

> **TEST_PLAN.md cross-ref:** Phase 1 → `test_digital_filters.c` verification criteria table

### Step 6 — Write `test_pid_block.c` (TDD for x205_PID_Block)

**Module:** `x205_PID_Block.c` — PID-IMC controllers
**Functions to test:**

| Test Name | Function | Stimulus | Expected |
|-----------|----------|----------|----------|
| `test_PID_Ponly_OutputEqualsKpTimesError` | `fcn_update_PID_Block` | SP=100, PV=90, Kp=2.0, Ki=Kd=0 | Output = 20.0 |
| `test_PID_Ionly_AccumulatesLinearly` | `fcn_update_PID_Block` | Constant error=10, Ki=1.0, N steps | Output = Ki×error×dt×N ±1% |
| `test_PID_AntiWindup_ClampsIntegral` | `fcn_update_PID_Block` | Error sustained, IntegralLimit=50 | HistoryError ≤ IntegralLimit |
| `test_PID_AntiWindup_SaturationFlag` | `fcn_update_PID_Block` | Large integral saturation | `WindupClampStatus` = true |
| `test_PID_Dterm_KickRejection` | `fcn_update_PID_Block` | SP step change, PV constant | D-term contribution ≈ 0 (SP change shouldn't cause spike if TypeB) |
| `test_PID_OutputSaturation_ClampsToLimit` | `fcn_update_PID_Block` | Very large error | Output ≤ OutputLimit |
| `test_PID_OutputSaturation_NegativeClamp` | `fcn_update_PID_Block` | Negative error | Output ≥ 0 (or -OutputLimit) |
| `test_PID_ResetI_HalvesIntegral` | `fcn_PID_Block_ResetI` | Accumulated integral, Attenuator=0.5 | HistoryError reduced by 50% |
| `test_PID_ResetI_ZeroAttenuator` | `fcn_PID_Block_ResetI` | Attenuator=0.0 | HistoryError = 0 |
| `test_PIDimc_TypeA_HeatOnPositiveError` | `fcn_update_PIDimc_typeA` | SP=100, PV=80 | Output > 0 |
| `test_PIDimc_TypeA_NoHeatOnNegativeError` | `fcn_update_PIDimc_typeA` | SP=80, PV=100 | Output ≤ 0 |
| `test_PIDimc_TypeA_vs_TypeB_DifferentDterm` | Both typeA/typeB | Same inputs, step in SP | TypeA D reacts to error; TypeB D reacts to PV only |

> **TEST_PLAN.md cross-ref:** Phase 1 → `test_pid_block.c` verification criteria table

### Step 7 — Compile, run, fix

Run `make all` from project root. Fix compilation issues (e.g., missing `x02_FlagValues.h` include path). Iterate Red→Green→Refactor.

**Phase 1 pass criteria:** All 3 test executables build and pass on host Windows gcc.

---

## Phase 2: Application Logic Tests (Host PC, Unity + FFF)

### Step 8 — Add FFF framework

Download FFF from https://github.com/meekrosoft/fff (single header).
Copy `fff.h` into `tests/fff/`.

### Step 9 — Create nRF SDK stub headers

Application modules `#include` nRF SDK headers. Create minimal stubs so they compile on x86:

```
tests/stubs/
├── nrf.h                      ← empty
├── nrf_drv_timer.h            ← typedef struct {} nrf_drv_timer_t; etc.
├── nrf_drv_gpiote.h           ← typedef uint32_t nrf_drv_gpiote_pin_t; etc.
├── nrf_drv_spi.h              ← typedef struct {} nrf_drv_spi_t; etc.
├── nrf_gpio.h                 ← empty (or pin macros)
├── nrf_log.h                  ← #define NRF_LOG_INFO(...) #define NRF_LOG_DEBUG(...)
├── nrf_log_ctrl.h             ← empty
├── nrf_log_default_backends.h ← empty
├── app_error.h                ← #define APP_ERROR_CHECK(x) ((void)(x))
├── boards.h                   ← pin defines from app_config.h
├── app_config.h               ← copy pin defines only
├── nrf_delay.h                ← #define nrf_delay_ms(x)
├── nrf_fstorage.h             ← empty (StorageController.h includes it)
├── nrf_fstorage_sd.h          ← empty
├── nrf_soc.h                  ← empty
└── nordic_common.h            ← #define STATIC_ASSERT(x) (empty)
```

Each stub provides **only** the types, macros, and function signatures needed for compilation. No real implementation.

### Step 10 — Create FFF fakes header (`tests/fakes.h`)

Centralized fake declarations for all hardware driver functions:

```c
#include "fff.h"
DEFINE_FFF_GLOBALS;

// solidStateRelay_Controller.h
FAKE_VOID_FUNC(fcn_boilerSSR_pwrUpdate, uint16_t);
FAKE_VOID_FUNC(fcn_pumpSSR_pwrUpdate, uint16_t);
FAKE_VOID_FUNC(fcn_SolenoidSSR_On);
FAKE_VOID_FUNC(fcn_SolenoidSSR_Off);
FAKE_VALUE_FUNC(ssr_status_t, fcn_initSSRController_BLEspresso);
FAKE_VALUE_FUNC(ssr_status_t, get_SolenoidSSR_State);

// spi_Devices.h
FAKE_VALUE_FUNC(float, f_getBoilerTemperature);
FAKE_VOID_FUNC(spim_ReadRTDconverter);
FAKE_VALUE_FUNC(bool, spim_operation_done);

// ac_inputs_drv.h
FAKE_VALUE_FUNC(acInput_status_t, fcn_GetInputStatus_Brew);
FAKE_VALUE_FUNC(acInput_status_t, fcn_GetInputStatus_Steam);

// nrf_drv_timer (stubs for timer init/enable/disable)
FAKE_VALUE_FUNC(uint32_t, nrf_drv_timer_init, void*, void*, void*);
FAKE_VOID_FUNC(nrf_drv_timer_enable, void*);
FAKE_VOID_FUNC(nrf_drv_timer_disable, void*);

// bluetooth_drv.h
FAKE_VOID_FUNC(ble_update_boilerWaterTemp, float);

// spi_Devices.h — NVM operations (for StorageController)
FAKE_VOID_FUNC(spi_NVMemoryRead, uint32_t, uint8_t, uint32_t, uint8_t*);
FAKE_VOID_FUNC(spi_NVMemoryWritePage, uint32_t, uint8_t, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, spim_initNVmemory);
```

### Step 11 — Extend Makefile for Phase 2

Add compile flags:

```makefile
CFLAGS += -DTEST -DHOST
CFLAGS += -I tests/stubs -I tests/fff
CFLAGS += -I ble_espresso_app/components/Application
CFLAGS += -I ble_espresso_app/components/Peripherals/include
CFLAGS += -I ble_espresso_app/components/BLE/include
```

The `-I tests/stubs` appears **before** any SDK path, so stubs shadow real SDK headers.

New targets: `test_temp_controller`, `test_pump_controller`, `test_blespresso_services`, `test_storage_controller`.

### Step 12 — Write `test_temp_controller.c`

**Module:** `tempController.c` — PID boiler regulation
**Strategy:** Fake `f_getBoilerTemperature()`, `fcn_boilerSSR_pwrUpdate()`, and the nrf timer. Provide the global `blEspressoProfile` struct in the test file.

| Test Name | Stimulus | Expected | Issue Validated |
|-----------|----------|----------|-----------------|
| `test_LoadPID_SetsGains` | Set `blEspressoProfile.Pid_P_term=9.5`, call `fcn_loadPID_ParamToCtrl_Temp()` | Returns `TEMPCTRL_LOAD_OK` | Basic functionality |
| `test_LoadIboost_MultipliesKi` | Call `fcn_loadIboost_ParamToCtrl_Temp()` | Internal Ki×6.5 during brew | Brew temp stability |
| `test_SetpointSwitch_ToSteam` | Call with `SETPOINT_STEAM` | PID target = `sp_StemTemp` | Basic functionality |
| `test_SetpointSwitch_IntegralReset` | Run PID to accumulate I, then switch SP | Integral history zeroed | **M1: No integral reset on SP change** |
| `test_UpdateTemp_OutputInRange` | PV=20, SP=93 (large error) | `fcn_boilerSSR_pwrUpdate` called with arg ∈ [0, 1000] | Safety bounds |
| `test_UpdateTemp_HeatsWhenCold` | `f_getBoilerTemperature` returns 70.0, SP=93 | Output > 0 | Directional correctness |
| `test_UpdateTemp_StopsWhenHot` | `f_getBoilerTemperature` returns 100.0, SP=93 | Output = 0 | Overheat prevention |
| `test_SensorFailure_OpenCircuit` | `f_getBoilerTemperature` returns 0.0 | Output = 0 (safe shutdown) | **H3: No sensor failure detection** |
| `test_SensorFailure_ShortCircuit` | `f_getBoilerTemperature` returns 400.0 | Output = 0 (safe shutdown) | **H3: No sensor failure detection** |

**Note on H3 and M1:** These tests will initially **FAIL** (Red). This is the TDD workflow — write the test, see it fail, then fix the source code to make it pass (Green), then refactor.

> **TEST_PLAN.md cross-ref:** Phase 2 → `test_temp_controller.c` verification criteria; Known Issues H3, M1

### Step 13 — Write `test_pump_controller.c`

**Module:** `PumpController.c` — 3-stage pressure profile

| Test Name | Stimulus | Expected | Issue Validated |
|-----------|----------|----------|-----------------|
| `test_Init_ReturnsOK` | Call `fcn_initPumpController()` | Returns `PUMPCTRL_INIT_OK` | Basic init |
| `test_LoadParams_SetsSlopes` | Load profile with known values | Internal slopes calculated correctly | Basic functionality |
| `test_LoadParams_ZeroTime_NoCrash` | `prof_preInfuseTmr=0` | No crash, returns error | **H1: Division by zero** |
| `test_StateProgression_FullCycle` | `fcn_StartBrew()` then N × `fcn_PumpStateDriver()` | States: idle→Ramp1→Keep1→Ramp2→Keep2→Ramp3→Keep3→Stop | Full cycle |
| `test_RampSlope_LinearIncrease` | Monitor `fcn_pumpSSR_pwrUpdate` args across calls | Power increases linearly ±5% | Extraction quality |
| `test_CancelBrew_MidRamp` | `fcn_CancelBrew()` during ramp | State=idle, power=0, solenoid OFF | Safety |
| `test_CancelBrew_SolenoidOff` | Cancel + check `fcn_SolenoidSSR_Off` | Called exactly once | Prevent stuck solenoid |

> **TEST_PLAN.md cross-ref:** Phase 2 → `test_pump_controller.c` verification criteria; Known Issue H1

### Step 14 — Write `test_blespresso_services.c`

**Module:** `BLEspressoServices.c` — Mode state machines
**Strategy:** Fake all sub-controllers (pump, temp, SSR, AC inputs). Provide `blEspressoProfile` globally.

| Test Name | Stimulus | Expected | Issue Validated |
|-----------|----------|----------|-----------------|
| `test_ClassicBrew_On` | `fcn_GetInputStatus_Brew` returns ASSERTED | `fcn_pumpSSR_pwrUpdate(1000)` + `fcn_SolenoidSSR_On()` called | Basic brew |
| `test_ClassicBrew_Off` | Brew DEASSERTED after ON | Pump=0, solenoid OFF | Clean shutdown |
| `test_ClassicSteam_SwitchesSetpoint` | Steam ASSERTED | `fcn_loaddSetPoint_ParamToCtrl_Temp` called with STEAM | Mode switch |
| `test_ProfileBrew_TriggersStages` | Profile mode, brew ON | `fcn_StartBrew()` called, state machine runs | Profile mode |
| `test_StepFunction_FullPower` | Step function mode active | `fcn_boilerSSR_pwrUpdate(1000)` | Diagnostic mode |
| `test_StepFunction_OverheatProtection` | Temp rises past 150°C in step mode | Heater stops (power=0) | **H4: No overheat cutoff** |
| `test_ProfileBrew_MaxTimeLimit` | Brew switch held for >120s | Auto-stops pump + solenoid | **H5: Infinite brew** |
| `test_RapidBrewCycling` | Toggle brew 5× in 2s | I-boost not stacked | **M5: I-gain reapplied** |

> **TEST_PLAN.md cross-ref:** Phase 2 → `test_blespresso_services.c` verification criteria; Known Issues H4, H5, M5

### Step 15 — Write `test_storage_controller.c`

**Module:** `StorageController.c` — NVM persistence of espresso profile and PID gains
**Strategy:** Fake `spi_NVMemoryRead()` and `spi_NVMemoryWritePage()` using FFF callbacks to inject or capture the 65-byte NVM page. Additional stubs: `nrf_fstorage.h`, `nrf_soc.h`, `nordic_common.h` (all empty).

**What the module does internally:**
- Three private helpers handle all serialization/deserialization — `parsingBytesToFloat`, `parsingBytesTo32bitVar`, `encodeFloatToBytes` — these are pure logic, not static, so they can be forward-declared and tested directly.
- `stgCtrl_ReadUserData` validates the 4-byte key (`0x00AA00AA`) before deserializing. If key is wrong, returns `STORAGE_USERDATA_EMPTY` and leaves the struct untouched.
- `stgCtrl_StoreShotProfileData` and `stgCtrl_StoreControllerData` perform an atomic **read-modify-write**: read full 65 bytes, update only their section (profile = 32 B at offset 8; PID = 25 B at offset 40), write back the full page — preserving the other section.
- Write-cycle counters are stored as two packed 16-bit values in the first `uint32_t`: bits [31:16] = shot profile cycles, bits [15:0] = controller cycles.

**FFF callback pattern used to simulate NVM reads:**
```c
static uint8_t g_nvm_page[65];   // simulated NVM content, set per test
static uint8_t g_written_buf[65]; // captured by write fake

void fake_Read(uint32_t pg, uint8_t off, uint32_t n, uint8_t* buf) {
    memcpy(buf, g_nvm_page + off, n);
}
void fake_Write(uint32_t pg, uint8_t off, uint32_t n, uint8_t* buf) {
    memcpy(g_written_buf, buf, n);
}
// In setUp: spi_NVMemoryRead_fake.custom_fake = fake_Read;
//           spi_NVMemoryWritePage_fake.custom_fake = fake_Write;
```

| Test Name | Stimulus | Expected | Issue Validated |
|-----------|----------|----------|------------------|
| `test_Init_ForwardsNVMStatus` | `spim_initNVmemory` fake returns `NVM_INIT_OK` | `stgCtrl_Init()` returns same value | Basic wiring |
| `test_ChkUserData_ValidKey_ReturnsLoaded` | NVM page contains key `0x00AA00AA` at offset 4 | Returns `STORAGE_USERDATA_LOADED` | Key detection |
| `test_ChkUserData_EmptyFlash_ReturnsEmpty` | NVM page contains `0xFFFFFFFF` at offset 4 | Returns `STORAGE_USERDATA_EMPTY` | Fresh flash detection |
| `test_ReadUserData_ValidKey_ParsesAllFloats` | Full 65-byte page with known IEEE 754 values (e.g. `temp_Target=93.0f`, `Pid_P_term=5.5f`) | All struct fields match original values ±0.001 | Deserialization correctness |
| `test_ReadUserData_InvalidKey_LeavesStructUnchanged` | Key bytes are `0xDEADBEEF` | Returns `STORAGE_USERDATA_EMPTY`; struct fields not modified | Guard against corrupt flash |
| `test_ReadUserData_NoRangeValidation` | Valid key, but `temp_Target = 999.0f` in NVM bytes | Function returns `LOADED` without clamping — struct contains 999.0 | **M3 (RED): No NVM data validation** |
| `test_StoreShotProfile_FirstWrite_WritesKey` | NVM returns `0xFFFFFFFF` (empty) | Written buffer bytes [4–7] = `{0xAA,0x00,0xAA,0x00}` | Key written on first save |
| `test_StoreShotProfile_IncrementsProfileWriteCycle` | NVM has valid key, existing wCycleShotprofile = 3 | Written buffer bits [31:16] of word 0 = 4 | Write-cycle tracking |
| `test_StoreShotProfile_PreservesPIDSection` | Valid NVM with known PID bytes at offset 40 | Written buffer bytes [40–64] match the original read bytes exactly | Atomic partial write — PID not corrupted |
| `test_StoreShotProfile_EncodesProfileFloats` | Profile fields set in struct | Written buffer bytes [8–39] decode back to same float values ±0.001 | Serialization round-trip |
| `test_StoreControllerData_IncrementsCtrlWriteCycle` | NVM has valid key, existing wCycleCtrlProfile = 7 | Written buffer bits [15:0] of word 0 = 8 | Write-cycle tracking |
| `test_StoreControllerData_PreservesProfileSection` | Valid NVM with known profile bytes at offset 8 | Written buffer bytes [8–39] match the original read bytes exactly | Atomic partial write — profile not corrupted |
| `test_StoreControllerData_EncodesPIDFloats` | PID fields set in struct (Kp=9.5, Ki=0.1, etc.) | Written buffer bytes [40–64] decode back to same float values ±0.001 | PID serialization |

**Note on M3:** `test_ReadUserData_NoRangeValidation` will initially **PASS** (it documents the current broken behavior — loading 999.0°C without error). The TDD fix requires adding range checks in `stgCtrl_ReadUserData()` and rewriting the test to assert the fixed behavior (clamp or reject out-of-range values).

> **TEST_PLAN.md cross-ref:** Phase 2 → `test_storage_controller.c`; Known Issue M3

### Step 16 — Iterate Red-Green-Refactor

For each test that exposes a known issue (H1, H3, H4, H5, M1, M3, M5):

1. **Red:** Write the test → confirm it fails
2. **Green:** Modify source to fix the bug → confirm test passes
3. **Refactor:** Clean up without breaking tests

---

## Source Code Modifications Required (Minimal)

| File | Change | Purpose |
|------|--------|---------|
| `tempController.c` | Add `#ifdef TEST` accessor for `static milisTicks` | Let tests control PID timing |
| `tempController.c` | Add sensor range check in `fcn_updateTemperatureController()` | Fix H3 (tests will drive this) |
| `PumpController.c` | Add zero-time guard in `fcn_LoadNewPumpParameters()` | Fix H1 (test_LoadParams_ZeroTime will drive this) |
| `BLEspressoServices.c` | Add overheat cutoff in step function | Fix H4 |
| `BLEspressoServices.c` | Add max brew timer | Fix H5 |
| `StorageController.c` | Add range validation in `stgCtrl_ReadUserData()` for all float fields | Fix M3 (test_ReadUserData_NoRangeValidation will drive this) |
| `x02_FlagValues.h` / `x01_StateMachineControls.h` | May need `-I` path added to Makefile | Utility header dependencies |

**No other source files modified.** All test isolation is through stubs + FFF fakes + include path ordering.

---

## Known Issue → TDD Traceability

Tests in this plan that target known issues from [TEST_PLAN.md](TEST_PLAN.md):

| Issue | Severity | Description | TDD Test | Test File |
|-------|----------|-------------|----------|-----------|
| H1 | HIGH | Division by zero in pump slope calc when ramp time = 0 | `test_LoadParams_ZeroTime_NoCrash` | `test_pump_controller.c` |
| H3 | HIGH | No temperature sensor failure detection (open/short) | `test_SensorFailure_OpenCircuit`, `test_SensorFailure_ShortCircuit` | `test_temp_controller.c` |
| H4 | HIGH | No overheat protection in step-function mode | `test_StepFunction_OverheatProtection` | `test_blespresso_services.c` |
| H5 | HIGH | No maximum brew time limit in profile mode | `test_ProfileBrew_MaxTimeLimit` | `test_blespresso_services.c` |
| M1 | MEDIUM | No integral reset on setpoint change (brew→steam overshoot) | `test_SetpointSwitch_IntegralReset` | `test_temp_controller.c` |
| M3 | MEDIUM | No NVM data validation — unsafe params load silently from corrupt/blank flash | `test_ReadUserData_NoRangeValidation` | `test_storage_controller.c` |
| M5 | MEDIUM | I-gain boost reapplied on rapid brew cycling | `test_RapidBrewCycling` | `test_blespresso_services.c` |

---

## Relevant Files

| File | Role |
|------|------|
| `ble_espresso_app/components/Utilities/x04_Numbers.c` | Phase 1 test target — math utilities |
| `ble_espresso_app/components/Utilities/x201_DigitalFiltersAlgorithm.c` | Phase 1 test target — RC filter |
| `ble_espresso_app/components/Utilities/x205_PID_Block.c` | Phase 1 test target — PID algorithm |
| `ble_espresso_app/components/Application/tempController.c` | Phase 2 test target — modify for H3, M1 |
| `ble_espresso_app/components/Application/PumpController.c` | Phase 2 test target — modify for H1 |
| `ble_espresso_app/components/Application/BLEspressoServices.c` | Phase 2 test target — modify for H4, H5 |
| `ble_espresso_app/components/Application/StorageController.c` | Phase 2 test target — modify for M3 |
| `ble_espresso_app/components/Application/StorageController.h` | NVM status enum (`storageCtrl_status_t`) |
| `ble_espresso_app/components/Application/BLEspressoServices.h` | `bleSpressoUserdata_struct` definition |
| `ble_espresso_app/components/Peripherals/include/solidStateRelay_Controller.h` | FFF fake target |
| `ble_espresso_app/components/Peripherals/include/spi_Devices.h` | FFF fake target |
| `ble_espresso_app/components/Peripherals/include/ac_inputs_drv.h` | FFF fake target |

---

## Verification

1. **Phase 1 build:** Run `make test_numbers test_digital_filters test_pid_block` — all compile with 0 warnings
2. **Phase 1 pass:** All Unity tests report `0 Failures`
3. **Phase 2 build:** Run `make test_temp_controller test_pump_controller test_blespresso_services test_storage_controller` — all compile with stubs
4. **Phase 2 known-issue tests:** H1, H3, H4, H5, M1 initially fail (Red) → source fixes → pass (Green); M3 initially passes documenting current broken behavior → rewrite after fix → passes with new expected behavior
5. **Regression:** Re-run all Phase 1+2 tests after any source fix → 0 Failures across all suites
6. **No firmware breakage:** Build production firmware in SEGGER ES after all source changes → compiles and links within 92KB app window

---

## Decisions

- **Framework:** Unity (lightweight, embedded-C native) + FFF (header-only fakes). Not Ceedling/CMock yet — that's Phase 3 from TEST_PLAN.md
- **Build:** Plain Makefile + gcc. Works on Windows (MinGW) and WSL.
- **Scope:** Phase 1 (pure logic) + Phase 2 (application logic with mocks). On-target testing (Phases 4–6) deferred.
- **TDD on existing code:** Use "characterization test" approach — write tests for current behavior first, then write failing tests for known issues, then fix.
- **Known issues as TDD targets:** H1 (div-by-zero), H3 (sensor failure), H4 (overheat), H5 (infinite brew), M1 (integral reset) — each becomes a Red-Green-Refactor cycle.
- **Excluded:** BLE input validation (H2, M4), watchdog (C1), GPIO boot state (C3) — these require on-target testing (Phase 4+).
