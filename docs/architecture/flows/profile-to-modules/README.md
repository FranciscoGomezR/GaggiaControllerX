# Flow: blEspressoProfile → Modules

## Overview

This flow documents how data stored in the global `blEspressoProfile` struct is consumed by application-layer controllers and other modules.

## Consumers

### 1. tempController — Temperature PID

| Field Read | Function | When |
|---|---|---|
| `temp_Boiler` | `fcn_updateTemperatureController()` | Every 500 ms (via BLEspressoServices monitor tick) |
| `temp_Target` | `fcn_updateTemperatureController()` | Every 500 ms |
| `Pid_P_term`, `Pid_I_term`, `Pid_Imax_term`, `Pid_D_term`, `Pid_Iwindup_term` | `fcn_loadPID_ParamToCtrl_Temp()` | At boot (from NVM) or after BLE PID config |
| `Pid_Iboost_term` | `fcn_loadIboost_ParamToCtrl_Temp()` | When brew pump activates |
| `Pid_I_term` (×factor) | `fcn_multiplyI_ParamToCtrl_Temp()` | Post-brew recovery |
| `sp_BrewTemp`, `sp_StemTemp` | `fcn_loaddSetPoint_ParamToCtrl_Temp()` | Mode transitions |

### 2. PumpController — Pressure Profile

| Field Read | Function | When |
|---|---|---|
| `prof_preInfusePwr`, `prof_preInfuseTmr` | `fcn_LoadNewPumpParameters()` | At boot or after BLE brew config |
| `prof_InfusePwr`, `prof_InfuseTmr` | `fcn_LoadNewPumpParameters()` | At boot or after BLE brew config |
| `Prof_DeclinePwr`, `Prof_DeclineTmr` | `fcn_LoadNewPumpParameters()` | At boot or after BLE brew config |

### 3. BLEspressoServices — Mode State Machines

| Field Read | Context | When |
|---|---|---|
| `temp_Target` | Classic/Profile: read for logging | Every 500 ms monitor tick |
| `sp_BrewTemp`, `sp_StemTemp` | Classic: switch setpoint on steam toggle | On steam switch assert/deassert |
| `prof_*Pwr`, `prof_*Tmr` | Profile: load ramp parameters on brew start | Brew switch assert |
| All PID fields | Passed to `fcn_updateTemperatureController()` | Every 500 ms |

### 4. bluetooth_drv — BLE Notification TX

| Field Read | Function | When |
|---|---|---|
| `temp_Boiler` | `ble_update_boilerWaterTemp()` | Every 1000 ms |

### 5. StorageController — NVM Write

| Field Read | Function | When |
|---|---|---|
| All brew profile fields | `stgCtrl_StoreShotProfileData()` | After BLE brew config (flg_BrewCfg) |
| All PID fields | `stgCtrl_StoreControllerData()` | After BLE PID config (flg_PidCfg) |

### 6. ble_cus — Characteristic Init Values

| Field Read | Function | When |
|---|---|---|
| All user data fields | `custom_value_char_add()` | BLE service initialization (once) |

## Diagram

See [profile-to-modules_diagram.mermaid](profile-to-modules_diagram.mermaid).
