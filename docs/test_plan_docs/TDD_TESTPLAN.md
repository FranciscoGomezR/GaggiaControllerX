# TDD Test Plan — GaggiaController (Phase 1 + Phase 2 + Phase 2/3 + Phase 3)

> **Companion to:** [TEST_PLAN.md](TEST_PLAN.md) — this document implements the TDD (Test-Driven Development) workflow for Phases 1–3 tests defined there. Known issues from TEST_PLAN.md become Red-Green-Refactor targets.

## TL;DR

Set up a host-side (Windows gcc) unit testing infrastructure using **Unity** + **FFF** to apply TDD retroactively to the GaggiaController embedded C project. Phase 1 tests pure-logic modules (PID, filters, math utilities) with zero dependencies. Phase 2 tests four application controllers — `tempController`, `PumpController`, `BLEspressoServices`, and `StorageController` — using FFF fakes for hardware drivers and nRF SDK stub headers. Phase 2/3 completes the remaining application-layer issues deferred from Phase 2: **H2** (BLE input validation via extracted `ProfileValidator`), **H5** (maximum brew time), **M5** (I-boost cooldown), and **M3 fix** (NVM range validation). Phase 3 migrates to **Ceedling + CMock** for auto-generated mocks, strict call-ordering verification, and gcov code coverage. The Red-Green-Refactor workflow is applied to existing code: write a failing test exposing a known issue → fix the code → refactor.

---

## Host-Side Infrastructure Setup

### Why host-side?

The nRF52832 firmware cannot run on a PC. Instead, the source files under test are recompiled with MinGW-w64 gcc targeting x86-64 Windows. This removes all ARM-specific constraints (no SoftDevice, no flash, no real peripherals). The only hard boundary is that the source code must be valid portable C99 — which it already is.

```
┌──────────────────────────────────────────────────────────────┐
│  Windows (x86-64)                                            │
│                                                              │
│  gcc (MinGW-w64)  ──compiles──►  test_numbers.exe           │
│                                  test_digital_filters.exe    │
│                                  test_pid_block.exe          │
│     Phase 1: sources compiled directly — no stubs needed     │
│                                                              │
│  gcc + stubs + FFF ──compiles──►  test_temp_controller.exe  │
│                                   test_pump_controller.exe   │
│                                   test_blespresso_services   │
│                                   test_storage_controller    │
│     Phase 2: nRF SDK headers shadowed by empty stubs         │
│                                                              │
│  gcc + stubs + FFF ──compiles──►  test_profile_validator     │
│     Phase 2/3: H2, H5, M5, M3 fix (remaining issues)        │
│                                                              │
│  Ceedling + CMock  ──auto-gen──►  All Phase 2/3 tests        │
│     Phase 3: strict ordering, coverage, auto-mocks           │
└──────────────────────────────────────────────────────────────┘
```

---

### Step 0 — Prerequisites

**Install MinGW-w64** (if not already present):

Option A — winget (recommended):
```
winget install -e --id GnuWin32.Make
winget install -e --id MSYS2.MSYS2
```
Then inside MSYS2 shell: `pacman -S mingw-w64-x86_64-gcc make`

Option B — standalone installer:
Download [mingw-w64 v13+](https://github.com/niXman/mingw-builds-binaries/releases), extract, add `bin\` to `PATH`.

**Verify the installation:**
```
gcc --version    → gcc (MinGW-w64) 13.x or newer
make --version   → GNU Make 4.x or newer
```

Both commands must be reachable from a plain `cmd` or PowerShell session (no MSYS2 context required once `PATH` is set).

---

### Repository layout

```
GaggiaController\
├── tests\
│   ├── Makefile                ← single build script for all test targets
│   ├── unity\                  ← Unity v2.6.0 framework (3 files)
│   │   ├── unity.c
│   │   ├── unity.h
│   │   └── unity_internals.h
│   ├── fff\                    ← FFF single-header mock framework
│   │   └── fff.h
│   ├── stubs\                  ← minimal nRF SDK surrogate headers (Phase 2)
│   │   ├── nrf.h
│   │   ├── nrf_log.h
│   │   ├── nrf_drv_timer.h
│   │   ├── nrf_drv_spi.h
│   │   ├── nrf_fstorage.h
│   │   ├── nrf_fstorage_sd.h
│   │   ├── app_error.h
│   │   ├── boards.h
│   │   └── ...                 ← (full list in Step 9)
│   ├── fakes.h                 ← all FAKE_VOID_FUNC / FAKE_VALUE_FUNC declarations
│   ├── test_numbers.c
│   ├── test_digital_filters.c
│   ├── test_pid_block.c
│   ├── test_temp_controller.c
│   ├── test_pump_controller.c
│   ├── test_blespresso_services.c
│   └── test_storage_controller.c
│
├── ble_espresso_app\           ← production firmware (unchanged except minimal #ifdef TEST guards)
│   └── ...
├── TDD_TESTPLAN.md             ← this document
└── TEST_PLAN.md
```

Each test `*.c` file is a **self-contained compilation unit**: it includes `unity.h`, optionally `fakes.h`, and the production `.c` file(s) under test directly (not via a library). The Makefile lists each source file explicitly.

---

### Obtaining the frameworks

**Unity v2.6.0**

```powershell
# Option A — git submodule (recommended for version control)
git submodule add https://github.com/ThrowTheSwitch/Unity.git tests/unity-repo
# Then copy the 3 files:
Copy-Item tests\unity-repo\src\unity.c     tests\unity\
Copy-Item tests\unity-repo\src\unity.h     tests\unity\
Copy-Item tests\unity-repo\src\unity_internals.h tests\unity\

# Option B — direct download (no git dependency)
# Download the zip from https://github.com/ThrowTheSwitch/Unity/releases/tag/v2.6.0
# Extract only: src/unity.c, src/unity.h, src/unity_internals.h → tests\unity\
```

**FFF (meekrosoft/fff)**

FFF is a single header file. No build step, no configuration:
```powershell
# Option A — git submodule
git submodule add https://github.com/meekrosoft/fff.git tests/fff-repo
Copy-Item tests\fff-repo\fff.h tests\fff\

# Option B — direct download
# Download fff.h from https://github.com/meekrosoft/fff/blob/master/fff.h → tests\fff\
```

---

### Compiler flags explained

| Flag | Purpose |
|------|---------|
| `-std=c99` | Match firmware's C standard; enables `//` comments, `<stdint.h>`, `<stdbool.h>` |
| `-Wall -Wextra -Wno-unused-parameter` | Catch real bugs; suppress unused-param noise from stubs |
| `-I tests/unity` | Resolve `#include "unity.h"` |
| `-I tests/fff` | Resolve `#include "fff.h"` |
| `-I tests/stubs` | **Placed first** — stub headers shadow any real nRF SDK headers with the same name |
| `-I ble_espresso_app/components/Utilities` | Phase 1: resolve internal utility headers |
| `-I ble_espresso_app/components/Application` | Phase 2: resolve application module headers |
| `-I ble_espresso_app/components/Peripherals/include` | Phase 2: resolve peripheral driver headers (for FFF fake signatures) |
| `-DTEST` | Activates `#ifdef TEST` guards in production source (e.g., `milisTicks` accessor) |
| `-DHOST` | Optional: activates any `#ifdef HOST` workarounds for x86-only code paths |
| `-lm` | Link `libm` (required by `x205_PID_Block.c` and filter code that call `expf`, `fabsf`) |

> **Critical ordering:** `-I tests/stubs` must appear **before** any `-I` path that contains real nRF SDK headers. gcc resolves includes left-to-right — the first match wins. This is how stub headers silently replace the SDK without modifying any source file.

---

### How test isolation works

```
test_pump_controller.c
        │
        ├── #include "unity.h"          ← test framework
        ├── #include "fakes.h"          ← FFF: DEFINE_FFF_GLOBALS + all FAKE_*_FUNC declarations
        │       └── fff.h               ← FFF single header
        │
        ├── #include "PumpController.h" ← resolved from: ble_espresso_app/components/Application
        │
        └── compiled together with:
                PumpController.c        ← production source, compiled for x86
                unity.c                 ← Unity framework
```

When `PumpController.c` calls `fcn_pumpSSR_pwrUpdate(500)`:
- The linker finds `fcn_pumpSSR_pwrUpdate` in `fakes.h` (a FFF-generated stub function)
- The stub records the call count and argument in `fcn_pumpSSR_pwrUpdate_fake.arg0_history[]`
- The test body then asserts on those recorded values — no real hardware involved

---

### `#ifdef TEST` source guards (minimal production changes)

Two changes are needed in production source to make it testable without a running HW timer:

```c
// tempController.c — expose the private tick counter
static volatile uint32_t milisTicks = 0;

#ifdef TEST
void test_set_milisTicks(uint32_t t) { milisTicks = t; }
uint32_t test_get_milisTicks(void)   { return milisTicks; }
#endif
```

These guards compile to nothing in the firmware build (`-DTEST` is never passed to SEGGER ES). Everything else — stubs, fakes, Makefile targets — lives entirely inside `tests/` and is never compiled into the firmware.

---

### Building and running tests

From `GaggiaController\` (project root):

```powershell
# Phase 1 — pure logic (no stubs needed)
make -f tests/Makefile test_numbers
make -f tests/Makefile test_digital_filters
make -f tests/Makefile test_pid_block

# Or build and run all at once:
make -f tests/Makefile all
```

Expected Unity output (all passing):
```
tests/test_numbers.c:42:test_Constrain_ValueWithinLimits:PASS
tests/test_numbers.c:55:test_Constrain_ValueBelowLower:FAIL: Expected -1 Was 1
...
-----------------------
9 Tests 1 Failures 0 Ignored
```

The failure on `test_Constrain_ValueBelowLower` is the **first Red test** — documenting the known bug in `fcn_Constrain_WithinFloats`. Fix the bug in `x04_Numbers.c`, re-run, confirm Green.

---

### Verification that the infrastructure is correctly set up

| Check | Command | Expected result |
|-------|---------|----------------|
| gcc available | `gcc --version` | Prints version ≥ 13 |
| make available | `make --version` | Prints GNU Make ≥ 4 |
| Unity compiles | `make -f tests/Makefile test_numbers` | `test_numbers.exe` produced, 0 linker errors |
| FFF header found | `make -f tests/Makefile test_pump_controller` | No `fff.h: No such file` error |
| Stubs shadow SDK | `make -f tests/Makefile test_temp_controller` | No `nrf_drv_timer.h: No such file` error |
| Tests run | `.\test_numbers.exe` | Unity summary line printed to stdout |
| No firmware regression | Build firmware in SEGGER ES | 0 new errors; same memory footprint |

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

| File | Change | Purpose | Phase |
|------|--------|---------|-------|
| `tempController.c` | Add `#ifdef TEST` accessor for `static milisTicks` | Let tests control PID timing | 2 |
| `tempController.c` | Add sensor range check in `fcn_updateTemperatureController()` | Fix H3 | 2 |
| `tempController.c` | Reset `HistoryError` in `fcn_loaddSetPoint_ParamToCtrl_Temp()` | Fix M1 | 2 |
| `PumpController.c` | Add zero-time guard in `fcn_LoadPumpParam()` | Fix H1 | 2 |
| `BLEspressoServices.c` | Add overheat cutoff in step function `sf_Mode_2b` | Fix H4 | 2 |
| `BLEspressoServices.c` | Add `MAX_BREW_TICKS` timeout in `cl_Mode_1` and profile states | Fix H5 | 2/3 |
| `BLEspressoServices.c` | Add `#ifdef TEST` accessor for `serviceTick` | Let tests control brew timer | 2/3 |
| `BLEspressoServices.c` | Revert phase2 I-gain before re-boost in `cl_idle`/`prof_idle` | Fix M5 | 2/3 |
| `StorageController.c` | `#include "ProfileValidator.h"`; call `fcn_ValidateAndClampProfile()` in `stgCtrl_ReadUserData()` | Fix M3 | 2/3 |
| `bluetooth_drv.c` | `#include "ProfileValidator.h"`; call `fcn_ValidateFloat_InRange()` after GATT writes | Fix H2 (integration) | 2/3 |
| **New:** `ProfileValidator.c` + `.h` | Pure-logic validation — clamp profile fields to safe ranges | Fix H2 | 2/3 |

**All test isolation is through stubs + FFF fakes + include path ordering. No other source files modified.**

---

## Known Issue → TDD Traceability

Tests in this plan that target known issues from [TEST_PLAN.md](TEST_PLAN.md):

| Issue | Severity | Description | TDD Test | Test File |
|-------|----------|-------------|----------|-----------|
| H1 | HIGH | Division by zero in pump slope calc when ramp time = 0 | `test_LoadParams_ZeroTime_NoCrash` | `test_pump_controller.c` |
| H2 | HIGH | No BLE input validation — any float accepted | `test_H2_*` (13 tests) | `test_profile_validator.c` (Phase 2/3) |
| H3 | HIGH | No temperature sensor failure detection (open/short) | `test_SensorFailure_OpenCircuit`, `test_SensorFailure_ShortCircuit` | `test_temp_controller.c` |
| H4 | HIGH | No overheat protection in step-function mode | `test_StepFunction_OverheatProtection` | `test_blespresso_services.c` |
| H5 | HIGH | No maximum brew time limit in profile mode | `test_H5_*` (3 tests) | `test_blespresso_services.c` (Phase 2/3) |
| M1 | MEDIUM | No integral reset on setpoint change (brew→steam overshoot) | `test_SetpointSwitch_IntegralReset` | `test_temp_controller.c` |
| M3 | MEDIUM | No NVM data validation — unsafe params load silently from corrupt/blank flash | `test_M3_*` (4 tests, replaces 1 existing) | `test_storage_controller.c` (Phase 2/3) |
| M5 | MEDIUM | I-gain boost reapplied on rapid brew cycling | `test_M5_*` (3 tests) | `test_blespresso_services.c` (Phase 2/3) |

---

## Relevant Files

| File | Role |
|------|------|
| `ble_espresso_app/components/Utilities/x04_Numbers.c` | Phase 1 test target — math utilities |
| `ble_espresso_app/components/Utilities/x201_DigitalFiltersAlgorithm.c` | Phase 1 test target — RC filter |
| `ble_espresso_app/components/Utilities/x205_PID_Block.c` | Phase 1 test target — PID algorithm |
| `ble_espresso_app/components/Application/tempController.c` | Phase 2 test target — modify for H3, M1 |
| `ble_espresso_app/components/Application/PumpController.c` | Phase 2 test target — modify for H1 |
| `ble_espresso_app/components/Application/BLEspressoServices.c` | Phase 2 test target — modify for H4; Phase 2/3 — modify for H5, M5 |
| `ble_espresso_app/components/Application/StorageController.c` | Phase 2 test target — modify for M3 fix (Phase 2/3) |
| `ble_espresso_app/components/Application/ProfileValidator.c` | Phase 2/3 — new file for H2 validation logic |
| `ble_espresso_app/components/Application/ProfileValidator.h` | Phase 2/3 — public API for validation |
| `ble_espresso_app/components/BLE/bluetooth_drv.c` | Phase 2/3 — integrate H2 validation calls (not tested on host due to BLE SDK deps) |
| `ble_espresso_app/components/Application/StorageController.h` | NVM status enum (`storageCtrl_status_t`) |
| `ble_espresso_app/components/Application/BLEspressoServices.h` | `bleSpressoUserdata_struct` definition |
| `ble_espresso_app/components/Peripherals/include/solidStateRelay_Controller.h` | FFF fake target / CMock mock target |
| `ble_espresso_app/components/Peripherals/include/spi_Devices.h` | FFF fake target / CMock mock target |
| `ble_espresso_app/components/Peripherals/include/ac_inputs_drv.h` | FFF fake target / CMock mock target |
| `tests_ceedling/project.yml` | Phase 3 — Ceedling configuration |

---

## Verification

1. **Phase 1 build:** Run `make -f tests/Makefile phase1` — all compile with 0 warnings
2. **Phase 1 pass:** All Unity tests report `0 Failures`
3. **Phase 2 build:** Run `make -f tests/Makefile phase2` — all compile with stubs
4. **Phase 2 known-issue tests:** H1, H3, H4, M1 initially fail (Red) → source fixes → pass (Green); M3 initially passes documenting current broken behavior
5. **Phase 2/3 build:** Run `make -f tests/Makefile phase2_3` — all compile
6. **Phase 2/3 known-issue tests:** H2, H5, M5 initially fail (Red) → source fixes → pass (Green); M3 test updated to assert clamped behavior (was documented → now enforced)
7. **Regression:** Re-run all Phase 1+2+2/3 tests after any source fix → 0 Failures across all suites
8. **Phase 3 build:** Run `ceedling test:all` — all Ceedling tests pass with strict call ordering
9. **Phase 3 coverage:** Run `ceedling gcov:all` — all modules at or above coverage targets
10. **No firmware breakage:** Build production firmware in SEGGER ES after all source changes → compiles and links within 92KB app window

---

## Decisions

- **Framework:** Unity (lightweight, embedded-C native) + FFF (header-only fakes) for Phases 1–2/3. Ceedling + CMock for Phase 3 (auto-generated mocks, strict ordering, coverage).
- **Build:** Plain Makefile + gcc for Phases 1–2/3. Ceedling (Ruby-based) for Phase 3. Both work on Windows (MinGW) and WSL.
- **Scope:** Phase 1 (pure logic) + Phase 2 (application logic with mocks) + Phase 2/3 (remaining application issues: H2, H5, M5, M3 fix) + Phase 3 (Ceedling migration). On-target testing (Phases 4–6) deferred.
- **TDD on existing code:** Use "characterization test" approach — write tests for current behavior first, then write failing tests for known issues, then fix.
- **Known issues as TDD targets:** H1 (div-by-zero), H3 (sensor failure), H4 (overheat), H5 (infinite brew), M1 (integral reset) — each becomes a Red-Green-Refactor cycle.
- **Excluded from Phase 2:** BLE input validation (H2), watchdog (C1), GPIO boot state (C3), atomic BLE update (M4) — H2 is addressed in Phase 2/3 via an extracted validation function; C1, C3, M4 require on-target testing (Phase 4+).

---

## Phase 2/3: Remaining Application Issues (Host PC, Unity + FFF)

> **Prerequisite:** Phase 2 complete (34/34 tests passing). All infrastructure (stubs, fakes, Makefile) already in place.

Phase 2/3 addresses the three deferred high/medium issues that are testable on the host but were not resolved in Phase 2: **H2** (BLE input validation), **H5** (no maximum brew time), **M5** (I-boost stacking on rapid cycling), plus the **M3 fix** (NVM validation — currently only documented, not fixed).

### Why a separate phase?

These issues require **new production code** (validation functions, timeout logic, cooldown guards) rather than simple bug fixes. Each is a full Red-Green-Refactor cycle where the test is written first, the feature is implemented to make it pass, and then cleaned up.

```
Phase 2 (complete)                Phase 2/3 (this section)
┌────────────────────┐            ┌────────────────────────┐
│ H1 ✅  H3 ✅  H4 ✅│            │ H2 — BLE validation    │
│ M1 ✅  M3 documented│   ──►     │ H5 — Max brew time     │
│ 34 tests passing   │            │ M5 — I-boost cooldown  │
└────────────────────┘            │ M3 — NVM range clamp   │
                                  │ +N new tests           │
                                  └────────────────────────┘
```

---

### Issue H2 — BLE Input Validation

**Problem:** `bluetooth_drv.c:cus_evt_handler()` stores every BLE GATT write directly into `blEspressoProfile` with zero validation. A malicious or buggy BLE client can write `NaN`, `Inf`, negative times, or temperatures above the safety thermostat.

**Challenge:** `bluetooth_drv.c` includes 15+ BLE SDK headers (`ble.h`, `ble_hci.h`, `ble_srv_common.h`, `nrf_sdh.h`, `fds.h`, `peer_manager.h`, etc.) that are impractical to stub for host compilation. Testing `cus_evt_handler` directly is not feasible in Phase 2/3.

**Strategy:** Extract a pure-logic validation function into a new compilation unit that can be tested on the host. The BLE handler calls it after each GATT write.

#### New file: `ProfileValidator.c` / `ProfileValidator.h`

```c
/* ProfileValidator.h */
#ifndef PROFILE_VALIDATOR_H__
#define PROFILE_VALIDATOR_H__

#include "BLEspressoServices.h"
#include <stdbool.h>

typedef enum {
    PROFILE_VALID  = 0,
    PROFILE_CLAMPED,        /* one or more fields were out of range and clamped */
    PROFILE_REJECTED        /* NaN or Inf detected — all fields set to defaults */
} profileValidation_status_t;

profileValidation_status_t fcn_ValidateAndClampProfile(
    bleSpressoUserdata_struct *profile);

/* Single-field validation (called by BLE handler after each GATT write) */
bool fcn_ValidateFloat_InRange(float *value, float min, float max, float safeDefault);

#endif
```

```c
/* ProfileValidator.c */
#include "ProfileValidator.h"
#include <math.h>

/* --- Safe ranges and defaults -------------------------------------------- */
typedef struct {
    float *field;
    float  min;
    float  max;
    float  safeDefault;
} field_range_t;

bool fcn_ValidateFloat_InRange(float *value, float min, float max, float safeDefault)
{
    if (isnan(*value) || isinf(*value) || *value < min || *value > max) {
        *value = safeDefault;
        return false;   /* was out of range */
    }
    return true;        /* valid */
}

profileValidation_status_t fcn_ValidateAndClampProfile(
    bleSpressoUserdata_struct *profile)
{
    bool all_valid = true;
    /* Temperature */
    all_valid &= fcn_ValidateFloat_InRange(&profile->temp_Target,      20.0f, 110.0f,  93.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->sp_BrewTemp,      20.0f, 110.0f,  93.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->sp_StemTemp,      100.0f, 160.0f, 130.0f);
    /* Profile — power (0–100%) */
    all_valid &= fcn_ValidateFloat_InRange(&profile->prof_preInfusePwr, 0.0f, 100.0f,  50.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->prof_InfusePwr,    0.0f, 100.0f, 100.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->Prof_DeclinePwr,   0.0f, 100.0f,  60.0f);
    /* Profile — timers (seconds) */
    all_valid &= fcn_ValidateFloat_InRange(&profile->prof_preInfuseTmr, 0.0f,  15.0f,   3.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->prof_InfuseTmr,    0.0f,  60.0f,  25.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->Prof_DeclineTmr,   0.0f,  30.0f,  10.0f);
    /* PID gains */
    all_valid &= fcn_ValidateFloat_InRange(&profile->Pid_P_term,       0.0f, 100.0f,   9.5f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->Pid_I_term,       0.0f,  10.0f,   0.3f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->Pid_Iboost_term,  0.0f,  20.0f,   6.5f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->Pid_Imax_term,    0.0f, 500.0f, 100.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->Pid_D_term,       0.0f,  50.0f,   0.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->Pid_Dlpf_term,    0.0f,   1.0f,   0.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->Pid_Gain_term,    0.01f, 10.0f,   1.0f);

    return all_valid ? PROFILE_VALID : PROFILE_CLAMPED;
}
```
##### file: `ProfileValidator.c` / `ProfileValidator.h` -> Relocated to different modules
bool fcn_ValidateFloat_InRange(float *value, float min, float max, float safeDefault) - Moved to x04_Numbers.h
profileValidation_status_t fcn_ValidateAndClampProfile(bleSpressoUserdata_struct *profile) - Moved to StorageController.h

#### Integration point in `bluetooth_drv.c`

After each GATT write case in `cus_evt_handler`, call the single-field validator:

```c
case BLE_BOILER_CHAR_EVT_NEW_TEMPERATURE:
    blEspressoProfile.temp_Target = (float) fcn_ChrArrayToFloat(...);
    fcn_ValidateFloat_InRange(&blEspressoProfile.temp_Target, 20.0f, 110.0f, 93.0f);
    break;
```

Or call `fcn_ValidateAndClampProfile` once at the end of `cus_evt_handler` for batch validation.

#### Test file: `test_profile_validator.c`

| Test Name | Stimulus | Expected | Issue |
|-----------|----------|----------|-------|
| `test_H2_ValidProfile_ReturnsValid` | All fields within range | Returns `PROFILE_VALID`, fields unchanged | H2 baseline |
| `test_H2_TempTarget_NaN_ClampedToDefault` | `temp_Target = NAN` | `temp_Target = 93.0`, returns `PROFILE_CLAMPED` | H2 |
| `test_H2_TempTarget_Inf_ClampedToDefault` | `temp_Target = INFINITY` | `temp_Target = 93.0`, returns `PROFILE_CLAMPED` | H2 |
| `test_H2_TempTarget_Negative_ClampedToDefault` | `temp_Target = -50.0` | `temp_Target = 93.0`, returns `PROFILE_CLAMPED` | H2 |
| `test_H2_TempTarget_TooHigh_ClampedToDefault` | `temp_Target = 200.0` | `temp_Target = 93.0`, returns `PROFILE_CLAMPED` | H2 |
| `test_H2_TempTarget_AtBoundary_Valid` | `temp_Target = 20.0` and `110.0` | Fields unchanged, returns `PROFILE_VALID` | H2 boundary |
| `test_H2_PidPterm_Negative_Clamped` | `Pid_P_term = -1.0` | `Pid_P_term = 9.5`, returns `PROFILE_CLAMPED` | H2 |
| `test_H2_PidIterm_TooHigh_Clamped` | `Pid_I_term = 50.0` | `Pid_I_term = 0.3`, returns `PROFILE_CLAMPED` | H2 |
| `test_H2_ProfTimerZero_Valid` | `prof_preInfuseTmr = 0.0` | Field unchanged (0.0 is in range) | H2 boundary |
| `test_H2_ProfDeclineTmr_Negative_Clamped` | `Prof_DeclineTmr = -5.0` | `Prof_DeclineTmr = 10.0` | H2 |
| `test_H2_MultipleFieldsInvalid_AllClamped` | 3 fields NaN | All 3 clamped to defaults, rest unchanged | H2 batch |
| `test_H2_SingleField_InRange_Valid` | `fcn_ValidateFloat_InRange(50.0, 0, 100, ...)` | Returns `true`, value unchanged | H2 unit |
| `test_H2_SingleField_OutOfRange_Clamped` | `fcn_ValidateFloat_InRange(150.0, 0, 100, 50.0)` | Returns `false`, value = 50.0 | H2 unit |

**Field range table** (used by both H2 and M3 fix):

| Field | Min | Max | Safe Default | Unit |
|-------|-----|-----|-------------|------|
| `temp_Target` | 20.0 | 110.0 | 93.0 | °C |
| `sp_BrewTemp` | 20.0 | 110.0 | 93.0 | °C |
| `sp_StemTemp` | 100.0 | 160.0 | 130.0 | °C |
| `prof_preInfusePwr` | 0.0 | 100.0 | 50.0 | % |
| `prof_preInfuseTmr` | 0.0 | 15.0 | 3.0 | s |
| `prof_InfusePwr` | 0.0 | 100.0 | 100.0 | % |
| `prof_InfuseTmr` | 0.0 | 60.0 | 25.0 | s |
| `Prof_DeclinePwr` | 0.0 | 100.0 | 60.0 | % |
| `Prof_DeclineTmr` | 0.0 | 30.0 | 10.0 | s |
| `Pid_P_term` | 0.0 | 100.0 | 9.5 | — |
| `Pid_I_term` | 0.0 | 10.0 | 0.3 | — |
| `Pid_Iboost_term` | 0.0 | 20.0 | 6.5 | — |
| `Pid_Imax_term` | 0.0 | 500.0 | 100.0 | — |
| `Pid_D_term` | 0.0 | 50.0 | 0.0 | — |
| `Pid_Dlpf_term` | 0.0 | 1.0 | 0.0 | — |
| `Pid_Gain_term` | 0.01 | 10.0 | 1.0 | — |

---

### Issue H5 — Maximum Brew Time Limit

**Problem:** Neither classic mode (`cl_Mode_1`) nor profile mode (`prof_Mode` → `prof_Mode_Decline`) has a time limit. If the brew switch stays asserted (stuck relay, jammed switch, user forgets), the pump and solenoid run indefinitely. This is both a safety and equipment damage risk.

**Root cause analysis:**
- Classic mode: `cl_Mode_1` only checks `swBrew == AC_SWITCH_ASSERTED` with no timeout. `classicData.svcStartT` is saved at brew start but used only for logging.
- Profile mode: the profiler walks through pre-infusion → infusion → decline phases with computed `tickTabTarget` values. When all phases finish, `prof_Mode_max` resets to `prof_idle`. If the phases take longer than expected or if the switch stays asserted after the final phase, `prof_idle` re-triggers the entire profile sequence indefinitely.

**Fix strategy:** Add a `MAX_BREW_TICKS` constant to `BLEspressoServices.c` and check elapsed time in every active brew state.

```c
/* BLEspressoServices.c — new define */
#define MAX_BREW_TICKS   (120000 / SERVICE_BASE_T_MS)   /* 120s = 1200 ticks @ 100ms */
```

Classic mode fix in `cl_Mode_1`:
```c
case cl_Mode_1:
    /* H5: enforce maximum brew duration */
    if ((serviceTick - classicData.svcStartT) >= MAX_BREW_TICKS) {
        /* Auto-stop: same shutdown sequence as switch release */
        espressoService_Status.sRunning = cl_idle;
        classicData.pumpPwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(classicData.pumpPwr);
        fcn_multiplyI_ParamToCtrl_Temp(..., 2.0f);
        classicData.b_boostI_phase1 = false;
        classicData.b_boostI_phase2 = true;
        fcn_SolenoidSSR_Off();
        break;
    }
    /* ... existing switch-release logic ... */
```

Profile mode fix — same guard added at the top of `prof_Mode`, `prof_Mode_Infuse`, `prof_Mode_Decline`, and `prof_Mode_Halt`:
```c
case prof_Mode:
    /* H5: enforce maximum brew duration */
    if ((serviceTick - profileData.svcStartT) >= MAX_BREW_TICKS) {
        /* Auto-stop */
        profileService_Status.sRunning = prof_idle;
        fcn_pumpSSR_pwrUpdate(PUMP_PWR_OFF);
        fcn_SolenoidSSR_Off();
        profileData.b_boostI_phase1 = false;
        profileData.b_boostI_phase2 = true;
        fcn_multiplyI_ParamToCtrl_Temp(..., 2.0f);
        break;
    }
    /* ... existing profile logic ... */
```

#### Tests (added to `test_blespresso_services.c`)

| Test Name | Stimulus | Expected | Issue |
|-----------|----------|----------|-------|
| `test_H5_ClassicBrew_AutoStops_After120s` | Brew switch asserted; advance `serviceTick` by 1200+ ticks (120s) | `fcn_pumpSSR_pwrUpdate(0)` called, `fcn_SolenoidSSR_Off` called | H5 |
| `test_H5_ClassicBrew_StillRunning_Before120s` | Brew switch asserted; advance 1199 ticks | Pump still running (power > 0) | H5 boundary |
| `test_H5_ProfileBrew_AutoStops_After120s` | Profile brew start; advance `serviceTick` by 1200+ ticks | Pump stopped, solenoid off, state = `prof_idle` | H5 |

**Testing approach:** The existing `serviceTick` is a `static uint16_t` inside `BLEspressoServices.c` that increments on every call to `fcn_svc_EspressoApp()` or `fcn_svc_ProfileApp()`. Tests must drive enough ticks by calling the service function in a loop. For efficiency, an `#ifdef TEST` accessor may be added:

```c
/* BLEspressoServices.c */
#ifdef TEST
static uint16_t serviceTick = 0;
void test_set_serviceTick(uint16_t t) { serviceTick = t; }
uint16_t test_get_serviceTick(void) { return serviceTick; }
#endif
```

This allows tests to jump the tick counter close to the timeout boundary instead of calling the service function 1200 times.

**Implementation note:** `serviceTick` is `uint16_t` (max 65535). At 100ms per tick, this overflows after ~109 minutes. The timeout check must handle wraparound: `(uint16_t)(serviceTick - svcStartT) >= MAX_BREW_TICKS`. This naturally works with unsigned subtraction modular arithmetic.

---

### Issue M5 — I-Gain Boost Rapid Cycling

**Problem:** When the user toggles the brew switch ON → OFF → ON rapidly (before `b_boostI_phase2` recovery completes), the `cl_idle` state calls `fcn_loadIboost_ParamToCtrl_Temp` again, stacking the boost (Ki×6.5) on top of the already-elevated Ki×2 from phase2 recovery. This causes excessive integral accumulation and temperature overshoot.

**Root cause in code:**

```c
/* cl_idle → brew start (line ~248) */
fcn_loadIboost_ParamToCtrl_Temp(&blEspressoProfile);   /* Ki = Pid_Iboost_term (6.5×) */
classicData.b_normalI = false;
classicData.b_boostI_phase1 = true;
/* BUT: classicData.b_boostI_phase2 may still be true from previous brew! */
```

The same pattern exists in `prof_idle` for profile mode.

**Fix strategy — Option A (revert before re-boost):** Before applying I-boost on brew start, check if phase2 recovery is still active and revert to 1× first:

```c
case cl_idle:
    if (swBrew == AC_SWITCH_ASSERTED) {
        /* M5 fix: revert phase2 recovery gain before applying new boost */
        if (classicData.b_boostI_phase2) {
            fcn_multiplyI_ParamToCtrl_Temp(&blEspressoProfile, 1.0f);
            classicData.b_boostI_phase2 = false;
        }
        classicData.svcStartT = serviceTick;
        fcn_loadIboost_ParamToCtrl_Temp(&blEspressoProfile);
        /* ... rest unchanged ... */
```

**Fix strategy — Option B (minimum cooldown):** Add a `MIN_BREW_COOLDOWN_TICKS` (e.g. 50 = 5s at 100ms). Ignore brew start if `(serviceTick - lastBrewStopTick) < MIN_BREW_COOLDOWN_TICKS`. This is simpler but blocks rapid intentional brew starts.

**Recommendation:** Option A — it fixes the root cause (stacked gain) without restricting the user.

#### Tests (added to `test_blespresso_services.c`)

| Test Name | Stimulus | Expected | Issue |
|-----------|----------|----------|-------|
| `test_M5_RapidCycling_IboostNotStacked` | Brew ON → OFF → ON (before phase2 recovery completes) | `fcn_multiplyI_ParamToCtrl_Temp` called with `1.0f` before second `fcn_loadIboost_ParamToCtrl_Temp` | M5 |
| `test_M5_NormalCycling_IboostAppliedOnce` | Brew ON → OFF → wait for phase2 recovery → ON | `fcn_loadIboost_ParamToCtrl_Temp` called once per brew, no `multiplyI(1.0)` between them | M5 baseline |
| `test_M5_ProfileMode_RapidCycling_NoCrash` | Profile brew ON → OFF → ON rapidly | No crash, I-boost applied cleanly | M5 profile mode |

**Testing approach:** Tests manipulate `classicData.b_boostI_phase2` indirectly by driving the state machine through a brew ON → OFF cycle (which sets `b_boostI_phase2 = true`), then immediately starting another brew. The test asserts on the sequence of `fcn_multiplyI_ParamToCtrl_Temp` and `fcn_loadIboost_ParamToCtrl_Temp` fake call histories.

---

### Issue M3 Fix — NVM Data Range Validation

**Problem (documented in Phase 2):** `stgCtrl_ReadUserData()` deserializes all float fields from raw NVM bytes with no range validation. Corrupted flash or a partial write could load unsafe parameters (e.g., `temp_Target = 999 °C`). Phase 2 test `test_M3_OutOfRange_Temp_Loaded_Without_Validation` captures this current behaviour.

**Fix strategy:** Call `fcn_ValidateAndClampProfile()` (from the new `ProfileValidator.c` — see H2 above) at the end of `stgCtrl_ReadUserData()` after all fields are deserialized. If any field is out of range, it gets clamped to the safe default.

```c
/* StorageController.c — at end of stgCtrl_ReadUserData() */
storageCtrl_status_t stgCtrl_ReadUserData(bleSpressoUserdata_struct *ptr_rxData)
{
    /* ... existing deserialization ... */
    if (keyStatus == STORAGE_USERDATA_LOADED) {
        /* M3 fix: validate all fields, clamp out-of-range to safe defaults */
        fcn_ValidateAndClampProfile(ptr_rxData);
    }
    return keyStatus;
}
```

#### Tests (update existing + add new in `test_storage_controller.c`)

| Test Name | Stimulus | Expected | Issue |
|-----------|----------|----------|-------|
| `test_M3_OutOfRange_Temp_Clamped_To_Default` | Valid key, `temp_Target = 999.0f` in NVM bytes | `stgCtrl_ReadUserData` returns `LOADED`, `temp_Target = 93.0f` (default) | M3 (Green) |
| `test_M3_NaN_In_NVM_Clamped` | Valid key, NaN bytes in `Pid_P_term` position | `Pid_P_term = 9.5f` (default) | M3 |
| `test_M3_NegativeTimer_Clamped` | Valid key, `prof_preInfuseTmr = -5.0f` | `prof_preInfuseTmr = 3.0f` (default) | M3 |
| `test_M3_AllFieldsValid_NoChange` | Valid key, all fields in range | Fields unchanged | M3 regression |

**Note:** The existing `test_M3_OutOfRange_Temp_Loaded_Without_Validation` must be **updated** to expect the clamped value once the fix is applied. This is the Red→Green transition for M3.

---

### Phase 2/3 — Makefile Changes

New Makefile target and source file:

```makefile
# ProfileValidator: pure-logic validation, no hardware deps
SRC_VALIDATOR = ble_espresso_app/components/Application/ProfileValidator.c

# Phase 2/3 targets
phase2_3: $(OUTDIR)/test_profile_validator.exe
	@echo.
	@echo === Running Phase 2/3 Tests ===
	-$(OUTDIR)/test_profile_validator.exe
	@echo.
	@echo === Phase 2/3 Complete ===

$(OUTDIR)/test_profile_validator.exe: \
        tests/test_profile_validator.c \
        $(SRC_VALIDATOR) \
        $(SRC_UNITY)
	$(CC) $(CFLAGS2) $^ -o $@ $(LDFLAGS)
```

The H5 and M5 tests are added to the existing `test_blespresso_services.c` and `test_storage_controller.c` files — no new test executables needed for those. The M3 fix tests go in `test_storage_controller.c` (already compiled with `StorageController.c`). `StorageController.c` must be updated to `#include "ProfileValidator.h"` and the validator object file linked:

```makefile
$(OUTDIR)/test_storage_controller.exe: \
        tests/test_storage_controller.c \
        $(SRC_STORAGE) \
        $(SRC_VALIDATOR) \
        $(SRC_UNITY)
	$(CC) $(CFLAGS2) $^ -o $@ $(LDFLAGS)
```

### Phase 2/3 — New Files

| File | Purpose |
|------|---------|
| `ble_espresso_app/components/Application/ProfileValidator.c` | Validation logic — clamp fields to safe ranges |
| `ble_espresso_app/components/Application/ProfileValidator.h` | Public API: `fcn_ValidateAndClampProfile`, `fcn_ValidateFloat_InRange` |
| `tests/test_profile_validator.c` | 13 tests for H2 validation |

### Phase 2/3 — Source Modifications

| File | Change | Purpose |
|------|--------|---------|
| `BLEspressoServices.c` | Add `MAX_BREW_TICKS` define; add timeout check in `cl_Mode_1`, `prof_Mode`, `prof_Mode_Infuse`, `prof_Mode_Decline`; add `#ifdef TEST` accessor for `serviceTick` | Fix H5 |
| `BLEspressoServices.c` | In `cl_idle` and `prof_idle` brew-start: check `b_boostI_phase2` and revert to 1× before applying boost | Fix M5 |
| `StorageController.c` | `#include "ProfileValidator.h"`; call `fcn_ValidateAndClampProfile()` at end of `stgCtrl_ReadUserData()` | Fix M3 |
| `bluetooth_drv.c` | `#include "ProfileValidator.h"`; call `fcn_ValidateFloat_InRange()` after each GATT write | Fix H2 (production integration — not tested in Phase 2/3 due to BLE SDK deps, but validated via `test_profile_validator.c`) |

### Phase 2/3 — Verification

1. **All existing tests still pass:** `make -f tests/Makefile all` → 66/66 (Phase 1 + Phase 2) with 0 regressions
2. **New tests pass:** `make -f tests/Makefile phase2_3` → all H2/H5/M5/M3 tests green
3. **M3 test updated:** `test_M3_OutOfRange_Temp_Loaded_Without_Validation` renamed/updated to assert clamped value
4. **No firmware breakage:** Build in SEGGER ES → compiles within 92KB app window

### Phase 2/3 — Known Issue Traceability

| Issue | Severity | TDD Test | Test File | Status after Phase 2/3 |
|-------|----------|----------|-----------|----------------------|
| H2 | HIGH | `test_H2_*` (13 tests) | `test_profile_validator.c` | ✅ Fixed (validation logic tested; BLE handler integration verified on-target in Phase 4) |
| H5 | HIGH | `test_H5_*` (3 tests) | `test_blespresso_services.c` | ✅ Fixed |
| M3 | MEDIUM | `test_M3_*` (4 tests, replaces 1 existing) | `test_storage_controller.c` | ✅ Fixed (was documented, now enforced) |
| M5 | MEDIUM | `test_M5_*` (3 tests) | `test_blespresso_services.c` | ✅ Fixed |

---

## Phase 3: Ceedling + CMock Migration (Host PC, auto-generated mocks)

> **Prerequisite:** Phase 2/3 complete. All Unity + FFF tests passing. Production code fixes for H1–H5, M1, M3, M5 applied.
>
> **Reference:** [TEST_PLAN.md — Phase 3](TEST_PLAN.md) defines Phase 3 scope as a Ceedling + CMock migration of Phase 2 tests with strict call-ordering verification and code coverage.

### Why migrate to Ceedling + CMock?

Phase 2 uses FFF (Fake Function Framework) — a lightweight, manual approach where each fake is declared explicitly and call history is checked after the fact. This works well for initial TDD but has limitations:

| Aspect | Phase 2 (FFF) | Phase 3 (CMock) |
|--------|---------------|-----------------|
| Mock creation | Manual `FAKE_*_FUNC` macros in `fakes.h` | Auto-generated from header files |
| Call verification | Post-hoc: check `fake.call_count`, `fake.arg0_history[]` | Pre-declared: `_Expect()`, `_ExpectAndReturn()` with strict ordering |
| Unexpected calls | Silent — ignored unless test explicitly checks | Immediate test failure |
| Build system | Manual Makefile, manual include paths | Ceedling manages compilation, linking, test discovery |
| Coverage | Manual gcov invocation | `ceedling gcov:all` built-in |
| Test discovery | Hand-written `main()` with `RUN_TEST()` | Automatic — any `void test_*()` function is found and run |
| Maintenance | Every new fake function must be added to `fakes.h` | Regenerated from headers automatically |

### What Phase 3 does NOT change

- **Test logic:** Same stimulus-expected-assert structure. Test names and assertions carry over.
- **Production code:** No source changes. All Phase 2/3 fixes remain.
- **Stub headers:** Same nRF SDK stubs in `tests/stubs/` — reused by Ceedling.

### Step 1 — Install Ceedling

```powershell
# Ceedling requires Ruby
winget install -e --id RubyInstallerTeam.Ruby.3.2
# Restart terminal, then:
gem install ceedling
ceedling version   # → Ceedling 1.0.0 or newer
```

### Step 2 — Initialize Ceedling project

```powershell
cd C:\WS\NRF\GaggiaController
ceedling new tests_ceedling --local
```

This creates `tests_ceedling/` with `project.yml`, `src/`, `test/`, and `build/` directories.

### Step 3 — Configure `project.yml`

```yaml
:project:
  :build_root: tests_ceedling/build
  :test_file_prefix: test_
  :use_test_preprocessor: true

:defines:
  :test:
    - TEST
    - HOST
    - NRF_LOG_ENABLED=0
    - SERVICE_HEAT_ACTION_EN=1
    - SERVICE_PUMP_ACTION_EN=1

:paths:
  :source:
    - ble_espresso_app/components/Utilities/**
    - ble_espresso_app/components/Application/**
  :include:
    - tests/stubs                                           # ← FIRST: shadows real SDK
    - ble_espresso_app/components/Utilities/include
    - ble_espresso_app/components/Application
    - ble_espresso_app/components/Peripherals/include
    - ble_espresso_app/components/BLE/include
  :test:
    - tests_ceedling/test/**

:cmock:
  :mock_prefix: mock_
  :enforce_strict_ordering: true
  :treat_externs: :include
  :when_no_prototypes: :warn
  :plugins:
    - :ignore
    - :callback
    - :return_thru_ptr
  :strippables:
    - '(?:__attribute__\s*\(+.*?\)+)'

:plugins:
  :enabled:
    - gcov
    - stdout_pretty_tests_report
    - module_generator

:gcov:
  :reports:
    - HtmlDetailed
  :gcovr:
    :html_medium_threshold: 75
    :html_high_threshold: 90
```

**Key configuration notes:**

- `:use_test_preprocessor: true` — CMock uses the preprocessor to parse headers and generate mocks. This must be enabled.
- `:paths:include` — `tests/stubs` listed first so stub headers shadow real SDK headers, same as the Makefile approach.
- `:cmock:enforce_strict_ordering: true` — calls must happen in the order specified by `_Expect()` declarations, or the test fails immediately.
- `:cmock:plugins: [:ignore, :callback, :return_thru_ptr]` — `:ignore` allows `_Ignore()` for functions we don't care about in a given test; `:callback` replaces FFF's custom-fake pattern; `:return_thru_ptr` allows injecting values through pointer parameters.

### Step 4 — Create CMock-based test files

Each Phase 2 test file is ported. The key change is replacing `#include "fakes.h"` with `#include "mock_<header>.h"`:

#### `test_temp_controller.c` (CMock version)

```c
#include "unity.h"
#include "tempController.h"
#include "mock_solidStateRelay_Controller.h"
#include "mock_spi_Devices.h"
#include "mock_ac_inputs_drv.h"
#include "mock_bluetooth_drv.h"

/* Real PID/filter/numbers compiled by Ceedling from :source paths */

volatile bleSpressoUserdata_struct blEspressoProfile;

void setUp(void) {
    memset((void*)&blEspressoProfile, 0, sizeof(blEspressoProfile));
    blEspressoProfile.temp_Target = 93.0f;
    blEspressoProfile.Pid_P_term = 9.5f;
    blEspressoProfile.Pid_I_term = 0.3f;
    /* CMock automatically resets all expectations between tests */
}

void tearDown(void) {}

void test_TC_LoadPID_Valid_Returns_OK(void) {
    blEspressoProfile.Pid_P_term = 9.5f;
    blEspressoProfile.Pid_I_term = 0.1f;
    TEST_ASSERT_EQUAL(TEMPCTRL_LOAD_OK,
        fcn_loadPID_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile));
}

void test_H3_OpenSensor_ShutoffHeater(void) {
    fcn_loadPID_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile);
    blEspressoProfile.temp_Boiler = 0.0f;  /* open sensor */
    test_set_milisTicks(60000);

    /* CMock strict ordering: expect NO call to fcn_boilerSSR_pwrUpdate
       (because sensor guard returns 0 before any SSR call) */
    float result = fcn_updateTemperatureController(
        (bleSpressoUserdata_struct*)&blEspressoProfile);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, result);
}
```

#### `test_pump_controller.c` (CMock version)

```c
#include "unity.h"
#include "PumpController.h"
#include "mock_solidStateRelay_Controller.h"

void setUp(void) {
    /* CMock: declare expected calls for init */
    get_SolenoidSSR_State_IgnoreAndReturn(SSR_STATE_ENGAGE);
    fcn_pumpSSR_pwrUpdate_Ignore();
    fcn_SolenoidSSR_On_Ignore();
    fcn_SolenoidSSR_Off_Ignore();
    fcn_initPumpController();
}

void test_H1_ZeroDeclineTime_NoCrash_Returns_OK(void) {
    bleSpressoUserdata_struct profile = {0};
    profile.prof_preInfusePwr = 50.0f;
    profile.prof_preInfuseTmr = 3.0f;
    profile.prof_InfusePwr    = 100.0f;
    profile.prof_InfuseTmr    = 25.0f;
    profile.Prof_DeclinePwr   = 60.0f;
    profile.Prof_DeclineTmr   = 0.0f;   /* ← trigger H1 */

    pumpCtrl_status_t result = fcn_LoadNewPumpParameters(&profile);
    TEST_ASSERT_EQUAL(PUMPCTRL_LOAD_OK, result);
}
```

#### `test_blespresso_services.c` (CMock version)

```c
#include "unity.h"
#include "BLEspressoServices.h"
#include "mock_tempController.h"
#include "mock_solidStateRelay_Controller.h"
#include "mock_spi_Devices.h"
#include "mock_ac_inputs_drv.h"
#include "mock_bluetooth_drv.h"
#include "mock_PumpController.h"

volatile bleSpressoUserdata_struct blEspressoProfile;

void test_Classic_BrewON_ActivatesPump_And_Solenoid(void) {
    /* Strict ordering: expect these calls in this exact sequence */
    fcn_loadIboost_ParamToCtrl_Temp_ExpectAndReturn(
        (bleSpressoUserdata_struct*)&blEspressoProfile, TEMPCTRL_LOAD_OK);
    fcn_SolenoidSSR_On_Expect();
    fcn_pumpSSR_pwrUpdate_Expect(PUMP_PWR_ON);

    fcn_svc_EspressoApp(AC_SWITCH_ASSERTED, AC_SWITCH_DEASSERTED);
}

void test_H5_ClassicBrew_AutoStops_After120s(void) {
    /* Start brew */
    fcn_loadIboost_ParamToCtrl_Temp_IgnoreAndReturn(TEMPCTRL_LOAD_OK);
    fcn_SolenoidSSR_On_Ignore();
    fcn_pumpSSR_pwrUpdate_Ignore();
    fcn_updateTemperatureController_IgnoreAndReturn(500.0f);
    fcn_boilerSSR_pwrUpdate_Ignore();
    f_getBoilerTemperature_IgnoreAndReturn(80.0f);

    fcn_svc_EspressoApp(AC_SWITCH_ASSERTED, AC_SWITCH_DEASSERTED);

    /* Advance past MAX_BREW_TICKS */
    test_set_serviceTick(test_get_serviceTick() + 1201);

    /* Expect auto-shutdown sequence */
    fcn_pumpSSR_pwrUpdate_Expect(0);
    fcn_multiplyI_ParamToCtrl_Temp_ExpectAndReturn(
        (bleSpressoUserdata_struct*)&blEspressoProfile, 2.0f, TEMPCTRL_LOAD_OK);
    fcn_SolenoidSSR_Off_Expect();

    fcn_svc_EspressoApp(AC_SWITCH_ASSERTED, AC_SWITCH_DEASSERTED);
}
```

#### `test_storage_controller.c` (CMock version)

```c
#include "unity.h"
#include "StorageController.h"
#include "ProfileValidator.h"
#include "mock_spi_Devices.h"

/* CMock callback replaces FFF custom_fake */
static uint8_t g_nvm_page[65];

void callback_NVMRead(uint32_t pg, uint8_t off, uint32_t n, uint8_t *buf, int num_calls) {
    (void)pg; (void)num_calls;
    if (off + n > 65) n = 65 - off;
    memcpy(buf, g_nvm_page + off, n);
}

void test_M3_OutOfRange_Temp_Clamped_To_Default(void) {
    /* Encode temp_Target = 999.0f into NVM page at correct offset */
    encode_valid_key(g_nvm_page);
    encode_float_at(g_nvm_page, BE_USERDATA_TARGETBOILER_TMP, 999.0f);

    spi_NVMemoryRead_StubWithCallback(callback_NVMRead);

    bleSpressoUserdata_struct rx = {0};
    storageCtrl_status_t status = stgCtrl_ReadUserData(&rx);

    TEST_ASSERT_EQUAL(STORAGE_USERDATA_LOADED, status);
    /* M3 fix: 999.0f out of [20, 110] range → clamped to 93.0f default */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 93.0f, rx.temp_Target);
}
```

### Step 5 — Auto-generated mocks

CMock reads headers from `:paths:include` and generates `mock_<filename>.h/.c` for every `#include "mock_<filename>.h"` found in test files.

| Mock Target Header | Generated Mock | Used By Tests |
|--------------------|----------------|---------------|
| `solidStateRelay_Controller.h` | `mock_solidStateRelay_Controller.h/.c` | tempController, PumpController, BLEspressoServices |
| `spi_Devices.h` | `mock_spi_Devices.h/.c` | tempController, StorageController |
| `ac_inputs_drv.h` | `mock_ac_inputs_drv.h/.c` | BLEspressoServices |
| `bluetooth_drv.h` | `mock_bluetooth_drv.h/.c` | BLEspressoServices |
| `tempController.h` | `mock_tempController.h/.c` | BLEspressoServices |
| `PumpController.h` | `mock_PumpController.h/.c` | BLEspressoServices |
| `StorageController.h` | `mock_StorageController.h/.c` | (future BLEspressoServices integration tests) |

**Important:** The stub `bluetooth_drv.h` in `tests/stubs/` must provide only the function declarations needed by tests (e.g. `ble_update_boilerWaterTemp`), not the full BLE SDK include chain. CMock will parse this stub header to generate the mock.

### Step 6 — Coverage targets

```powershell
ceedling gcov:all
```

| Module | Line Coverage Target | Branch Coverage Target |
|--------|---------------------|----------------------|
| `x04_Numbers.c` | 100% | 100% |
| `x201_DigitalFiltersAlgorithm.c` | 100% | 100% |
| `x205_PID_Block.c` | ≥90% | ≥80% |
| `tempController.c` | ≥80% | ≥70% |
| `PumpController.c` | ≥80% | ≥70% |
| `BLEspressoServices.c` | ≥70% | ≥60% |
| `StorageController.c` | ≥85% | ≥75% |
| `ProfileValidator.c` | 100% | 100% |

Coverage below target triggers a review — either add tests or document why the uncovered path is unreachable on the host.

### Step 7 — Build and run

```powershell
# Run all Ceedling tests
ceedling test:all

# Run a single test file
ceedling test:test_pump_controller

# Generate coverage report
ceedling gcov:all
# → HTML report in tests_ceedling/build/artifacts/gcov/

# Clean
ceedling clobber
```

### Step 8 — Continuous integration

Add to the existing host-test CI step:

```yaml
# .github/workflows/test.yml
jobs:
  host-tests:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install Ruby + Ceedling
        run: |
          choco install ruby -y
          gem install ceedling
      - name: Phase 1+2 (Makefile)
        run: make -f tests/Makefile all
      - name: Phase 3 (Ceedling)
        run: ceedling test:all
      - name: Coverage
        run: ceedling gcov:all
      - name: Coverage gate
        run: |
          $report = Get-Content tests_ceedling/build/artifacts/gcov/GcovCoverageResults.txt
          # Parse and assert minimum coverage
```

### Phase 3 — Repository Layout

```
GaggiaController\
├── tests\                          ← Phase 1 + Phase 2 (Unity + FFF, Makefile)
│   ├── Makefile
│   ├── unity\
│   ├── fff\
│   ├── stubs\                      ← shared with Phase 3
│   ├── test_numbers.c
│   ├── test_digital_filters.c
│   ├── test_pid_block.c
│   ├── test_temp_controller.c
│   ├── test_pump_controller.c
│   ├── test_blespresso_services.c
│   ├── test_storage_controller.c
│   └── test_profile_validator.c    ← Phase 2/3
│
├── tests_ceedling\                 ← Phase 3 (Ceedling + CMock)
│   ├── project.yml
│   ├── test\
│   │   ├── test_temp_controller.c
│   │   ├── test_pump_controller.c
│   │   ├── test_blespresso_services.c
│   │   ├── test_storage_controller.c
│   │   └── test_profile_validator.c
│   └── build\                      ← auto-generated by Ceedling
│       └── artifacts\gcov\         ← HTML coverage reports
│
├── ble_espresso_app\               ← production source
└── TDD_TESTPLAN.md
```

Both `tests/` (FFF) and `tests_ceedling/` (CMock) coexist. Phase 3 does not delete Phase 2 — the Makefile-based tests remain as a fast smoke-test suite. Ceedling tests are the authoritative suite with strict verification and coverage.

### Phase 3 — Verification

1. **All Ceedling tests pass:** `ceedling test:all` → 0 failures
2. **Coverage meets targets:** `ceedling gcov:all` → all modules at or above target thresholds
3. **Strict ordering enforced:** Any out-of-order driver call causes immediate test failure (verified by intentionally reordering calls in one test and confirming failure)
4. **No unexpected calls:** Removing an `_Expect()` or `_Ignore()` for a function that IS called causes test failure
5. **Phase 2 regression:** `make -f tests/Makefile all` still passes — both suites coexist
6. **No firmware breakage:** SEGGER ES build compiles and links within 92KB app window

### Phase 3 — Decisions

- **Coexistence:** Phase 2 (Makefile + FFF) and Phase 3 (Ceedling + CMock) coexist. Phase 2 is the fast sanity check; Phase 3 is the authoritative CI gate.
- **Same stubs:** Both phases share `tests/stubs/`. No duplication.
- **Coverage gating:** Coverage reports are generated but not yet enforced as a CI gate in Phase 3. Enforcement deferred to when the team establishes baseline metrics.
- **Test naming:** Phase 3 test functions use the same names as Phase 2 where possible, making it easy to trace equivalence.
- **No on-target scope:** Phase 3 remains host-only. On-target testing is Phase 4+ (see [TEST_PLAN.md](TEST_PLAN.md)).
