# Flow: BLE → blEspressoProfile

## Overview

This flow documents how data written by a BLE client (mobile app) to GATT characteristics is parsed and stored into the global `blEspressoProfile` struct. This covers all writable characteristics across both the BLEspresso Service (`0x1400`) and PID Service (`0x1500`).

## Trigger

A connected BLE client writes a value to a writable GATT characteristic.

## Data Path

1. **SoftDevice** — receives BLE GATTS write event
2. **ble_cus.c → `on_write()`** — identifies which characteristic handle was written; packages data + event type into `ble_cus_evt_t`
3. **bluetooth_drv.c → `cus_evt_handler()`** — switch on event type:
   - Parses ASCII char array → float via `fcn_ChrArrayToFloat()`
   - Writes directly to the appropriate `blEspressoProfile.*` field
4. **Flag Set** — after the last brew param (`Prof_DeclineTmr`), `flg_BrewCfg = 1`; after last PID param (`Pid_Gain_term`), `flg_PidCfg = 1`

## Characteristic → Field Mapping

| BLE Characteristic (UUID) | Event | Target Field |
|---|---|---|
| Boiler Target Temp (`0x1403`) | `BLE_BOILER_CHAR_EVT_NEW_TEMPERATURE` | `blEspressoProfile.temp_Target` |
| Steam Target Temp (`0x140A`) | `BLE_BOILER_STEAM_TEMP_CHAR_RX_EVT` | `blEspressoProfile.sp_StemTemp` |
| Pre-Infusion Power (`0x1404`) | `BLE_BREW_PRE_INFUSION_POWER_CHAR_RX_EVT` | `blEspressoProfile.prof_preInfusePwr` |
| Pre-Infusion Time (`0x1405`) | `BLE_BREW_PRE_INFUSION_TIME__CHAR_RX_EVT` | `blEspressoProfile.prof_preInfuseTmr` |
| Infusion Power (`0x1406`) | `BLE_BREW_INFUSION_POWER_CHAR_RX_EVT` | `blEspressoProfile.prof_InfusePwr` |
| Infusion Time (`0x1407`) | `BLE_BREW_INFUSION_TIME__CHAR_RX_EVT` | `blEspressoProfile.prof_InfuseTmr` |
| Decline Power (`0x1408`) | `BLE_BREW_DECLINING_PR_POWER_CHAR_RX_EVT` | `blEspressoProfile.Prof_DeclinePwr` |
| Decline Time (`0x1409`) | `BLE_BREW_DECLINING_PR_TIME__CHAR_RX_EVT` | `blEspressoProfile.Prof_DeclineTmr` |
| P Term (`0x1501`) | `PID_P_TERM_CHAR_RX_EVT` | `blEspressoProfile.Pid_P_term` |
| I Term (`0x1502`) | `PID_I_TERM_CHAR_RX_EVT` | `blEspressoProfile.Pid_I_term` |
| I Max (`0x1503`) | `PID_I_TERM_INT_CHAR_RX_EVT` | `blEspressoProfile.Pid_Imax_term` |
| I Windup (`0x1504`) | `PID_I_TERM_WINDUP_CHAR_RX_EVT` | `blEspressoProfile.Pid_Iwindup_term` |
| D Term (`0x1505`) | `PID_D_TERM_CHAR_RX_EVT` | `blEspressoProfile.Pid_D_term` |
| D LPF (`0x1506`) | `PID_D_TERM_LPF_CHAR_RX_EVT` | `blEspressoProfile.Pid_Dlpf_term` |
| Gain (`0x1507`) | `PID_GAIN___CHAR_RX_EVT` | `blEspressoProfile.Pid_Gain_term` |

## Diagram

See [ble-to-profile_diagram.mermaid](ble-to-profile_diagram.mermaid).
