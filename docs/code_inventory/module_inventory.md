# Module Inventory Report

Generated: 2026-04-30  
Project: GaggiaController (nRF52832-QFAB, BLE Espresso Machine Controller)

---

## How to Use — Rename Proposals

Fill the **Proposed Name** column for any symbol you want to rename.  
Leave it **blank** to keep the current name.  
A machine agent will later apply all non-blank proposals to the source files.

```
| Current Name      | ... | Proposed Name     |
|                   |     | (fill in or leave blank)
```

---

## Summary

| Category | Count |
|----------|-------|
| Modules documented | 17 |
| Public variables (global, no `static`) | 20 |
| Private variables (correctly `static`) | 31 |
| Variables listed "private" but missing `static` | 15 |
| Extern declarations in headers | 7 |
| Public functions | 55 |
| Public ISRs | 7 |
| Private functions (correctly `static`) | 22 |
| Functions listed "private" but missing `static` | 14 |
| Test accessor stubs (`#ifdef TEST`) | 5 |

### Convention Anomalies

| # | Location | Symbol | Issue |
|---|----------|--------|-------|
| A1 | `BLEspressoServices.h:9` | `appModeToRun` | `static` variable defined inside a header — each TU that includes the header gets its own independent copy |
| A2 | `BLEspressoServices.c` | `intservice_GetSwitcheState` | Prototype listed under "private" but missing `static` keyword |
| A3 | `PumpController.c` | `sPumpParam` | Listed under "PUBLIC VARIABLES" comment but has no `extern` in header; no other TU should see it — should be `static` |
| A4 | `PumpController.c` | `fcn_LoadPumpParam` | Listed under "private" functions but missing `static` |
| A5 | `StorageController.c` | `fcn_ValidateAndClampProfile`, `parsingBytesToFloat`, `parsingBytesTo32bitVar`, `encodeFloatToBytes` | All four private function prototypes missing `static` |
| A6 | `tempController.c` | `fcn_loadI_ParamToCtrl_Temp`, `fcn_initMilisecondHWTimer` | Listed as private but missing `static` |
| A7 | `bluetooth_drv.h` | `m_gatt`, `m_qwr`, `m_advertising` | SDK macro-expanded global instances defined in the header — should be in the `.c` file |
| A8 | `bluetooth_drv.c` | `assert_nrf_callback` | Listed under private prototypes but missing `static` |
| A9 | `spi_Devices.c` | `hwreg_SensorConfig`, `resistanceRTD`, `sm_SPIdevices` | Listed under "PRIVATE VARIABLES" but all three lack `static` |
| A10 | `spi_Devices.c` | `spi_event_handler`, `spi_initTempDevCtrlPins`, `spi_initNvmDevCtrlPins`, `spi_NVMemoryCMDreset`, `spi_NVMemoryReadJEDECID`, `spi_NVMemoryWriteEnable`, `spi_NVMemoryWriteDisable`, `spi_NVMemoryEraseSector`, `NVMpageNoByteToW` | Private function prototypes missing `static` |
| A11 | `solidStateRelay_Controller.c` | `sSSRcontroller`, `sBoilderSSRzc`, `sPumpSSRdrv`, `sSolenoidSSRdrv` | Listed under "PRIVATE VARIABLES" but all lack `static` |
| A12 | `solidStateRelay_Controller.c` | `fcn_initSSRController`, `fcn_createSSRinstance`, `fcn_createOnOffSSRinstance`, `fcn_boilerSSR_ctrlUpdate`, `fcn_SSR_pwrUpdate`, `fcn_pumpSSR_ctrlUpdate`, `fcn_SSR_ctrlUpdate` | Private function prototypes missing `static` |
| A13 | `dc12Vouput_drv.c` | `s12Vout`, `seqCounter`, `startFlag`, `seqOn`, `seqDimDown`, `seqDimUp`, `seqOff` | Listed under "PRIVATE VARIABLES" but all lack `static` |
| A14 | `dc12Vouput_drv.c` | `sm_DC12Voutput_drv` | Internal PWM state machine driver — no `static`, not declared in header |
| A15 | `i2c_sensors.c` | `twi_handler` | TWI event callback prototype listed as private but missing `static` |
| A16 | `main.c` | `fcn_DO_NOTHING` | Dead-code stub with external linkage — not declared anywhere, never called |
| A17 | `main.c` | `PrintTask_flag`, `ReadSensors_flag`, `OneSecond_flag`, `LightSeq_flag`, `PumpCtrl_flag` | Volatile flags defined without `static`; appear unused (dead code remnants) |
| A18 | `x04_Numbers.c:118` | `fcn_Constrain_WithinIntValues` | Declared `inline void inline` — duplicate `inline` keyword, likely copy-paste error |

---

## Table of Contents

**Modules**
1. [BLEspressoServices](#1-bleespressoservices)
2. [PumpController](#2-pumpcontroller)
3. [StorageController](#3-storagecontroller)
4. [tempController](#4-tempcontroller)
5. [bluetooth_drv](#5-bluetooth_drv)
6. [ble_cus](#6-ble_cus)
7. [spi_Devices](#7-spi_devices)
8. [ac_inputs_drv](#8-ac_inputs_drv)
9. [solidStateRelay_Controller](#9-solidstaterelay_controller)
10. [dc12Vouput_drv](#10-dc12vouput_drv)
11. [i2c_sensors](#11-i2c_sensors)
12. [x04_Numbers](#12-x04_numbers)
13. [x205_PID_Block](#13-x205_pid_block)
14. [x201_DigitalFiltersAlgorithm](#14-x201_digitalfiltersalgorithm)
15. [x01_StateMachineControls (header-only)](#15-x01_statemachinecontrols-header-only)
16. [x02_FlagValues (defines-only)](#16-x02_flagvalues-defines-only)
17. [main](#17-main)

**Struct Definitions**
- [bleSpressoUserdata_struct](#bleSpressoUserdata_struct)
- [s_classic_data_t](#s_classic_data_t)
- [s_profile_data_t](#s_profile_data_t)
- [struct_HWTimer](#struct_HWTimer)
- [Struct_PumpProfileData](#Struct_PumpProfileData)
- [Struct_PumpParam](#Struct_PumpParam)
- [struct_CharData](#struct_CharData)
- [ble_cus_evt_t](#ble_cus_evt_t)
- [ble_cus_init_t](#ble_cus_init_t)
- [ble_cus_t](#ble_cus_t)
- [lpf_rc_param_t](#lpf_rc_param_t)
- [StateMachineCtrl_Struct](#StateMachineCtrl_Struct-struct)
- [input_PID_Block_fStruct](#input_PID_Block_fStruct)
- [PID_Block_fStruct](#PID_Block_fStruct)
- [PID_IMC_Block_fStruct](#PID_IMC_Block_fStruct)
- [struct_AcInputPin](#struct_AcInputPin)
- [struct_ControllerInputs](#struct_ControllerInputs)
- [struct_ssrTiming](#struct_ssrTiming)
- [struct_SSRinstance](#struct_SSRinstance)
- [struct_OnOffSSRinstance](#struct_OnOffSSRinstance)
- [struct_SSR_ZC_instance](#struct_SSR_ZC_instance)
- [struct_SSRcontroller](#struct_SSRcontroller)
- [StateMachine12Vout_Struct](#StateMachine12Vout_Struct)
- [struct_TaskFlg](#struct_TaskFlg)

---

## 1. BLEspressoServices

**Files:** `ble_espresso_app/components/Application/BLEspressoServices.c`  
**Header:** `ble_espresso_app/components/Application/BLEspressoServices.h`

### Variables

#### Public (global, no `static`)
| Variable | Type | Declared In | Description | Proposed Name |
|----------|------|-------------|-------------|---------------|
| `blEspressoProfile` | `volatile bleSpressoUserdata_struct` | `.c` | Central data hub — single shared struct carrying all user-configurable parameters (setpoints, PID gains, brew profile) across every module |  |

#### Private (`static`)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `espressoService_Status` | `StateMachineCtrl_Struct` | `.c` | Three-state machine (idle / cl_Mode_1 heating / cl_Mode_2 brewing) for Classic espresso service |  |
| `profileService_Status` | `StateMachineCtrl_Struct` | `.c` | Three-state machine for Profile (exponential ramp) espresso service |  |
| `stepfcnService_Status` | `StateMachineCtrl_Struct` | `.c` | Three-state machine for Step Function (PID tuning) service |  |
| `classicData` | `s_classic_data_t` | `.c` | Runtime values for Classic mode: current tick counter, heat/pump power computed per cycle |  |
| `profileData` | `s_profile_data_t` | `.c` | Runtime values for Profile mode including embedded exponential lookup table for temp ramp |  |
| `serviceTick` | `uint32_t` | `.c` | Software tick counter incremented each 20 ms scheduler call; drives brew-time and phase transitions |  |
| `app_timeStamp` | `uint32_t` | `.c` | Millisecond timestamp snapshot used to compute PID delta-time |  |
| `app_PumpPwr` | `float` | `.c` | Computed pump power (0–100 %) written to SSR driver each cycle |  |
| `app_HeatPwr` | `float` | `.c` | Computed boiler heater power (0–100 %) written to SSR driver each cycle |  |
| `f_BoilerTemp` | `float` | `.c` | Cached boiler temperature reading retrieved from SPI RTD sensor |  |
| `f_BoilerTragetTemp` | `float` | `.c` | Active temperature setpoint; may differ from profile value during brew phase transitions |  |
| `b_phase2_flag` | `bool` | `.c` | Latches true when brew phase 2 (phi2 recovery) has been entered; prevents re-entering phi1 boost |  |
| `stpfcn_tickCounter` | `uint32_t` | `.c` | Dedicated tick counter for Step Function mode; drives time-base for PID tuning steps |  |
| `TAG_SYS_MONITOR` | `static const char[]` | `.c` | NRF_LOG prefix string for system-monitor log lines |  |
| `TAG_SYS_MSG` | `static const char[]` | `.c` | NRF_LOG prefix string for general system messages |  |
| `TAG_PROF_MODE` | `static const char[]` | `.c` | NRF_LOG prefix string for Profile mode log lines |  |
| `TAG_CLAS_MODE` | `static const char[]` | `.c` | NRF_LOG prefix string for Classic mode log lines |  |

#### Extern Declarations
| Variable | Type | Declared In | Description | Proposed Name |
|----------|------|-------------|-------------|---------------|
| `blEspressoProfile` | `volatile bleSpressoUserdata_struct` | `.h` | Forward declaration so other modules can share the central data hub |  |

> **Anomaly A1:** `static uint32_t appModeToRun = machine_App;` is defined inside the `.h` file — each translation unit that includes the header gets its own independent, silently diverging copy.

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_service_ClassicMode` | `void (void)` | Cooperative task: runs one iteration of Classic espresso state machine — heats to setpoint then drives pump at fixed power for configured brew time |  |
| `fcn_service_ProfileMode` | `void (void)` | Cooperative task: runs one iteration of Profile mode — follows exponential temperature ramp table, switches from phi1 I-boost to phi2 recovery after brew start |  |
| `fcn_service_StepFunction` | `void (void)` | Cooperative task: runs one iteration of Step Function mode — applies a configurable step input for open-loop PID tuning observations |  |

#### Private
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `intservice_GetSwitcheState` | `uint32_t (void)` | Reads debounced AC brew/steam switch state from `ac_inputs_drv` and returns combined bitmask — **missing `static`** (anomaly A2) |  |

#### Test Accessors (`#ifdef TEST`)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `test_set_serviceTick` | `void (uint32_t)` | Injects a tick count value for unit-test control of time-based transitions |  |
| `test_get_serviceTick` | `uint32_t (void)` | Reads back `serviceTick` for assertion in tests |  |

---

## 2. PumpController

**Files:** `ble_espresso_app/components/Application/PumpController.c`  
**Header:** `ble_espresso_app/components/Application/PumpController.h`

### Variables

#### Public (global, no `static`)
| Variable | Type | Declared In | Description | Proposed Name |
|----------|------|-------------|-------------|---------------|
| `sPumpParam` | `Struct_PumpParam` | `.c` | Active pump profile and state machine; holds target flow, phase durations, and current state — **should be `static`**, has no `extern` in header (anomaly A3) |  |

#### Private (`static`)
*(None — all private state is embedded inside `sPumpParam` or local to functions.)*

#### Extern Declarations
*(None)*

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_initPumpController` | `void (void)` | Initialises `sPumpParam` fields to safe defaults and resets the pump state machine to idle |  |
| `fcn_PumpStateDriver` | `void (void)` | Cooperative task: advances the pump state machine one step — sequences through pre-infusion, brew, and ramp-down phases |  |
| `fcn_LoadNewPumpParameters` | `void (Struct_PumpProfileData*)` | Copies a new pump profile into `sPumpParam` and validates phase durations |  |
| `fcn_StartBrew` | `void (void)` | Triggers the pump state machine to leave idle and begin the pre-infusion phase |  |
| `fcn_CancelBrew` | `void (void)` | Forces pump state machine back to idle and zeroes pump power immediately |  |

#### Private
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_LoadPumpParam` | `void (Struct_PumpProfileData*)` | Internal helper that copies raw profile bytes into `sPumpParam` — **missing `static`** (anomaly A4) |  |

---

## 3. StorageController

**Files:** `ble_espresso_app/components/Application/StorageController.c`  
**Header:** `ble_espresso_app/components/Application/StorageController.h`

### Variables

#### Public (global, no `static`)
*(None)*

#### Private (`static`)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `hexTemp` | `static uint32_t` | `.c` (inside `encodeFloatToBytes`) | Scratch register used during float-to-4-byte little-endian encoding; static to avoid stack allocation on every NVM write |  |

#### Extern Declarations
*(None)*

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `stgCtrl_Init` | `storageCtrl_status_t (void)` | Initialises the FDS (Flash Data Storage) subsystem and waits for the ready event |  |
| `stgCtrl_ChkForUserData` | `storageCtrl_status_t (void)` | Probes FDS for existing user records; returns status indicating whether first-time defaults are needed |  |
| `stgCtrl_ReadUserData` | `storageCtrl_status_t (void)` | Reads shot-profile and controller NVM records, decodes them into `blEspressoProfile`, then calls `fcn_ValidateAndClampProfile` to clamp out-of-range values (M3 fix) |  |
| `stgCtrl_StoreShotProfileData` | `storageCtrl_status_t (void)` | Encodes and writes only the shot-profile fields to NVM (read-modify-write; controller section untouched) |  |
| `stgCtrl_StoreControllerData` | `storageCtrl_status_t (void)` | Encodes and writes only the PID/controller fields to NVM (read-modify-write; shot-profile section untouched) |  |
| `stgCtrl_PrintUserData` | `void (void)` | Dumps current `blEspressoProfile` values to NRF_LOG for debugging |  |

#### Private (missing `static` — anomaly A5)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_ValidateAndClampProfile` | `profileValidation_status_t (bleSpressoUserdata_struct*)` | Clamps all float fields to safe engineering ranges using `fcn_ValidateFloat_InRange`; returns a bitmask of fields corrected — added for M3 NVM-corruption fix |  |
| `parsingBytesToFloat` | `float (uint8_t*, uint8_t)` | Reassembles 4 raw NVM bytes (little-endian) into an IEEE-754 float |  |
| `parsingBytesTo32bitVar` | `uint32_t (uint8_t*, uint8_t)` | Reassembles 4 raw NVM bytes (little-endian) into a uint32_t |  |
| `encodeFloatToBytes` | `void (float, uint8_t*, uint8_t)` | Converts an IEEE-754 float to 4 little-endian bytes for NVM storage |  |

---

## 4. tempController

**Files:** `ble_espresso_app/components/Application/tempController.c`  
**Header:** `ble_espresso_app/components/Application/tempController.h`

### Variables

#### Public (global, no `static`)
*(None)*

#### Private (`static`)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `sctrl_profile_main` | `PID_IMC_Block_fStruct` | `.c` | PID instance for steady-state temperature hold (base Kp/Ki/Kd) |  |
| `sctrl_profile_phi1` | `PID_IMC_Block_fStruct` | `.c` | PID instance for brew-boost phase — I-gain multiplied ×6.5 to rapidly close the brew temperature drop |  |
| `sctrl_profile_phi2` | `PID_IMC_Block_fStruct` | `.c` | PID instance for post-brew recovery — I-gain multiplied ×2.0 for smooth return to setpoint |  |
| `sHwTmr_Miliseconds` | `struct_HWTimer` | `.c` | Hardware timer configuration struct for the 1 ms periodic interrupt that drives `milisTicks` |  |
| `milisTicks` | `volatile uint32_t` | `.c` | Free-running millisecond counter incremented by the 1 ms HW timer ISR; used as PID delta-time source |  |

#### Extern Declarations
*(None)*

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_initCntrl_Temp` | `void (void)` | Initialises all three PID instances and the millisecond hardware timer |  |
| `fcn_loadIboost_ParamToCtrl_Temp` | `void (PID_IMC_Block_fStruct*)` | Loads the ×6.5 brew-boost I-gain into the active PID instance (phi1 transition) |  |
| `fcn_multiplyI_ParamToCtrl_Temp` | `void (float)` | Multiplies the current I-gain by a scalar factor — used for the ×2.0 phi2 recovery transition |  |
| `fcn_loaddSetPoint_ParamToCtrl_Temp` | `void (float)` | Updates the temperature setpoint in all three PID instances atomically |  |
| `fcn_loadPID_ParamToCtrl_Temp` | `void (PID_IMC_Block_fStruct*)` | Replaces PID coefficients in all instances from a source struct (e.g., after BLE reconfiguration) |  |
| `fcn_updateTemperatureController` | `void (void)` | Runs one PID-IMC Type-A iteration: reads `milisTicks`, updates `sctrl_profile_main`, writes result to `app_HeatPwr` |  |
| `fcn_startTempCtrlSamplingTmr` | `void (void)` | Enables the 1 ms HW timer peripheral, starting `milisTicks` accumulation |  |
| `fcn_stopTempCtrlSamplingTmr` | `void (void)` | Disables the 1 ms HW timer to halt temperature sampling |  |
| `isr_HwTmr3_Period_EventHandler` | `void (void)` *(ISR)* | 1 ms hardware timer ISR — increments `milisTicks`; registered as a timer callback, requires public linkage |  |

#### Private (missing `static` — anomaly A6)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_loadI_ParamToCtrl_Temp` | `void (PID_IMC_Block_fStruct*, float)` | Writes a new Ki value into a single PID instance |  |
| `fcn_initMilisecondHWTimer` | `void (void)` | Configures and starts the nRF52 hardware timer peripheral for 1 ms period |  |

#### Test Accessors (`#ifdef TEST`)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `test_set_milisTicks` | `void (uint32_t)` | Injects a millisecond count for deterministic PID delta-time in unit tests |  |
| `test_get_milisTicks` | `uint32_t (void)` | Reads back `milisTicks` for assertion |  |
| `test_get_integral_error` | `float (void)` | Reads back `sctrl_profile_main.HistoryError` for I-term assertions |  |

---

## 5. bluetooth_drv

**Files:** `ble_espresso_app/components/BLE/bluetooth_drv.c`  
**Header:** `ble_espresso_app/components/BLE/include/bluetooth_drv.h`

### Variables

#### Public (global, no `static`)
| Variable | Type | Declared In | Description | Proposed Name |
|----------|------|-------------|-------------|---------------|
| `read_NvmData` | `volatile bleSpressoUserdata_struct` | `.c` | Temporary buffer populated when the BLE client requests a read of current NVM settings |  |
| `DataReceived[4]` | `volatile uint8_t` | `.c` | Raw 4-byte BLE write payload buffer; filled by `on_write` GATT handler |  |
| `iTagertTemp` | `volatile uint32_t` | `.c` | Target temperature received over BLE as a fixed-point integer (°C × 10) |  |
| `iTagertTemp2` | `volatile float` | `.c` | Target temperature as float; decoded from `iTagertTemp` for PID setpoint loading |  |
| `dataLen` | `volatile uint8_t` | `.c` | Length of the most recent BLE characteristic write |  |
| `flg_BrewCfg` | `volatile uint8_t` | `.c` | Set to 1 when a brew-profile BLE write arrives; cleared after `stgCtrl_StoreShotProfileData` |  |
| `flg_PidCfg` | `volatile uint8_t` | `.c` | Set to 1 when a PID-config BLE write arrives; cleared after `stgCtrl_StoreControllerData` |  |
| `flg_ReadCfg` | `volatile uint8_t` | `.c` | Set to 1 when the BLE client requests a config read; triggers NVM read and notification |  |

#### Private (`static`)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `m_conn_handle` | `static uint16_t` | `.c` | Active BLE connection handle; `BLE_CONN_HANDLE_INVALID` when disconnected |  |
| `m_adv_uuids[]` | `static ble_uuid_t` | `.c` | UUID table used during advertising data construction |  |

#### Extern Declarations
| Variable | Type | Declared In | Description | Proposed Name |
|----------|------|-------------|-------------|---------------|
| `DataReceived[4]` | `volatile uint8_t` | `.h` | Exposed so Application layer can read incoming BLE payload bytes |  |
| `iTagertTemp` | `volatile uint32_t` | `.h` | Exposed so Application layer can consume the decoded setpoint |  |
| `dataLen` | `volatile uint8_t` | `.h` | Exposed so Application layer can check payload length |  |
| `flg_BrewCfg` | `volatile uint8_t` | `.h` | Exposed so main loop can detect pending brew-profile NVM write |  |
| `flg_PidCfg` | `volatile uint8_t` | `.h` | Exposed so main loop can detect pending PID-config NVM write |  |
| `flg_ReadCfg` | `volatile uint8_t` | `.h` | Exposed so main loop can detect pending config-read request |  |

> **Anomaly A7:** `NRF_BLE_GATT_DEF(m_gatt)`, `NRF_BLE_QWR_DEF(m_qwr)`, `BLE_ADVERTISING_DEF(m_advertising)` are macro-expanded global definitions inside the `.h` file — each TU including the header would instantiate its own copy (currently only one TU includes it, masking the bug).

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `BLE_bluetooth_init` | `void (void)` | Top-level BLE stack initialisation: calls GAP, GATT, services, peer manager, advertising, and power management init in sequence |  |
| `advertising_start` | `void (bool)` | Starts undirected connectable advertising; `erase_bonds` flag triggers bond deletion before restart |  |
| `sleep_mode_enter` | `void (void)` | Puts the nRF52 into System OFF; called when no connection is present and no tasks are pending |  |
| `ble_disconnect` | `void (void)` | Issues a GAP disconnection request on the active connection |  |
| `ble_restart_without_whitelist` | `void (void)` | Restarts advertising without peer whitelist filtering (used after bond deletion) |  |
| `ble_update_boilerWaterTemp` | `void (float)` | Sends a BLE notification with the current boiler temperature to the connected client |  |

#### Private (`static`)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `pm_evt_handler` | `static void (pm_evt_t const*)` | Peer Manager event handler — processes bond events and triggers advertising restart after deletion |  |
| `gap_params_init` | `static void (void)` | Sets device name, appearance, and PPCP connection parameters in the BLE stack |  |
| `gatt_init` | `static void (void)` | Initialises the BLE GATT module (MTU and data length negotiation) |  |
| `nrf_qwr_error_handler` | `static void (uint32_t)` | Queued Write Module error handler — calls `APP_ERROR_HANDLER` on any QWR failure |  |
| `services_init` | `static void (void)` | Registers the custom BLE service (`ble_cus`) with the BLE stack |  |
| `db_discovery_init` | `static void (void)` | Initialises the BLE DB discovery module (used for client-role discovery) |  |
| `on_conn_params_evt` | `static void (ble_conn_params_evt_t*)` | Connection parameter negotiation event handler — disconnects if negotiation fails |  |
| `conn_params_error_handler` | `static void (uint32_t)` | Connection parameter module error handler — calls `APP_ERROR_HANDLER` |  |
| `conn_params_init` | `static void (void)` | Configures connection parameter update request timing |  |
| `on_adv_evt` | `static void (ble_adv_evt_t)` | Advertising event handler — starts `sleep_mode_enter` on idle timeout |  |
| `ble_evt_handler` | `static void (ble_evt_t const*, void*)` | Central BLE event dispatcher — handles connect, disconnect, and PHY update events; updates `m_conn_handle` |  |
| `ble_stack_init` | `static void (void)` | Enables the SoftDevice with nRF52832 default clock config and registers `ble_evt_handler` |  |
| `peer_manager_init` | `static void (void)` | Initialises the peer manager with security parameters (bonding, MITM = false, I/O = no-input-no-output) |  |
| `delete_bonds` | `static void (void)` | Erases all stored peer bonds from flash |  |
| `advertising_init` | `static void (void)` | Configures advertising data (UUID, name) and sets fast/slow advertising intervals |  |
| `power_management_init` | `static void (void)` | Initialises the nRF power management module |  |
| `cus_evt_handler` | `static void (ble_cus_t*, ble_cus_evt_t*)` | Custom service event handler — routes GATT write events to the appropriate BLE flag (`flg_BrewCfg`, `flg_PidCfg`, `flg_ReadCfg`) |  |

> **Anomaly A8:** `void assert_nrf_callback(uint16_t line_num, const uint8_t* p_file_name)` is listed under private prototypes but has no `static` keyword.

---

## 6. ble_cus

**Files:** `ble_espresso_app/components/BLE_Services/ble_cus.c`  
**Header:** `ble_espresso_app/components/BLE_Services/include/ble_cus.h`

### Variables

#### Public / Private / Extern
*(No module-level variables in either file — all state is embedded in the `ble_cus_t` instance owned by the caller.)*

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `ble_cus_init` | `uint32_t (ble_cus_t*, const ble_cus_init_t*)` | Registers the custom GATT service and its characteristics (boiler temp notify + config write) with the SoftDevice |  |
| `ble_cus_on_ble_evt` | `void (ble_evt_t const*, void*)` | BLE event dispatcher registered with the SoftDevice observer — routes connect/disconnect/write events to private handlers |  |
| `ble_cus_BoilerWaterTemperature_update` | `uint32_t (ble_cus_t*, float*)` | Sends an HVX notification carrying the current boiler temperature float to the connected client |  |

#### Private (`static`)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `on_connect` | `static void (ble_cus_t*, ble_evt_t const*)` | Stores the new connection handle into the `ble_cus_t` instance |  |
| `on_disconnect` | `static void (ble_cus_t*, ble_evt_t const*)` | Resets the connection handle to `BLE_CONN_HANDLE_INVALID` |  |
| `on_write` | `static void (ble_cus_t*, ble_evt_t const*)` | Handles GATTS write events — copies payload into `DataReceived`, updates `dataLen`, and invokes the registered event handler |  |
| `custom_value_char_add` | `static uint32_t (ble_cus_t*, const ble_cus_init_t*)` | Adds the boiler-temperature notify characteristic to the GATT table |  |
| `custom_value_PIDchar_add` | `static uint32_t (ble_cus_t*, const ble_cus_init_t*)` | Adds the PID/brew-config write characteristic to the GATT table |  |

---

## 7. spi_Devices

**Files:** `ble_espresso_app/components/Peripherals/spi_Devices.c`  
**Header:** `ble_espresso_app/components/Peripherals/include/spi_Devices.h`

### Variables

#### Public (global, no `static`)
*(None)*

#### Private (`static`)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `rtdTemperature` | `static float` | `.c` | Filtered boiler temperature in °C decoded from MAX31865 RTD register |  |
| `spi` | `static nrf_drv_spi_t const` | `.c` | nRF SPIM peripheral instance handle |  |
| `spi_xfer_done` | `static volatile bool` | `.c` | Transfer-complete flag set by `spi_event_handler`; polled by SPI read functions |  |
| `nvm_tx_buf[256]` | `static uint8_t` | `.c` | Transmit buffer for NVM (W25Q64FV) SPI operations |  |
| `nvm_rx_buf[260]` | `static uint8_t` | `.c` | Receive buffer for NVM SPI operations (extra 4 bytes for command/address overhead) |  |
| `tmp_buf_len` | `static const uint8_t` | `.c` | Fixed transfer length (3) for RTD sensor read/write transactions |  |
| `tmp_tx_buf[3]` | `static uint8_t` | `.c` | Transmit buffer for RTD (MAX31865) SPI transactions |  |
| `tmp_rx_buf[3]` | `static uint8_t` | `.c` | Receive buffer for RTD SPI transactions |  |

#### Missing `static` (anomaly A9)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `hwreg_SensorConfig` | `volatile uint8_t` | `.c` | MAX31865 configuration register shadow (bias on/off, fault clear, conversion mode) |  |
| `resistanceRTD` | `volatile float` | `.c` | Raw RTD resistance value computed from ADC counts before Callendar-Van Dusen conversion |  |
| `sm_SPIdevices` | `StateMachineCtrl_Struct` | `.c` | State machine controlling multi-step NVM read/write/erase sequences on the shared SPI bus |  |

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `spim_init` | `void (void)` | Initialises the nRF SPIM peripheral (SCK, MOSI, MISO pins, 4 MHz, mode 1) and configures CS GPIOs |  |
| `spim_initNVmemory` | `void (void)` | Sends the W25Q64FV wake/reset command sequence and reads the JEDEC ID to verify communication |  |
| `spim_initRTDconverter` | `void (void)` | Writes the MAX31865 configuration register to enable RTD bias and select 2-wire/4-wire mode |  |
| `spim_ReadRTDconverter` | `void (void)` | Reads the MAX31865 RTD MSB/LSB registers, computes resistance and temperature, stores into `rtdTemperature` |  |
| `spim_operation_done` | `bool (void)` | Returns the current value of `spi_xfer_done`; used by callers to poll for non-blocking transfer completion |  |
| `f_getBoilerTemperature` | `float (void)` | Returns the last decoded boiler temperature from `rtdTemperature` |  |
| `spim_DevCommMng` | `void (void)` | Cooperative state-machine task: advances `sm_SPIdevices` to sequence multi-step NVM operations |  |
| `spi_NVMemoryRead` | `void (uint32_t, uint8_t*, uint16_t)` | Sends a READ command to the W25Q64FV at the specified 24-bit address and populates `nvm_rx_buf` |  |
| `spi_NVMemoryWritePage` | `void (uint32_t, uint8_t*, uint16_t)` | Enables write, issues a PAGE PROGRAM command, then polls the status register for write-complete |  |

#### Private (missing `static` — anomaly A10)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `spi_event_handler` | `void (nrf_drv_spi_evt_t const*, void*)` | nRF SPIM event callback — sets `spi_xfer_done` on transfer completion |  |
| `spi_initTempDevCtrlPins` | `void (void)` | Configures MAX31865 chip-select and DRDY GPIO pins |  |
| `spi_initNvmDevCtrlPins` | `void (void)` | Configures W25Q64FV chip-select GPIO pin |  |
| `spi_NVMemoryCMDreset` | `void (void)` | Sends Enable-Reset then Reset commands to W25Q64FV |  |
| `spi_NVMemoryReadJEDECID` | `void (void)` | Issues JEDEC-ID read command and logs manufacturer/device IDs |  |
| `spi_NVMemoryWriteEnable` | `void (void)` | Sends WRITE-ENABLE command and verifies the WEL status bit |  |
| `spi_NVMemoryWriteDisable` | `void (void)` | Sends WRITE-DISABLE command |  |
| `spi_NVMemoryEraseSector` | `void (uint32_t)` | Issues a 4 KB sector-erase command at the given address |  |
| `NVMpageNoByteToW` | `uint8_t (uint32_t, uint8_t*, uint8_t)` | Converts a 24-bit NVM address into the 3-byte address field of a SPI command frame |  |

#### Private `__STATIC_INLINE`
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `spi_TempSensorCSenable` | `__STATIC_INLINE void (void)` | Drives MAX31865 CS low to begin a SPI transaction |  |
| `spi_TempSensorCSdisable` | `__STATIC_INLINE void (void)` | Drives MAX31865 CS high to end a SPI transaction |  |
| `spi_NVMemoryCSenable` | `__STATIC_INLINE void (void)` | Drives W25Q64FV CS low to begin a SPI transaction |  |
| `spi_NVMemoryCSdisable` | `__STATIC_INLINE void (void)` | Drives W25Q64FV CS high to end a SPI transaction |  |

---

## 8. ac_inputs_drv

**Files:** `ble_espresso_app/components/Peripherals/ac_inputs_drv.c`  
**Header:** `ble_espresso_app/components/Peripherals/include/ac_inputs_drv.h`

### Variables

#### Public (global, no `static`)
*(None)*

#### Private (`static`)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `sIO_ACinput` | `static struct_ControllerInputs` | `.c` | Encapsulates both AC input channels (Brew + Steam); holds debounce counter, ISR counter, and latched logic state for each |  |

#### Extern Declarations
*(None)*

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_initACinput_drv` | `void (void)` | Configures Brew and Steam GPIO pins as inputs with GPIOTE interrupt and initialises debounce counters |  |
| `fcn_GetInputStatus_Brew` | `uint8_t (void)` | Returns the debounced logic state of the AC Brew switch |  |
| `fcn_GetInputStatus_Steam` | `uint8_t (void)` | Returns the debounced logic state of the AC Steam switch |  |
| `fcn_SenseACinputs_Sixty_ms` | `void (void)` | Called every 60 ms from the scheduler; evaluates debounce counters for both channels via `fcn_acInputLogic` |  |

#### Public ISRs
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `acinBrew_eventHandler` | `void (nrf_drv_gpiote_pin_t, nrf_gpiote_polarity_t)` | GPIOTE ISR for Brew AC input — increments `isr_Counter` for odd/even edge debounce |  |
| `acinSteam_eventHandler` | `void (nrf_drv_gpiote_pin_t, nrf_gpiote_polarity_t)` | GPIOTE ISR for Steam AC input — increments `isr_Counter` for odd/even edge debounce |  |

#### Private (`static`)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_acInputLogic` | `static void (struct_AcInputPin*)` | Evaluates one AC input channel: compares ISR edge counts against `counterTop` to determine switch open/closed state |  |

---

## 9. solidStateRelay_Controller

**Files:** `ble_espresso_app/components/Peripherals/solidStateRelay_Controller.c`  
**Header:** `ble_espresso_app/components/Peripherals/include/solidStateRelay_Controller.h`

### Variables

#### Public (global, no `static`)
*(None)*

#### Missing `static` (anomaly A11 — listed as private, no `static` keyword)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `sSSRcontroller` | `volatile struct_SSRcontroller` | `.c` | Master SSR controller state: holds phase-angle setpoints, ZC event counters, and output enable flags for boiler and pump |  |
| `sBoilderSSRzc` | `volatile struct_SSR_ZC_instance` | `.c` | Zero-cross sync instance for the boiler SSR — tracks half-cycle counter and fires at the computed phase-angle tick (conditional on `ZERO_CROSS` define) |  |
| `sPumpSSRdrv` | `struct_SSRinstance` | `.c` | Pump SSR drive instance — holds current power setting and phase-angle tick for TRIAC firing |  |
| `sSolenoidSSRdrv` | `struct_OnOffSSRinstance` | `.c` | Solenoid (3-way valve) SSR instance — simple on/off control with GPIO pin reference |  |

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_initSSRController_BLEspresso` | `void (void)` | Initialises all SSR instances, configures GPIO outputs, and sets up zero-cross interrupt |  |
| `fcn_boilerSSR_pwrUpdate` | `void (float)` | Sets the boiler heater power percentage; updates `sSSRcontroller` phase-angle target |  |
| `fcn_pumpSSR_pwrUpdate` | `void (float)` | Sets the pump power percentage; updates `sPumpSSRdrv` phase-angle target |  |
| `fcn_SolenoidSSR_On` | `void (void)` | Drives the solenoid SSR GPIO high to energize the 3-way valve |  |
| `fcn_SolenoidSSR_Off` | `void (void)` | Drives the solenoid SSR GPIO low to de-energize the 3-way valve |  |
| `get_SolenoidSSR_State` | `uint8_t (void)` | Returns the current solenoid SSR output state (1 = energised) |  |

#### Public ISRs
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `isr_ZeroCross_EventHandler` | `void (void)` *(ISR)* | Zero-cross detection ISR — resets half-cycle counters in `sSSRcontroller` and `sBoilderSSRzc` at each AC zero-crossing |  |
| `isr_BoilderSSR_EventHandler` | `void (void)` *(ISR)* | Boiler SSR timer ISR — fires TRIAC gate pulse at the phase-angle tick computed from power setpoint (angle-control mode) |  |
| `isr_PumpSSR_EventHandler` | `void (void)` *(ISR)* | Pump SSR timer ISR — fires TRIAC gate pulse for pump at phase-angle tick derived from pump power setpoint |  |

#### Private (missing `static` — anomaly A12)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_initSSRController` | `void (struct_SSRcontroller*)` | Zeroes all fields of the master SSR controller instance |  |
| `fcn_createSSRinstance` | `void (struct_SSRinstance*, uint8_t)` | Initialises a single phase-control SSR instance with its GPIO pin |  |
| `fcn_createOnOffSSRinstance` | `void (struct_OnOffSSRinstance*, uint8_t)` | Initialises a simple on/off SSR instance with its GPIO pin |  |
| `fcn_boilerSSR_ctrlUpdate` | `void (void)` | Recalculates boiler phase-angle from power percentage and updates `sBoilderSSRzc` tick target |  |
| `fcn_SSR_pwrUpdate` | `void (struct_SSRinstance*, float)` | Translates a 0–100 % power value to a phase-angle tick count for a given SSR instance |  |
| `fcn_pumpSSR_ctrlUpdate` | `void (void)` | Recalculates pump phase-angle from power percentage and updates `sPumpSSRdrv` |  |
| `fcn_SSR_ctrlUpdate` | `void (struct_SSRinstance*)` | Generic SSR output update — compares half-cycle counter to phase-angle tick and fires gate pulse |  |

---

## 10. dc12Vouput_drv

**Files:** `ble_espresso_app/components/Peripherals/dc12Vouput_drv.c`  
**Header:** `ble_espresso_app/components/Peripherals/include/dc12Vouput_drv.h`

### Variables

#### Public (global, no `static`)
*(None)*

#### Private (`static`)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `m_pwm0` | `static nrf_drv_pwm_t` | `.c` | nRF PWM peripheral instance used for lamp dimming |  |
| `seq_ONvalues[25]` | `static uint16_t` | `.c` | 25-step ramp-up duty-cycle table (0 → 2487 counts at 250 kHz) for lamp turn-on animation |  |
| `seq_DimmDown[8]` | `static uint16_t` | `.c` | 8-step dim-down duty-cycle table for partial lamp dimming |  |
| `seq_DimmUp[8]` | `static uint16_t` | `.c` | 8-step dim-up duty-cycle table for partial lamp brightening |  |
| `seq_OFFvalues[26]` | `static uint16_t` | `.c` | 26-step ramp-down duty-cycle table (2487 → 100 counts) for lamp turn-off animation |  |

#### Missing `static` (anomaly A13)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `s12Vout` | `StateMachine12Vout_Struct` | `.c` | State machine instance tracking current lamp animation state and output status |  |
| `seqCounter` | `volatile uint32_t` | `.c` | Step counter tracking progress through the active PWM sequence |  |
| `startFlag` | `volatile uint32_t` | `.c` | Command register: 1 = turn on, 2 = dim down, 3 = dim up, 4 = turn off |  |
| `seqOn` | `nrf_pwm_sequence_t` | `.c` | PWM sequence descriptor wrapping `seq_ONvalues` |  |
| `seqDimDown` | `nrf_pwm_sequence_t` | `.c` | PWM sequence descriptor wrapping `seq_DimmDown` |  |
| `seqDimUp` | `nrf_pwm_sequence_t` | `.c` | PWM sequence descriptor wrapping `seq_DimmUp` |  |
| `seqOff` | `nrf_pwm_sequence_t` | `.c` | PWM sequence descriptor wrapping `seq_OFFvalues` |  |

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_initDC12Voutput_drv` | `dc12vout_status_t (void)` | Initialises the nRF PWM peripheral (250 kHz, 2500-count top, step-triggered), starts the ramp-up sequence, returns `DRV_12VO_INIT_AS_LAMP` |  |

#### Private (missing `static` — anomaly A14)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `sm_DC12Voutput_drv` | `void (StateMachine12Vout_Struct*)` | PWM animation state machine driver: reads `startFlag` to load a new sequence, then calls `nrf_drv_pwm_step()` each call until the sequence completes |  |

---

## 11. i2c_sensors

**Files:** `ble_espresso_app/components/Peripherals/i2c_sensors.c`  
**Header:** `ble_espresso_app/components/Peripherals/include/i2c_sensors.h`

> Note: File and registers reference "TMP006" but the sensor address and register map match the LM75B family. The TWI driver is for I2C sensing; this module appears to be a legacy/unused path — the active temperature sensor is the MAX31865 RTD on the SPI bus.

### Variables

#### Public (global, no `static`)
| Variable | Type | Declared In | Description | Proposed Name |
|----------|------|-------------|-------------|---------------|
| `eTMP006_Temp` | `float` | `.c` | Last decoded temperature reading from the I2C sensor, latched in `twi_handler` after a successful RX |  |

#### Private (`static`)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `m_xfer_done` | `static volatile bool` | `.c` | Transfer-complete flag set by `twi_handler` TWI event callback |  |
| `m_twi` | `static const nrf_drv_twi_t` | `.c` | nRF I2C (TWI) peripheral instance |  |
| `m_sample[2]` | `static uint8_t` | `.c` | Raw 2-byte receive buffer for the I2C temperature register |  |
| `pTemp` | `static float` | `.c` | Intermediate temperature float computed from `m_sample`; written to `eTMP006_Temp` on successful transfer |  |

#### Extern Declarations
| Variable | Type | Declared In | Description | Proposed Name |
|----------|------|-------------|-------------|---------------|
| `eTMP006_Temp` | `float` | `.h` | Exposed so other modules can read the last I2C temperature value |  |

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `LM75B_set_mode` | `void (void)` | Writes the NORMAL_MODE byte to the TMP006/LM75B config register and sets the temperature register pointer |  |
| `twi_init` | `void (void)` | Initialises the nRF TWI peripheral at 100 kHz with `twi_handler` as the event callback |  |
| `read_sensor_data` | `void (void)` | Initiates a 2-byte I2C receive from the temperature register if the previous transfer is complete |  |
| `twi_operation_done` | `bool (void)` | Returns `m_xfer_done` — allows callers to poll for non-blocking transfer completion |  |

#### Private (missing `static` — anomaly A15)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `twi_handler` | `void (nrf_drv_twi_evt_t const*, void*)` | TWI event callback — on RX completion computes temperature from raw bytes and latches into `eTMP006_Temp` |  |

---

## 12. x04_Numbers

**Files:** `ble_espresso_app/components/Utilities/x04_Numbers.c`  
**Header:** `ble_espresso_app/components/Utilities/x04_Numbers.h`

### Variables

#### Public / Private / Extern
*(No module-level variables)*

| Symbol | Location | Description | Proposed Name |
|--------|----------|-------------|---------------|
| `fNumber` | `static float` (local to `fcn_ChrArrayToFloat`) | Scratch accumulator for ASCII-to-float conversion; static to avoid stack churn on embedded target |  |

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_ValidateFloat_InRange` | `bool (float*, float, float, float)` | Returns true if `*value` is finite and within `[min, max]`; otherwise replaces it with `safeDefault` and returns false — foundation of the M3 NVM clamp fix |  |
| `fcn_Constrain_WithinFloats` | `int8_t inline (float*, float, float)` | Clamps a float to `[LowerLimit, UpperLimit]`; returns `POSITIVE_SATURATION`, `NEGATIVE_SATURATION`, or `NO_SATURATION` |  |
| `fcn_AddHysteresis_WithinFloat` | `void inline (float*, float, float)` | Applies a symmetric deadband: if value falls inside `±OffsetLimit` it is replaced by `NumberWithoutHyst` |  |
| `fcn_AddHysteresisMinusOffset` | `void inline (float*, float, float, float)` | Asymmetric deadband with separate upper/lower offsets; handles negative values by sign-flip before evaluation |  |
| `fcn_Constrain_WithinIntValues` | `void inline (long*, long, long)` | Clamps a `long` integer to `[LowerLimit, UpperLimit]` — **anomaly A18**: declared `inline void inline` (duplicate keyword) |  |
| `fcn_ChrArrayToFloat` | `float (char*, char, char)` | Converts a packed ASCII digit array of known width and decimal count to a float |  |
| `fcn_FloatToChrArray` | `void (float, uint8_t*, char, char)` | Converts a float to a packed ASCII digit array of known width and decimal count |  |

---

## 13. x205_PID_Block

**Files:** `ble_espresso_app/components/Utilities/x205_PID_Block.c`  
**Header:** `ble_espresso_app/components/Utilities/include/x205_PID_Block.h`

### Variables

#### Public / Private / Extern
*(No module-level variables — all state lives in the caller-owned `PID_Block_fStruct` / `PID_IMC_Block_fStruct` instances passed by pointer)*

| Symbol | Location | Description | Proposed Name |
|--------|----------|-------------|---------------|
| `Block` (typeA) | `static PID_InternalModeControl_fStruct` (local to `fcn_update_PIDimc_typeA`) | Per-call working variables for IMC Type-A iteration (dt, error, P/I/D terms, output) |  |
| `Block` (typeB) | `static PID_InternalModeControl_fStruct` (local to `fcn_update_PIDimc_typeB`) | Per-call working variables for IMC Type-B iteration |  |
| `Block` (classic) | `static PID_BlockVariable_fStruct` (local to `fcn_PID_Block_Iteration`) | Per-call working variables for classic PID iteration |  |

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_update_PID_Block` | `inline float (float, float, uint32_t, PID_Block_fStruct*)` | Feeds input/setpoint/time into a classic PID instance and returns the clamped output |  |
| `fcn_update_PIDimc_typeA` | `inline float (PID_IMC_Block_fStruct*)` | IMC Type-A PID iteration (textbook P+I+D on error signal) with anti-windup clamping; only variant actively used by `tempController` |  |
| `fcn_update_PIDimc_typeB` | `inline float (PID_IMC_Block_fStruct*)` | IMC Type-B PID iteration (D term computed on PV derivative, not error derivative) — available but not currently wired |  |
| `fcn_PID_Block_ResetI` | `inline void (PID_Block_fStruct*, float)` | Attenuates the integrator history by a scalar factor; used for M1/M5 integral-reset on mode transitions |  |
| `fcn_PID_Block_Dterm_LPF_Init` | `void (PID_Block_fStruct*)` | Initialises the optional first-order RC low-pass filter on the D-term if `D_TERM_LP_FILTER_CTRL` is set |  |
| `fcn_PID_Block_Init_Dterm_LPfilter` | `void (PID_Block_fStruct*, float*, uint8_t, uint8_t)` | Configures the D-term FIR filter coefficient buffer (multi-tap path, not the RC path) |  |

#### Private (`static`)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_PID_Block_Iteration` | `static inline void (PID_Block_fStruct*)` | Classic PID computation core: calculates dt, error, P/I/D terms with optional RC filter on D, clamps output, saves state |  |

---

## 14. x201_DigitalFiltersAlgorithm

**Files:** `ble_espresso_app/components/Utilities/x201_DigitalFiltersAlgorithm.c`  
**Header:** `ble_espresso_app/components/Utilities/include/x201_DigitalFiltersAlgorithm.h`

### Variables

#### Public / Private / Extern
*(No module-level variables — all state lives in the caller-owned `lpf_rc_param_t` instance)*

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `pfcn_InitRCFilterAlgorithm` | `void (lpf_rc_param_t*, float, float)` | Pre-computes the two RC filter coefficients for a fixed sampling time; zeroes output history |  |
| `pfcn_RCFilterAlgorithm` | `void (lpf_rc_param_t*, float)` | Runs one fixed-Ts first-order RC low-pass filter iteration: shifts `n-1`, computes new `n` output |  |
| `lpf_rc_calculate_const` | `void (lpf_rc_param_t*, float)` | Pre-computes and stores the RC time constant from cutoff frequency; used when sampling time varies |  |
| `lpf_rc_update` | `float (lpf_rc_param_t*, float, float)` | Runs one variable-Ts RC filter iteration: recomputes coefficients from current `dt`, returns filtered sample; used by PID D-term |  |

---

## 15. x01_StateMachineControls (header-only)

**File:** `ble_espresso_app/components/Utilities/x01_StateMachineControls.h`

### Defines
| Symbol | Value | Description | Proposed Name |
|--------|-------|-------------|---------------|
| `STATE_MACHINE_RUNNING` | `0` | State machine is actively processing |  |
| `STATE_MACHINE_IDLE` | `1` | State machine has completed its sequence and is waiting |  |

### Types
| Type | Fields | Description | Proposed Name |
|------|--------|-------------|---------------|
| `StateMachineCtrl_Struct` | `uint8_t sPrevious, sRunning, sNext` | Reusable three-field state machine control block used throughout the codebase |  |

*(No variables, no functions)*

---

## 16. x02_FlagValues (defines-only)

**File:** `ble_espresso_app/components/Utilities/x02_FlagValues.h`

### Defines
| Symbol | Value | Description | Proposed Name |
|--------|-------|-------------|---------------|
| `IDLE` | `0` | Generic idle/inactive flag value |  |
| `RUNNING` | `1` | Generic running/active flag value |  |
| `BLOCKED` | `2` | Task or resource is blocked |  |
| `DATA_AVAILABLE` | `1` | Data is ready to consume |  |
| `DATA_NOT_AVAILABLE` | `0` | No data ready |  |
| `DATA_VALID` | `1` | Payload passes validation |  |
| `DATA_NOT_VALID` | `0` | Payload failed validation |  |
| `TEST_OK` | `1` | Self-test passed |  |
| `TEST_NOT_OK` | `0` | Self-test failed |  |
| `CALIBRATION_OK` | `1` | Calibration accepted |  |
| `CALIBRATION_NOT_OK` | `0` | Calibration failed |  |
| `TRANSMISSION_DONE` | `1` | Transfer complete |  |
| `TRANSMITING` | `0` | Transfer in progress |  |
| `ACTIVE` | `1` | Feature or control path enabled |  |
| `NOT_ACTIVE` | `0` | Feature or control path disabled |  |
| `ACTIVE_FLAG` | `1` | Flag is set |  |
| `CLEAR_FLAG` | `0` | Flag is cleared |  |
| `CONTINOUS` | `0` | Continuous (non-one-shot) operation mode |  |

*(No variables, no functions)*

---

## 17. main

**File:** `ble_espresso_app/main/main.c`

### Variables

#### Public (global, no `static`)
| Variable | Type | Declared In | Description | Proposed Name |
|----------|------|-------------|-------------|---------------|
| `ssrPower` | `volatile uint16_t` | `main.c` | Boiler SSR power value (0–100 %) shared between temperature controller and SSR driver |  |
| `ssrPump` | `volatile uint16_t` | `main.c` | Pump SSR power value (0–100 %) shared between pump controller and SSR driver |  |
| `swTmr_tick_x0ms` | `volatile uint32_t` | `main.c` | 20 ms software timer tick counter; incremented by `t_x0ms_swTmrHandler` ISR callback |  |
| `sSchedulerFlags` | `volatile struct_TaskFlg` | `main.c` | Bitmask struct of boolean ready-flags set by the 20 ms timer and consumed by the main cooperative scheduler loop |  |

#### Dead-code flags (no `static` — anomaly A17)
| Variable | Type | File | Description | Proposed Name |
|----------|------|------|-------------|---------------|
| `PrintTask_flag` | `volatile bool` | `main.c` | Appears unused — likely a remnant from early development |  |
| `ReadSensors_flag` | `volatile bool` | `main.c` | Appears unused — replaced by `sSchedulerFlags.tf_GetBoilerTemp` |  |
| `OneSecond_flag` | `volatile bool` | `main.c` | Appears unused |  |
| `LightSeq_flag` | `volatile bool` | `main.c` | Appears unused |  |
| `PumpCtrl_flag` | `volatile bool` | `main.c` | Appears unused — replaced by `sSchedulerFlags.tf_svc_EspressoApp` |  |

#### Private (`static` — local to `main()`)
| Symbol | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `userDataLoadedFlag` | `static uint32_t` | One-shot guard inside `main()` — set after `stgCtrl_ReadUserData` succeeds to prevent repeated NVM reads |  |
| `config0` | `static nrf_drv_pwm_config_t const` | PWM config struct inside `fcn_initDC12Voutput_drv` (local static for zero-initialisation guarantee) |  |

---

### Functions

#### Public
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `main` | `int (void)` | Firmware entry point: initialises all peripherals and BLE stack, loads NVM config, then enters the cooperative scheduler loop dispatching tasks via `sSchedulerFlags` |  |

#### Private (`static`)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `t_x0ms_swTmrHandler` | `static void (void*)` | 20 ms app timer callback — increments `swTmr_tick_x0ms` and sets all `sSchedulerFlags` bits to trigger a scheduler cycle |  |
| `timers_init` | `static void (void)` | Creates the 20 ms repeating app timer backed by `t_x0ms_swTmrHandler` |  |
| `application_timers_start` | `static void (void)` | Starts the 20 ms app timer |  |
| `idle_state_handle` | `static void (void)` | Called at the bottom of the scheduler loop — flushes the NRF_LOG buffer then calls `nrf_pwr_mgmt_run()` to sleep until the next event |  |

#### Dead Code (anomaly A16)
| Function | Signature | Description | Proposed Name |
|----------|-----------|-------------|---------------|
| `fcn_DO_NOTHING` | `void (void)` | Empty function with external linkage — not declared in any header, never called; should be removed |  |

---

## Struct Type Definitions

> For each struct: write a new type name in **Rename type to** if you want to rename the struct itself.  
> Fill **Proposed Name** on any member row to rename that field. Leave blank to keep.

---

### `bleSpressoUserdata_struct`
**File:** `BLEspressoServices.h`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `nvmWcycles` | `uint32_t` | NVM write-cycle counter — tracks flash wear |  |
| `nvmKey` | `uint32_t` | Magic key used to validate that stored NVM data is not blank/corrupted |  |
| `temp_Target` | `float` | Current active temperature setpoint sent to the PID controller |  |
| `temp_Boiler` | `float` | Last measured boiler temperature read from RTD sensor |  |
| `sp_BrewTemp` | `float` | User-configured brew temperature setpoint (°C) |  |
| `sp_StemTemp` | `float` | User-configured steam temperature setpoint (°C) |  |
| `prof_preInfusePwr` | `float` | Pre-infusion stage pump power (0–100 %) |  |
| `prof_preInfuseTmr` | `float` | Pre-infusion stage duration (seconds) |  |
| `prof_InfusePwr` | `float` | Infusion stage pump power (0–100 %) |  |
| `prof_InfuseTmr` | `float` | Infusion stage duration (seconds) |  |
| `Prof_DeclinePwr` | `float` | Decline stage pump power (0–100 %) |  |
| `Prof_DeclineTmr` | `float` | Decline stage duration (seconds) |  |
| `Pid_P_term` | `float` | PID proportional gain Kp |  |
| `Pid_I_term` | `float` | PID integral gain Ki |  |
| `Pid_Iboost_term` | `float` | PID integral boost multiplier used during brew phase 1 |  |
| `Pid_Imax_term` | `float` | PID integrator anti-windup clamp limit |  |
| `Pid_Iwindup_term` | `bool` | Enable flag for PID integrator anti-windup |  |
| `Pid_D_term` | `float` | PID derivative gain Kd |  |
| `Pid_Dlpf_term` | `float` | D-term low-pass filter cutoff frequency (Hz) |  |
| `Pid_Gain_term` | `float` | Overall PID output gain multiplier |  |

---

### `s_classic_data_t`
**File:** `BLEspressoServices.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `heatingPwr` | `uint16_t` | Boiler heater power computed for the current cycle (0–100) |  |
| `pumpPwr` | `uint16_t` | Pump power computed for the current cycle (0–100) |  |
| `svcStartT` | `uint16_t` | Service start tick; used to measure elapsed brew time |  |
| `b_boostI_phase1` | `bool` | Flag: currently in phase-1 I-boost (brew start) |  |
| `b_boostI_phase2` | `bool` | Flag: currently in phase-2 I-recovery (post-brew) |  |
| `b_normalI` | `bool` | Flag: currently in normal steady-state I-gain mode |  |

---

### `s_profile_data_t`
**File:** `BLEspressoServices.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `tick` | `uint16_t` | Current tick within the active profile stage |  |
| `tickTabTarget` | `uint16_t` | Tick count target for the current lookup-table index |  |
| `heatingPwr` | `uint16_t` | Boiler heater power for the current cycle |  |
| `delta_pumpPwr` | `int16_t` | Signed pump power delta applied per tick during ramp |  |
| `base_pumpPwr` | `uint16_t` | Starting pump power for the current ramp segment |  |
| `pumpPwr` | `uint16_t` | Current pump power output |  |
| `svcStartT` | `uint16_t` | Service start tick; used to measure elapsed brew time |  |
| `a_expGrowth[14]` | `const float` | 14-point exponential growth table for pressure ramp-up |  |
| `a_expDecay[14]` | `const float` | 14-point exponential decay table for pressure ramp-down |  |
| `noTabs` | `const uint16_t` | Number of entries in the active lookup table |  |
| `ptrTab` | `const float*` | Pointer to the active ramp lookup table |  |
| `tabCnt` | `uint16_t` | Current index into the active lookup table |  |
| `b_activeFlg` | `bool` | Flag: profile service is actively running |  |
| `b_boostI_phase1` | `bool` | Flag: currently in phase-1 I-boost |  |
| `b_boostI_phase2` | `bool` | Flag: currently in phase-2 I-recovery |  |
| `b_normalI` | `bool` | Flag: currently in normal steady-state I-gain mode |  |

---

### `struct_HWTimer`
**File:** `tempController.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `hwTmr` | `nrf_drv_timer_t` | nRF hardware timer peripheral instance handle |  |
| `hwTmr_isr_handler` | `nrfx_timer_event_handler_t` | Callback registered for timer compare/overflow events |  |
| `tmrPeriod_us` | `uint32_t` | Desired timer period in microseconds |  |
| `tmrPeriod_ticks` | `uint32_t` | Period converted to hardware timer ticks |  |
| `status` | `bool` | True when the timer has been successfully initialised and is running |  |

---

### `Struct_PumpProfileData`
**File:** `PumpController.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `Pwr_1stP` | `uint16_t` | Pump power for pre-infusion stage (0–100) |  |
| `Pwr_2ndP` | `uint16_t` | Pump power for infusion stage (0–100) |  |
| `Pwr_3rdP` | `uint16_t` | Pump power for decline stage (0–100) |  |
| `Time_1stP_ms` | `uint16_t` | Pre-infusion stage duration (ms) |  |
| `Time_2ndP_ms` | `uint16_t` | Infusion stage duration (ms) |  |
| `Time_3rdP_ms` | `uint16_t` | Decline stage duration (ms) |  |
| `Time_Rampup_ms` | `uint16_t` | Ramp-up transition duration (ms) |  |
| `Time_Rampdown_ms` | `uint16_t` | Ramp-down transition duration (ms) |  |

---

### `Struct_PumpParam`
**File:** `PumpController.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `smPump` | `StateMachineCtrl_Struct` | State machine control block for the pump sequencer |  |
| `PumpPower` | `uint16_t` | Current pump output power (0–100) |  |
| `RampUp_tCounts` | `uint16_t` | Tick count budget for the ramp-up transition |  |
| `RampDown_tCounts` | `uint16_t` | Tick count budget for the ramp-down transition |  |
| `tCounts_1stP` | `uint16_t` | Tick count budget for pre-infusion stage |  |
| `tCounts_2ndP` | `uint16_t` | Tick count budget for infusion stage |  |
| `tCounts_3rdP` | `uint16_t` | Tick count budget for decline stage |  |
| `SlopeTo_1stP` | `int16_t` | Power slope (Δpower/tick) ramping into pre-infusion |  |
| `SlopeTo_2ndP` | `int16_t` | Power slope ramping into infusion |  |
| `SlopeTo_3rdP` | `int16_t` | Power slope ramping into decline |  |
| `SlopeTo_Stop` | `int16_t` | Power slope ramping down to zero at brew end |  |
| `Counter` | `uint16_t` | Current tick counter within the active stage |  |
| `Slope` | `int16_t` | Active slope value being applied this stage |  |

---

### `struct_CharData`
**File:** `ble_cus.h`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `ptr_data` | `uint8_t const*` | Pointer to the raw characteristic payload bytes |  |
| `length` | `uint16_t` | Byte length of the payload |  |

---

### `ble_cus_evt_t`
**File:** `ble_cus.h`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `evt_type` | `ble_cus_evt_type_t` | Event identifier indicating which characteristic triggered the callback |  |
| `param_command.sBoilerTempTarget` | `struct_CharData` | Payload for brew temperature target write |  |
| `param_command.sBoilerSteamTemp` | `struct_CharData` | Payload for steam temperature target write |  |
| `param_command.s_preInfusePwr` | `struct_CharData` | Payload for pre-infusion power write |  |
| `param_command.s_preInfuseTmr` | `struct_CharData` | Payload for pre-infusion timer write |  |
| `param_command.s_InfusePwr` | `struct_CharData` | Payload for infusion power write |  |
| `param_command.s_InfuseTmr` | `struct_CharData` | Payload for infusion timer write |  |
| `param_command.s_DeclinePwr` | `struct_CharData` | Payload for decline power write |  |
| `param_command.s_DeclineTmr` | `struct_CharData` | Payload for decline timer write |  |
| `param_command.sPid_P_term` | `struct_CharData` | Payload for PID Kp write |  |
| `param_command.sPid_I_term` | `struct_CharData` | Payload for PID Ki write |  |
| `param_command.sPid_Imax_term` | `struct_CharData` | Payload for PID integrator limit write |  |
| `param_command.sPid_Iwindup_term` | `struct_CharData` | Payload for PID anti-windup enable write |  |
| `param_command.sPid_D_term` | `struct_CharData` | Payload for PID Kd write |  |
| `param_command.sPid_Dlpf_term` | `struct_CharData` | Payload for D-term LPF cutoff write |  |
| `param_command.sPid_Gain_term` | `struct_CharData` | Payload for PID output gain write |  |

---

### `ble_cus_init_t`
**File:** `ble_cus.h`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `evt_handler` | `ble_cus_evt_handler_t` | Application callback invoked on every custom-service GATT event |  |
| `initial_custom_value` | `uint8_t` | Initial value written to the custom characteristic at registration |  |
| `blespressoStatus_char_attr_md` | `ble_srv_cccd_security_mode_t` | Security mode for the status characteristic |  |
| `boilerTemp_char_attr_md` | `ble_srv_cccd_security_mode_t` | Security mode for the boiler-temperature notify characteristic |  |
| `boilerTargetTemp_char_attr_md` | `ble_srv_cccd_security_mode_t` | Security mode for the brew target-temperature characteristic |  |
| `brewPreInfussionPower_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for pre-infusion power characteristic |  |
| `brewPreInfussiontime__char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for pre-infusion time characteristic |  |
| `brewInfussionPower_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for infusion power characteristic |  |
| `brewInfussiontime__char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for infusion time characteristic |  |
| `brewDecliningPower_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for decline power characteristic |  |
| `brewDecliningTime__char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for decline time characteristic |  |
| `pid_Pterm_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for PID Kp characteristic |  |
| `pid_Iterm_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for PID Ki characteristic |  |
| `pid_ImaxTerm_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for PID I-limit characteristic |  |
| `pid_Iwindup_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for PID anti-windup characteristic |  |
| `pid_Dterm_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for PID Kd characteristic |  |
| `pid_DlpfTerm_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for D-term LPF characteristic |  |
| `pid_GainTerm_char_attr` | `ble_srv_cccd_security_mode_t` | Security mode for PID gain characteristic |  |

---

### `ble_cus_t`
**File:** `ble_cus.h` (`struct ble_cus_s`)  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `evt_handler` | `ble_cus_evt_handler_t` | Registered application callback for GATT events |  |
| `service_handle` | `uint16_t` | SoftDevice handle for the registered GATT service |  |
| `blespressoStatus_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for the status characteristic |  |
| `boilerTemp_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for the boiler-temperature notify characteristic |  |
| `boilerTargetTemp_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for the brew target-temperature characteristic |  |
| `boilerSteamTargetTemp_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for the steam target-temperature characteristic |  |
| `brewPreInfussionPower_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for pre-infusion power characteristic |  |
| `brewPreInfussiontime__char_handles` | `ble_gatts_char_handles_t` | GATTS handles for pre-infusion time characteristic |  |
| `brewInfussionPower_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for infusion power characteristic |  |
| `brewInfussiontime__char_handles` | `ble_gatts_char_handles_t` | GATTS handles for infusion time characteristic |  |
| `brewDecliningPower_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for decline power characteristic |  |
| `brewDecliningtime__char_handles` | `ble_gatts_char_handles_t` | GATTS handles for decline time characteristic |  |
| `pid_Pterm_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for PID Kp characteristic |  |
| `pid_Iterm_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for PID Ki characteristic |  |
| `pid_ImaxTerm_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for PID I-limit characteristic |  |
| `pid_Iwindup_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for PID anti-windup characteristic |  |
| `pid_Dterm_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for PID Kd characteristic |  |
| `pid_DlpfTerm_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for D-term LPF characteristic |  |
| `pid_GainTerm_char_handles` | `ble_gatts_char_handles_t` | GATTS handles for PID gain characteristic |  |
| `conn_handle` | `uint16_t` | Active BLE connection handle; `BLE_CONN_HANDLE_INVALID` when disconnected |  |
| `uuid_type` | `uint8_t` | UUID type index assigned by SoftDevice when the vendor UUID was registered |  |

---

### `lpf_rc_param_t`
**File:** `x201_DigitalFiltersAlgorithm.h`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `DataOut_n` | `float` | Current filter output y[n] |  |
| `DataOut_n_1` | `float` | Previous filter output y[n-1] |  |
| `FilterRCCoefficients[2]` | `float` | Precomputed RC filter coefficients [b0, a1] |  |
| `rc_constant` | `float` | RC time constant (τ = 1 / (2π·fc)) |  |

---

### `StateMachineCtrl_Struct` {#StateMachineCtrl_Struct-struct}
**File:** `x01_StateMachineControls.h`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `sPrevious` | `uint8_t` | State index of the previously executed state |  |
| `sRunning` | `uint8_t` | State index currently being executed |  |
| `sNext` | `uint8_t` | State index to transition to on next iteration |  |

---

### `input_PID_Block_fStruct`
**File:** `x205_PID_Block.h`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `ProcessVariable` | `float` | Current measured value fed into the PID (e.g. boiler temperature) |  |
| `SetPoint` | `float` | Desired target value |  |
| `TimeMilis` | `uint32_t` | Current timestamp in milliseconds used to compute dt |  |

---

### `PID_Block_fStruct`
**File:** `x205_PID_Block.h`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `feedPIDblock` | `input_PID_Block_fStruct` | Current PV, setpoint, and timestamp inputs |  |
| `PrevError` | `float` | Error value from the previous iteration (used for D-term) |  |
| `prevT_Milis` | `float` | Timestamp of the previous iteration (ms) |  |
| `OutputLimit` | `float` | Symmetric clamp applied to the PID output |  |
| `OutputSaturationOut` | `int8_t` | Saturation status flag: +1, −1, or 0 |  |
| `Output` | `float` | Current PID output value after clamping |  |
| `P_TERM_CTRL` | `bool` | Enable flag for the proportional term |  |
| `Kp` | `float` | Proportional gain |  |
| `I_TERM_CTRL` | `bool` | Enable flag for the integral term |  |
| `Ki` | `float` | Integral gain |  |
| `HistoryError` | `float` | Accumulated integral (sum of error × dt) |  |
| `IntegralLimit` | `float` | Anti-windup clamp on the integrator accumulator |  |
| `I_ANTIWINDUP_CTRL` | `bool` | Enable flag for integrator anti-windup clamping |  |
| `WindupClampStatus` | `bool` | True when the integrator is currently clamped |  |
| `D_TERM_CTRL` | `bool` | Enable flag for the derivative term |  |
| `Kd` | `float` | Derivative gain |  |
| `D_TERM_LP_FILTER_CTRL` | `bool` | Enable flag for the D-term low-pass filter |  |
| `sLPF_Param` | `lpf_rc_param_t` | RC low-pass filter state for D-term noise suppression |  |
| `LPF_FCUTOFF_HZ` | `float` | D-term LPF cutoff frequency (Hz) |  |

---

### `PID_IMC_Block_fStruct`
**File:** `x205_PID_Block.h`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `feedPIDblock` | `input_PID_Block_fStruct` | Current PV, setpoint, and timestamp inputs |  |
| `prevT_Milis` | `float` | Timestamp of the previous iteration (ms) |  |
| `errorK_1` | `float` | Error at previous sample k−1 |  |
| `errorK_2` | `float` | Error at sample k−2 (used in IMC Type-B velocity form) |  |
| `OutputLimit` | `float` | Symmetric clamp applied to the PID output |  |
| `OutputSaturationOut` | `int8_t` | Saturation status flag: +1, −1, or 0 |  |
| `Output` | `float` | Current PID output value after clamping |  |
| `P_TERM_CTRL` | `bool` | Enable flag for the proportional term |  |
| `Kp` | `float` | Proportional gain |  |
| `I_TERM_CTRL` | `bool` | Enable flag for the integral term |  |
| `I_ANTIWINDUP_CTRL` | `bool` | Enable flag for integrator anti-windup clamping |  |
| `Ki` | `float` | Integral gain |  |
| `HistoryError` | `float` | Accumulated integral (sum of error × dt) |  |
| `IntegralError` | `float` | Current raw integral error before anti-windup clamp |  |
| `IntegralLimit` | `float` | Anti-windup clamp on the integrator accumulator |  |
| `WindupClampStatus` | `bool` | True when the integrator is currently clamped |  |
| `D_TERM_CTRL` | `bool` | Enable flag for the derivative term |  |
| `D_TERM_FILTER_CTRL` | `bool` | Enable flag for the D-term filter |  |
| `prevPV` | `float` | Previous process variable value (used for IMC Type-B D on PV) |  |
| `Kd` | `float` | Derivative gain |  |

---

### `struct_AcInputPin`
**File:** `ac_inputs_drv.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `pinID` | `uint8_t` | GPIO pin number for this AC input channel |  |
| `isr_Counter` | `volatile uint32_t` | Edge-count incremented by the GPIOTE ISR; used for debounce evaluation |  |
| `logicEvaluation` | `volatile bool` | Latched logic state after debounce algorithm runs |  |
| `counterTop` | `uint32_t` | Edge-count threshold for debounce decision |  |
| `Status` | `uint8_t` | Debounced switch state exported to the application layer |  |
| `ext_isr_handler` | `nrfx_gpiote_evt_handler_t` | GPIOTE event handler registered for this pin |  |

---

### `struct_ControllerInputs`
**File:** `ac_inputs_drv.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `Brew` | `struct_AcInputPin` | AC input channel instance for the Brew switch |  |
| `Steam` | `struct_AcInputPin` | AC input channel instance for the Steam switch |  |

---

### `struct_ssrTiming`
**File:** `solidStateRelay_Controller.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `tStep` | `uint16_t` | Timer step resolution in microseconds |  |
| `tPeriod` | `uint16_t` | Full AC half-cycle period in microseconds (e.g. 10 000 µs @ 50 Hz) |  |
| `tTrigger` | `uint16_t` | Phase-angle trigger time in microseconds from zero-cross |  |
| `tZCdelay` | `uint16_t` | Zero-cross detection propagation delay compensation in microseconds |  |
| `cPeriod` | `uint32_t` | Full half-cycle period converted to timer ticks |  |
| `cTrigger` | `uint32_t` | Trigger time converted to timer ticks |  |
| `cZCdelay` | `uint32_t` | Zero-cross delay converted to timer ticks |  |

---

### `struct_SSRinstance`
**File:** `solidStateRelay_Controller.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `hwTmr` | `nrf_drv_timer_t` | Hardware timer peripheral instance for phase-angle firing |  |
| `hwTmr_isr_handler` | `nrfx_timer_event_handler_t` | Timer compare ISR that fires the TRIAC gate pulse |  |
| `in_zCross` | `uint8_t` | GPIO pin number for zero-cross input sensing |  |
| `out_SSRelay` | `uint8_t` | GPIO pin number for SSR gate output |  |
| `zcross_isr_handler` | `nrfx_gpiote_evt_handler_t` | GPIOTE handler for zero-cross edge detection |  |
| `sSRR_timing_us` | `struct_ssrTiming` | Timing parameters (period, trigger, delay) in µs and ticks |  |
| `smTrigStatus` | `uint8_t` | State of the phase-angle trigger sequencer |  |
| `srrPower` | `uint16_t` | Current power setpoint (0–100) for this SSR channel |  |
| `ssrPWRstatus` | `uint8_t` | Output enable/disable status flag |  |

---

### `struct_OnOffSSRinstance`
**File:** `solidStateRelay_Controller.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `out_SSRelay` | `uint8_t` | GPIO pin number for SSR gate output |  |
| `ssrState` | `uint8_t` | Current on/off state of the relay |  |
| `ssrPWRstatus` | `uint8_t` | Output enable/disable status flag |  |

---

### `struct_SSR_ZC_instance`
**File:** `solidStateRelay_Controller.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `out_SSRelay` | `uint8_t` | GPIO pin number for SSR gate output |  |
| `cntPWRcycles` | `uint8_t` | Counter of AC half-cycles since last zero-cross event |  |
| `smTrigStatus` | `uint8_t` | State of the phase-angle trigger sequencer |  |
| `srrPowerCnt` | `uint8_t` | Cycle-count threshold derived from power setpoint |  |
| `srrPower` | `uint16_t` | Current power setpoint (0–100) |  |
| `ssrPWRstatus` | `uint8_t` | Output enable/disable status flag |  |

---

### `struct_SSRcontroller`
**File:** `solidStateRelay_Controller.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `in_zCross` | `uint8_t` | GPIO pin number for shared zero-cross input |  |
| `zcross_isr_handler` | `nrfx_gpiote_evt_handler_t` | GPIOTE handler shared by all SSR instances for zero-cross detection |  |
| `status` | `bool` | True when the SSR controller has been successfully initialised |  |

---

### `StateMachine12Vout_Struct`
**File:** `dc12Vouput_drv.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `sDrv` | `StateMachineCtrl_Struct` | State machine control block for the lamp animation driver |  |
| `outState` | `uint8_t` | Current lamp output state (on / off / dimming) |  |

---

### `struct_TaskFlg`
**File:** `main.c`  
**Rename type to:**

| Member | Type | Description | Proposed Name |
|--------|------|-------------|---------------|
| `tf_ReadButton` | `bool` | Ready flag: time to sample AC brew/steam switches (set every 60 ms) |  |
| `tf_GetBoilerTemp` | `bool` | Ready flag: time to read RTD temperature sensor (set every 100 ms) |  |
| `tf_ble_update` | `bool` | Ready flag: time to send BLE temperature notification (set every 100 ms) |  |
| `tf_svc_StepFunction` | `bool` | Ready flag: time to run Step Function PID-tuning service (set every 100 ms) |  |
| `tf_svc_EspressoApp` | `bool` | Ready flag: time to run the active espresso mode state machine (set every 100 ms) |  |
