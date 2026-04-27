# Phase 2 Test Report — GaggiaController

**Date:** 2026-04-13  
**Environment:** Host PC (Windows x86-64), gcc (MinGW-W64) 15.2.0, GNU Make 4.4.1  
**Framework:** Unity v2.6.0 + FFF (meekrosoft/fff)  
**Target firmware:** nRF52832-QFAB, nRF5 SDK 17.1.0  

---

## Summary

| Metric | Value |
|--------|-------|
| Test executables | 4 |
| Total tests | 34 |
| Passed | **34** |
| Failed | **0** |
| Ignored | 0 |
| Overall result | **PASS** |

Phase 1 (32 tests — pure algorithm) continues to pass with no regressions.

---

## Scope

Phase 2 tests application-layer logic on the host PC using FFF hardware fakes in place of nRF SDK peripherals. No physical hardware is required.

| Module under test | Source file | Test file | Tests |
|---|---|---|:---:|
| Temperature Controller | `tempController.c` | `test_temp_controller.c` | 11 |
| Pump Controller | `PumpController.c` | `test_pump_controller.c` | 7 |
| BLEspresso Services | `BLEspressoServices.c` | `test_blespresso_services.c` | 6 |
| Storage Controller | `StorageController.c` | `test_storage_controller.c` | 10 |

---

## Infrastructure

### Stub Headers (`tests/stubs/`)

nRF SDK headers that cannot compile on x86 are shadowed by minimal stubs.  
The `-I tests/stubs` flag is placed first so stubs are found before any SDK path.

| Stub | Purpose |
|---|---|
| `nrf.h` | `NRF_SUCCESS` constant |
| `nrf_drv_timer.h` | Timer types, `NRF_DRV_TIMER_INSTANCE`, inline no-ops |
| `nrf_drv_gpiote.h` | GPIO types, polarity enum |
| `nrf_drv_spi.h` | SPI driver types |
| `nrf_gpio.h` | GPIO output/input inline no-ops |
| `nrf_log.h` | All `NRF_LOG_*` macros expand to nothing; `NRF_LOG_ENABLED=0` |
| `nrf_log_ctrl.h` | `NRF_LOG_FLUSH` no-op |
| `nrf_log_default_backends.h` | `NRF_LOG_DEFAULT_BACKENDS_INIT` no-op |
| `app_error.h` | `APP_ERROR_CHECK` no-op |
| `app_config.h` | Pin defines; keeps `NRF_LOG_ENABLED=0` (overrides real file) |
| `boards.h` | GPIO pin constants |
| `nordic_common.h` | `MIN`, `MAX`, `STATIC_ASSERT` macros |
| `nrf_soc.h` | Empty — no SoftDevice API used |
| `nrf_fstorage.h` | `NRF_FSTORAGE_DEF` stub only |
| `nrf_fstorage_sd.h` | Empty |
| `nrf_delay.h` | `nrf_delay_ms/us` no-ops |
| `bluetooth_drv.h` | Function declarations only; bodies provided by FFF |

### FFF Fakes (`tests/fakes.h`)

Hardware-driver call fakes shared by all Phase 2 test executables.

| Fake group | Functions faked |
|---|---|
| `solidStateRelay_Controller` | `fcn_boilerSSR_pwrUpdate`, `fcn_pumpSSR_pwrUpdate`, `fcn_SolenoidSSR_On`, `fcn_SolenoidSSR_Off`, `get_SolenoidSSR_State`, `fcn_initSSRController_BLEspresso` |
| `spi_Devices` | `f_getBoilerTemperature`, `spim_ReadRTDconverter`, `spim_operation_done`, `spim_init`, `spim_initRTDconverter`, `spim_initNVmemory`, `spi_NVMemoryRead`, `spi_NVMemoryWritePage`, `spim_DevCommMng` |
| `ac_inputs_drv` | `fcn_GetInputStatus_Brew`, `fcn_GetInputStatus_Steam`, `fcn_SenseACinputs_Sixty_ms`, `fcn_initACinput_drv` |
| `bluetooth_drv` | `ble_update_boilerWaterTemp` |

### `#ifdef TEST` Source Guards

Two accessors were added to `tempController.c` inside `#ifdef TEST` blocks. They compile to nothing in the firmware build (`-DTEST` is never passed to SEGGER ES).

```c
#ifdef TEST
void     test_set_milisTicks(uint32_t t)  { milisTicks = t; }
uint32_t test_get_milisTicks(void)        { return (uint32_t)milisTicks; }
float    test_get_integral_error(void)    { return sctrl_profile_main.HistoryError; }
#endif
```

---

## Source Code Fixes Applied

### H1 — Division by Zero in Pump Slope Calculation
**File:** `PumpController.c` — `fcn_LoadPumpParam()`  
**Root cause:** A BLE write of `Prof_DeclineTmr = 0` (or any zero timer field) maps to `tCounts = 0`. The integer slope formula `Pwr / tCounts` then divides by zero, causing a hard fault on the target.  
**Fix:** Clamp all three timing divisors to a minimum of 1 before computing slopes.

```c
if (prt_PumpParam->RampUp_tCounts   == 0) prt_PumpParam->RampUp_tCounts   = 1;
if (prt_PumpParam->RampDown_tCounts == 0) prt_PumpParam->RampDown_tCounts = 1;
if (prt_PumpParam->tCounts_3rdP     == 0) prt_PumpParam->tCounts_3rdP     = 1;
```

### H3 — No Temperature Sensor Failure Detection
**File:** `tempController.c` — `fcn_updateTemperatureController()`  
**Root cause:** An open-circuit RTD reads ~0 °C; a shorted RTD reads very high. Both conditions cause the PID to compute an extreme error and command 100 % heater power, risking thermal runaway.  
**Fix:** Reject readings outside the physically plausible range `[5 °C, 200 °C]` and return 0 % output.

```c
float boilerTemp = (float)prt_profData->temp_Boiler;
if (boilerTemp < 5.0f || boilerTemp > 200.0f)
{
    return 0.0f;  /* sensor fault — safe default: no heating */
}
```

### H4 — No Overheat Protection in Step-Function Mode
**File:** `BLEspressoServices.c` — `fcn_service_StepFunction()`, `sf_Mode_2b` case  
**Root cause:** Step-function mode applies 100 % heater power indefinitely for PID characterisation with no temperature ceiling. If the operator forgets a water-filled boiler or the session runs long, the boiler can overheat.  
**Fix:** Check `temp_Boiler > 150 °C` at the start of every `sf_Mode_2b` tick; if exceeded, shut the heater off and exit to `sf_Mode_max`.

```c
if ((float)blEspressoProfile.temp_Boiler > 150.0f)
{
    app_HeatPwr = PUMP_PWR_OFF;
    fcn_boilerSSR_pwrUpdate(app_HeatPwr);
    Stpfcn_HeatingStatus = false;
    stepfcnService_Status.sRunning = sf_Mode_max;
    break;
}
```

### M1 — No Integral Reset on Setpoint Change
**File:** `tempController.c` — `fcn_loaddSetPoint_ParamToCtrl_Temp()`  
**Root cause:** When the operator switches from brew mode (93 °C) to steam mode (130 °C), the integral accumulator built up during brew carries over. The sudden large positive error combined with the existing integral causes an overshoot before the controller stabilises.  
**Fix:** Zero `HistoryError` (the integral accumulator in `PID_IMC_Block_fStruct`) whenever the setpoint is changed.

```c
sctrl_profile_main.HistoryError = 0.0f;
```

### M3 — No NVM Data Range Validation (Documented, Not Yet Fixed)
**File:** `StorageController.c` — `stgCtrl_ReadUserData()`  
**Status:** Current behaviour captured by `test_M3_OutOfRange_Temp_Loaded_Without_Validation`. A valid NVM key with `temp_Target = 999 °C` is loaded into the profile struct without clamping. This is the **Red** state for M3; the fix (input validation and safe-default fallback) will be applied in a future session.

---

## Test Results

### `test_temp_controller` — 11 Tests, 0 Failures

| # | Test | Issue | Result |
|---|---|---|:---:|
| 1 | `test_TC_Init_Returns_OK` | — | PASS |
| 2 | `test_TC_LoadPID_Valid_Returns_OK` | — | PASS |
| 3 | `test_TC_LoadPID_P_Zero_Disables_P_Control` | — | PASS |
| 4 | `test_TC_SetPoint_Brew_Sets_BrewTemp` | — | PASS |
| 5 | `test_TC_SetPoint_Steam_Sets_SteamTemp` | — | PASS |
| 6 | `test_TC_IBoost_Load_Returns_OK` | — | PASS |
| 7 | `test_TC_MultiplyI_Returns_OK` | — | PASS |
| 8 | `test_TC_Update_Normal_Positive_Output` | — | PASS |
| 9 | `test_H3_OpenSensor_ShutoffHeater` | H3 | PASS |
| 10 | `test_H3_ShortedSensor_ShutoffHeater` | H3 | PASS |
| 11 | `test_M1_SetpointChange_ResetsIntegral` | M1 | PASS |

**Key verifications:**
- Tests 9–10 confirm that `fcn_updateTemperatureController` returns `0.0` for both the open-circuit sensor (`temp_Boiler = 0 °C`) and the shorted sensor (`temp_Boiler = 400 °C`) cases.
- Test 11 confirms that after running the PID for 40 simulated seconds at 130 °C setpoint (building a positive integral), a call to `fcn_loaddSetPoint_ParamToCtrl_Temp(SETPOINT_BREW)` resets `HistoryError` to `0.0 ± 0.001`.

---

### `test_pump_controller` — 7 Tests, 0 Failures

| # | Test | Issue | Result |
|---|---|---|:---:|
| 1 | `test_PC_Init_Returns_OK` | — | PASS |
| 2 | `test_PC_Init_StateDriver_Returns_IDLE` | — | PASS |
| 3 | `test_PC_LoadNewParams_Valid_Returns_OK` | — | PASS |
| 4 | `test_H1_ZeroDeclineTime_NoCrash_Returns_OK` | H1 | PASS |
| 5 | `test_H1_AllZeroTimes_NoCrash` | H1 | PASS |
| 6 | `test_PC_StartBrew_ThenDriver_CallsSolenoidOn` | — | PASS |
| 7 | `test_PC_CancelBrew_StopsPump` | — | PASS |

**Key verifications:**
- Tests 4–5 confirm that `fcn_LoadNewPumpParameters` with zero-value timing fields (`Prof_DeclineTmr = 0`, all timers = 0) completes without a crash and returns `PUMPCTRL_LOAD_OK`. Prior to fix, these cases caused integer division by zero.
- Test 6 verifies that one `fcn_PumpStateDriver` tick after `fcn_StartBrew` calls `fcn_SolenoidSSR_On` exactly once.
- Test 7 verifies pump power reaches 0 after `fcn_CancelBrew` followed by 50 driver ticks.

---

### `test_blespresso_services` — 6 Tests, 0 Failures

| # | Test | Issue | Result |
|---|---|---|:---:|
| 1 | `test_Classic_BrewON_ActivatesPump_And_Solenoid` | — | PASS |
| 2 | `test_Classic_BrewOFF_DeactivatesPump_And_Solenoid` | — | PASS |
| 3 | `test_Classic_SteamON_SetsTemperature` | — | PASS |
| 4 | `test_Classic_Every5thTick_UpdatesBoilerSSR` | — | PASS |
| 5 | `test_StepFcn_Mode2b_Heater_Set_To_100Percent` | — | PASS |
| 6 | `test_H4_StepFcn_Overheat_ShutoffHeater` | H4 | PASS |

**Key verifications:**
- Test 1 confirms that a brew switch assertion causes `fcn_SolenoidSSR_On` (once), `fcn_pumpSSR_pwrUpdate(1000)` (once), and `fcn_loadIboost_ParamToCtrl_Temp` (once).
- Test 2 confirms that releasing the brew switch causes `fcn_pumpSSR_pwrUpdate(0)`, `fcn_SolenoidSSR_Off`, and `fcn_multiplyI_ParamToCtrl_Temp` (I×2 phase-2 gain).
- Test 4 confirms the boiler SSR is updated on every 5th service tick (`SVC_MONITOR_TICK = 5`) with the value returned by the `fcn_updateTemperatureController` fake.
- Test 5 confirms that entering `sf_Mode_2b` calls `fcn_boilerSSR_pwrUpdate(1000)`.
- Test 6 confirms that setting `temp_Boiler = 160 °C` in `sf_Mode_2b` causes `fcn_boilerSSR_pwrUpdate(0)` to be called on the next tick (H4 fix).

> **Note on static state:** `BLEspressoServices.c` uses static state machines that persist across calls within the same process. Tests 1–4 are coupled in sequence (ON → OFF → Steam → 5th-tick). Tests 5–6 use a helper that drives the state machine through `sf_Mode_2a` (300 ticks) then into `sf_Mode_2b`.

---

### `test_storage_controller` — 10 Tests, 0 Failures

| # | Test | Issue | Result |
|---|---|---|:---:|
| 1 | `test_SC_Init_Returns_NVM_INIT_OK` | — | PASS |
| 2 | `test_SC_ChkForUserData_ValidKey_Returns_LOADED` | — | PASS |
| 3 | `test_SC_ChkForUserData_EmptyKey_Returns_EMPTY` | — | PASS |
| 4 | `test_SC_ReadUserData_ValidKey_Deserialises_Correctly` | — | PASS |
| 5 | `test_SC_ReadUserData_InvalidKey_Returns_EMPTY` | — | PASS |
| 6 | `test_M3_OutOfRange_Temp_Loaded_Without_Validation` | M3 ⚠ | PASS |
| 7 | `test_SC_StoreShotProfile_FirstWrite_SetsKey` | — | PASS |
| 8 | `test_SC_StoreController_WriteCycle_Incremented` | — | PASS |
| 9 | `test_SC_Private_ParseFloat_RoundTrip` | — | PASS |
| 10 | `test_SC_Private_EncodeFloat_KnownPattern` | — | PASS |

**Key verifications:**
- Tests use FFF custom-fake callbacks for `spi_NVMemoryRead` / `spi_NVMemoryWritePage` that read from / write to a local `uint8_t g_nvm_page[65]` buffer. No real SPI hardware is involved.
- Test 4 encodes `temp_Target = 93.0f` and `Pid_P_term = 9.52156f` into the 65-byte NVM page using IEEE 754 little-endian byte order, reads the page back through `stgCtrl_ReadUserData`, and asserts both fields match within `±0.001`.
- Test 6 (⚠ M3 documented) shows that `stgCtrl_ReadUserData` loads `temp_Target = 999 °C` from NVM without validation and returns `STORAGE_USERDATA_LOADED`. This **documents the known unsafe behaviour**; the fix is deferred to M3 remediation.
- Test 8 verifies that the controller write-cycle counter increments from 3 → 4 (packed in the low 16 bits of the 32-bit write-cycles word) while the shot counter (high 16 bits) is preserved at 5.
- Test 10 verifies that `encodeFloatToBytes(1.0f)` produces the known IEEE 754 LE byte pattern `{ 0x00, 0x00, 0x80, 0x3F }`.

---

## Issues Status

| ID | Description | Severity | Status | Fixed in |
|---|---|---|---|---|
| H1 | Division by zero in pump slope calc (`Prof_DeclineTmr = 0`) | High | ✅ Fixed | `PumpController.c` |
| H3 | No temperature sensor failure detection | High | ✅ Fixed | `tempController.c` |
| H4 | No overheat protection in step-function mode | High | ✅ Fixed | `BLEspressoServices.c` |
| M1 | No integral reset on setpoint change | Medium | ✅ Fixed | `tempController.c` |
| M3 | No NVM data range validation | Medium | ⚠ Documented | `StorageController.c` (fix pending) |
| H2 | No BLE input validation for out-of-range floats | High | 🔲 Deferred | `bluetooth_drv.c` (Phase 2/3) |
| H5 | No maximum brew time limit in profile mode | High | 🔲 Deferred | `BLEspressoServices.c` (Phase 2/3) |
| M5 | I-gain boost reapplied on rapid brew cycling | Medium | 🔲 Deferred | `BLEspressoServices.c` (Phase 2/3) |

---

## Build Configuration

```
make -f tests/Makefile phase2
```

Compiler flags (Phase 2):
```
-Wall -Wextra -Wno-unused-parameter -Wno-old-style-declaration
-Wno-missing-field-initializers -Wno-unused-variable
-std=c99 -DTEST -DHOST -lm
```

Include paths (in resolution order):
```
-I tests/unity
-I tests/stubs          ← shadows nRF SDK headers
-I tests/fff
-I ble_espresso_app/components/Utilities
-I ble_espresso_app/components/Utilities/include
-I ble_espresso_app/components/Application
-I ble_espresso_app/components/Peripherals/include
-I ble_espresso_app/components/BLE/include
```

Sources compiled per executable:

| Executable | Sources |
|---|---|
| `test_temp_controller.exe` | `test_temp_controller.c`, `tempController.c`, `x205_PID_Block.c`, `x201_DigitalFiltersAlgorithm.c`, `x04_Numbers.c`, `unity.c` |
| `test_pump_controller.exe` | `test_pump_controller.c`, `PumpController.c`, `unity.c` |
| `test_blespresso_services.exe` | `test_blespresso_services.c`, `BLEspressoServices.c`, `unity.c` |
| `test_storage_controller.exe` | `test_storage_controller.c`, `StorageController.c`, `unity.c` |

---

## Regression Status

| Phase | Tests | Result |
|---|:---:|---|
| Phase 1 — Pure algorithms | 32 | ✅ All pass (no regressions from source changes) |
| Phase 2 — Application logic | 34 | ✅ All pass |
| **Total** | **66** | **✅ 66 / 66** |
