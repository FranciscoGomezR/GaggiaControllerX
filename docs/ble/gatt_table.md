# GATT Attribute Table — BLEspresso Controller

> **Source files:** `ble_espresso_app/components/BLE_Services/ble_cus.c/.h`
> **UUID registration + service creation:** `ble_cus_init()` in `ble_cus.c`
> **Data hub:** `volatile espresso_user_config_t g_Espresso_user_config_s` (see [modules.md](../architecture/modules/modules.md))

---

## Overview

The BLEspresso device exposes two custom GATT Primary Services. Both services share **one** 128-bit vendor-specific UUID base, registered once with the SoftDevice S132 in `ble_cus_init()` via a single `sd_ble_uuid_vs_add(CUSTOM_SERVICE_UUID_BASE, ...)` call. Both `sd_ble_gatts_service_add()` calls use the same `p_cus->uuid_type`.

| Property | Value |
|---|---|
| Device Name | `"BLEspresso"` (`DEVICE_NAME`, `bluetooth_drv.h`) |
| Manufacturer | `"PaxsElectronics"` (`MANUFACTURER_NAME`, `bluetooth_drv.h`) |
| UUID Base (both services) | `f364adc9-b000-xxxx-ba50-05ca45bf8abc` (`CUSTOM_SERVICE_UUID_BASE`) |
| ATT Handle Assignment | Dynamic — assigned by SoftDevice S132 during `ble_cus_init()` |
| Security | All attributes: `SEC_OPEN` (no pairing required) |
| Char add helper | `characteristic_add()` (nRF5 SDK `ble_srv_common`) |

> **Dead code:** `CUSTOM_PID_SERVICE_UUID_BASE` is defined in `ble_cus.h` but never used. The PID service is built on `CUSTOM_SERVICE_UUID_BASE`, same as the Brew service.

> **Handle notation:** Handles are dynamic. This table uses relative offsets from each service's Primary Service Declaration handle (`Sn+0`). Read actual values via GATT discovery.

---

## Service 1 — BLEspresso Machine / Brew Service

**Service UUID:** `0x1400` (`BLE_SERVICE_ESPRESSO_MACHINE_UUID`)
**Full UUID:** `f364adc9-b000-**1400**-ba50-05ca45bf8abc`
**Built in:** `ble_cus_init()` → `ble_cus_espresso_char_add()`
**Number of characteristics:** 11

### Service Attribute Table

| Handle Offset | Attribute Type | UUID | Properties | Len (bytes) | Data Format | `g_Espresso_user_config_s` Field | Notes |
|---|---|---|---|---|---|---|---|
| S1+0 | Primary Service Declaration (`0x2800`) | `0x1400` | — | 2 | UUID | — | Service boundary |
| **S1+1** | Characteristic Declaration (`0x2803`) | — | Read, Notify | 5 | Props + Handle + UUID | — | Declares `0x1401` |
| **S1+2** | **Characteristic Value** | `0x1401` | Read, Notify | 10 | ASCII string (space-padded) | — (status string) | Machine status string |
| **S1+3** | Client Characteristic Configuration (`0x2902`) | — | Read, Write | 2 | `0x0000`=off `0x0001`=notify | — | Enables/disables notifications for `0x1401` |
| **S1+4** | Characteristic Declaration (`0x2803`) | — | Read, Notify | 5 | Props + Handle + UUID | — | Declares `0x1402` |
| **S1+5** | **Characteristic Value** | `0x1402` | Read, Notify | 4 | `XXXD` ASCII (D=tenth °C) | `boilerTempDegC` | Boiler water temperature |
| **S1+6** | Client Characteristic Configuration (`0x2902`) | — | Read, Write | 2 | `0x0000`=off `0x0001`=notify | — | Enables/disables temp notifications (1 s period) |
| **S1+7** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1403` |
| **S1+8** | **Characteristic Value** | `0x1403` | Read, Write | 4 | `XXXD` ASCII (D=tenth °C) | `boilerTempSetpointDegC` | Active boiler setpoint |
| **S1+9** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1404` |
| **S1+10** | **Characteristic Value** | `0x1404` | Read, Write | 4 | `XXXD` ASCII (D=tenth °C) | `brewTempDegC` | Brew preset setpoint |
| **S1+11** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1405` |
| **S1+12** | **Characteristic Value** | `0x1405` | Read, Write | 4 | `XXXD` ASCII (D=tenth °C) | `steamTempDegC` | Steam preset setpoint |
| **S1+13** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1406` |
| **S1+14** | **Characteristic Value** | `0x1406` | Read, Write | 3 | `XXD` ASCII (D=tenth %) | `profPreInfusePwr` | Pre-infusion pump power |
| **S1+15** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1407` |
| **S1+16** | **Characteristic Value** | `0x1407` | Read, Write | 3 | `XXD` ASCII (D=tenth s) | `profPreInfuseTmr` | Pre-infusion duration |
| **S1+17** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1408` |
| **S1+18** | **Characteristic Value** | `0x1408` | Read, Write | 4 | `XXXD` ASCII (D=tenth %) | `profInfusePwr` | Infusion pump power |
| **S1+19** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1409` |
| **S1+20** | **Characteristic Value** | `0x1409` | Read, Write | 3 | `XXD` ASCII (D=tenth s) | `profInfuseTmr` | Infusion duration |
| **S1+21** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x140A` |
| **S1+22** | **Characteristic Value** | `0x140A` | Read, Write | 4 | `XXXD` ASCII (D=tenth %) | `profTaperingPwr` | Declining pressure power |
| **S1+23** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x140B` |
| **S1+24** | **Characteristic Value** | `0x140B` | Read, Write | 3 | `XXD` ASCII (D=tenth s) | `profTaperingTmr` | Declining pressure duration |

### Characteristic Summary — Brew Service

| UUID | `ble_cus.h` macro | Name | Char Decl | Char Value | CCCD | Properties | Val Len | Default (NVM validate) |
|---|---|---|---|---|---|---|---|---|
| `0x1401` | `BLE_CHAR_MACHINE_STATUS__UUID` | Machine Status | S1+1 | S1+2 | S1+3 | R, Ntf | 10 B | `"          "` (spaces) |
| `0x1402` | `BLE_CHAR_BOILER_WATER_TEMP_UUID` | Boiler Water Temp | S1+4 | S1+5 | S1+6 | R, Ntf | 4 B | `"0000"` (0.0 °C) |
| `0x1403` | `BLE_CHAR_BOILER_SET_POINT_TEMP_UUID` | Boiler Setpoint | S1+7 | S1+8 | — | R, W | 4 B | 95.5 °C |
| `0x1404` | `BLE_CHAR_BREW_TEMP_UUID` | Brew Preset Temp | S1+9 | S1+10 | — | R, W | 4 B | 95.0 °C |
| `0x1405` | `BLE_CHAR_STEAM_TEMP_UUID` | Steam Preset Temp | S1+11 | S1+12 | — | R, W | 4 B | 110.0 °C |
| `0x1406` | `BLE_CHAR_BREW_PRE_INFUSION_POWER_UUID` | Pre-Infusion Power | S1+13 | S1+14 | — | R, W | 3 B | 75.0 % |
| `0x1407` | `BLE_CHAR_BREW_PRE_INFUSION_TIME__UUID` | Pre-Infusion Time | S1+15 | S1+16 | — | R, W | 3 B | 8.0 s |
| `0x1408` | `BLE_CHAR_BREW_INFUSION_POWER_UUID` | Infusion Power | S1+17 | S1+18 | — | R, W | 4 B | 100.0 % |
| `0x1409` | `BLE_CHAR_BREW_INFUSION_TIME__UUID` | Infusion Time | S1+19 | S1+20 | — | R, W | 3 B | 8.0 s |
| `0x140A` | `BLE_CHAR_BREW_DECLINING_PR_POWER_UUID` | Declining Power | S1+21 | S1+22 | — | R, W | 4 B | 85.0 % |
| `0x140B` | `BLE_CHAR_BREW_DECLINING_PR_TIME__UUID` | Declining Time | S1+23 | S1+24 | — | R, W | 3 B | 8.0 s |

> **R** = Read · **W** = Write (Write Without Response not used) · **Ntf** = Notify
> **CUDD** (Characteristic User Description, `0x2901`) — **not present** in this project (`char_user_desc` not set in `add_char_param`).

> **Known code issue:** in `on_write()` the branch for `0x1405` (steam) compares against `brew_temp_char_handles.value_handle` a second time instead of `steam_temp_char_handles.value_handle`, so `BLE_MACHINE_STEAM_TEMP_CHAR_RX_EVT` is unreachable via that path.

---

## Service 2 — PID / Controller Service

**Service UUID:** `0x1500` (`BLE_SERVICE_CONTROLLER_UUID`)
**Full UUID:** `f364adc9-b000-**1500**-ba50-05ca45bf8abc` (same base as Service 1)
**Built in:** `ble_cus_init()` → `ble_cus_controller_char_add()`
**Number of characteristics:** 6

### Service Attribute Table

| Handle Offset | Attribute Type | UUID | Properties | Len (bytes) | Data Format | `g_Espresso_user_config_s` Field | Notes |
|---|---|---|---|---|---|---|---|
| S2+0 | Primary Service Declaration (`0x2800`) | `0x1500` | — | 2 | UUID | — | Service boundary |
| **S2+1** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1501` |
| **S2+2** | **Characteristic Value** | `0x1501` | Read, Write | 4 | `XXXD` ASCII (D=tenth) | `pidPTerm` | Proportional gain |
| **S2+3** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1502` |
| **S2+4** | **Characteristic Value** | `0x1502` | Read, Write | 3 | `XXD` ASCII (D=tenth) | `pidITerm` | Integral gain |
| **S2+5** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1503` |
| **S2+6** | **Characteristic Value** | `0x1503` | Read, Write | 4 | `XXXD` ASCII (D=tenth) | `pidImaxTerm` | Integral limit |
| **S2+7** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1504` |
| **S2+8** | **Characteristic Value** | `0x1504` | Read, Write | 3 | `XXD` ASCII (D=tenth) | `pidDTerm` | Derivative gain |
| **S2+9** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1505` |
| **S2+10** | **Characteristic Value** | `0x1505` | Read, Write | 3 | `XXD` ASCII (D=tenth) | `pidPboostTerm` | Phase-1 P-gain boost multiplier |
| **S2+11** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1506` |
| **S2+12** | **Characteristic Value** | `0x1506` | Read, Write | 3 | `XXD` ASCII (D=tenth) | `pidIboostTerm` | Phase-1 I-gain boost multiplier |

### Characteristic Summary — PID Service

| UUID | `ble_cus.h` macro | Name | Char Decl | Char Value | Properties | Val Len | Default (NVM validate) |
|---|---|---|---|---|---|---|---|
| `0x1501` | `BLE_CHAR_PID_P_TERM_UUID` | P Term | S2+1 | S2+2 | R, W | 4 B | 9.5 |
| `0x1502` | `BLE_CHAR_PID_I_TERM_UUID` | I Term | S2+3 | S2+4 | R, W | 3 B | 0.3 |
| `0x1503` | `BLE_CHAR_PID_I_MAX_TERM_UUID` | I Max | S2+5 | S2+6 | R, W | 4 B | 100.0 |
| `0x1504` | `BLE_CHAR_PID_D_TERM_UUID` | D Term | S2+7 | S2+8 | R, W | 3 B | 0.0 |
| `0x1505` | `BLE_CHAR_PID_P_BOOST_UUID` | P Boost | S2+9 | S2+10 | R, W | 3 B | 1.0 |
| `0x1506` | `BLE_CHAR_PID_I_BOOST_UUID` | I Boost | S2+11 | S2+12 | R, W | 3 B | 6.5 |

> **CUDD** — **not present** in either service.

---

## Data Format Reference

All characteristic values are **ASCII char arrays** (not raw binary floats). Conversions:
- `float_to_chr_array(value, buf, int_digits, dec_digits)` — float → ASCII (used at init)
- `chr_array_to_float(buf, int_digits, dec_digits)` — ASCII → float (on RX in `cus_evt_handler`)

| Format | Bytes | `float_to_chr_array` digits | Example | Range |
|---|---|---|---|---|
| `XXD` | 3 | `(2, 1)` | `"356"` → 35.6 | 0.0 – 99.9 |
| `XXXD` | 4 | `(3, 1)` | `"0985"` → 98.5 | 0.0 – 999.9 |
| status string | 10 | — | `"BREW      "` | ASCII |

> No decimal-point character is on the wire. Last digit is the first decimal place (×10 encoding), except the status string.

---

## BLE Write Flow (RX Path)

```
Mobile App writes char value
       ↓
SoftDevice S132 → BLE_GATTS_EVT_WRITE
       ↓
ble_cus.c: ble_cus_on_ble_evt() → on_write()
  - matches p_evt_write->handle to a known value_handle / cccd_handle
  - builds ble_cus_evt_t (data pointer + length), calls evt_handler
       ↓
bluetooth_drv.c: cus_evt_handler()
  - chr_array_to_float() parses ASCII → float
  - validate_float_in_range() clamps
  - writes result into g_Espresso_user_config_s.*
  - sets flag_brew_cfg = 1 (after 0x140B / profTaperingTmr write)
        or flag_pid_cfg  = 1 (after 0x1506 / pidIboostTerm write)
```

## BLE Notification Flow (TX Path — Temperature only)

```
main.c: tf_ble_update flag (1000 ms period)
       ↓
bluetooth_drv.c: ble_notify_boiler_water_temp(waterTemp)
  - float → 4-char ASCII (XXXD) via float_to_chr_array()
  - ble_cus_notify_boiler_water_temp() → sd_ble_gatts_hvx() on 0x1402 value handle
       ↓
SoftDevice S132 → ATT Notification PDU → Mobile App
```

---

## Notes

1. **Single UUID base** — `ble_cus_init()` registers `CUSTOM_SERVICE_UUID_BASE` once; both services use it. `CUSTOM_PID_SERVICE_UUID_BASE` is dead code.
2. **No Characteristic User Description (`0x2901`)** — not configured for any characteristic.
3. **Two CCCDs only** — `0x1401` (Machine Status) and `0x1402` (Boiler Temp) have Notify; all other characteristics are Read + Write.
4. **Write Without Response not used** — all writable characteristics use ATT Write Request + Response.
5. **Handle values are runtime-assigned** — discover at runtime, or read `ble_gatts_char_handles_t` members (`value_handle`, `cccd_handle`, `decl_handle`).
6. **Security** — all attributes `SEC_OPEN`; no bonding or encryption.
