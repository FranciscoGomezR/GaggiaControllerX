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
4. **Flag Set** — after the last brew param (`profTaperingTmr`), `flag_brew_cfg = 1`; after last PID param (`pidIboostTerm`), `flag_pid_cfg = 1`

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
| P Term (`0x1501`) | `PID_P_TERM_CHAR_RX_EVT` | `g_Espresso_user_config_s.pidPTerm` |
| I Term (`0x1502`) | `PID_I_TERM_CHAR_RX_EVT` | `g_Espresso_user_config_s.pidITerm` |
| I Max (`0x1503`) | `PID_I_TERM_INT_CHAR_RX_EVT` | `g_Espresso_user_config_s.pidImaxTerm` |
| D Term (`0x1504`) | `PID_D_TERM_CHAR_RX_EVT` | `g_Espresso_user_config_s.pidDTerm` |
| P Boost (`0x1505`) | `PID_P_BOOST_CHAR_RX_EVT` | `g_Espresso_user_config_s.pidPboostTerm` |
| I Boost (`0x1506`) | `PID_I_BOOST_CHAR_RX_EVT` | `g_Espresso_user_config_s.pidIboostTerm` |

## Diagram

See [ble-to-profile_diagram.mermaid](ble-to-profile_diagram.mermaid).
