# GATT Attribute Table — BLEspresso Controller

> **Source files:** `ble_espresso_app/components/BLE_Services/ble_cus.c/.h`
> **UUID registration:** `ble_espresso_app/components/BLE/bluetooth_drv.c`
> **Data hub:** `volatile bleSpressoUserdata_struct blEspressoProfile` (see [modules.md](../architecture/modules/modules.md))

---

## Overview

The BLEspresso device exposes two custom GATT Primary Services. Both share the same 128-bit vendor-specific UUID base registered with the SoftDevice S132 at runtime.

| Property | Value |
|---|---|
| Device Name | `"BLEspresso"` |
| Manufacturer | `"PaxsElectronics"` |
| UUID Base (Brew) | `f364adc9-b000-4042-ba50-05ca45bf8abc` |
| UUID Base (PID) | `11121314-1516-1718-1920-212200000000` |
| ATT Handle Assignment | Dynamic — assigned by SoftDevice S132 at `ble_cus_init()` |
| Security | All attributes: `SEC_OPEN` (no pairing required) |

> **Handle notation:** Handles are dynamic (assigned by S132 at runtime). This table uses relative offsets from each service's Primary Service Declaration handle (`Sn+0`). Actual values can be read via GATT discovery.

---

## Service 1 — BLEspresso Brew Service

**Service UUID:** `0x1400`  
**Full UUID:** `f364adc9-b000-**1400**-ba50-05ca45bf8abc`  
**Registered in:** `ble_cus_init()` → `custom_value_char_add()`  
**Number of characteristics:** 10

### Service Attribute Table

| Handle Offset | Attribute Type | UUID | Properties | Len (bytes) | Data Format | `blEspressoProfile` Field | Notes |
|---|---|---|---|---|---|---|---|
| S1+0 | Primary Service Declaration (`0x2800`) | `0x1400` | — | 2 | UUID | — | Service boundary |
| **S1+1** | Characteristic Declaration (`0x2803`) | — | Read, Notify | 5 | Props + Handle + UUID | — | Declares `0x1401` |
| **S1+2** | **Characteristic Value** | `0x1401` | Read, Notify | 10 | ASCII string (space-padded) | — (status string) | Machine status string |
| **S1+3** | Client Characteristic Configuration (`0x2902`) | — | Read, Write | 2 | `0x0000`=off `0x0001`=notify | — | Enables/disables notifications for `0x1401` |
| **S1+4** | Characteristic Declaration (`0x2803`) | — | Read, Notify | 5 | Props + Handle + UUID | — | Declares `0x1402` |
| **S1+5** | **Characteristic Value** | `0x1402` | Read, Notify | 4 | 4-char ASCII (`XXXD`, D=tenth °C) | `temp_Boiler` | Boiler water temperature |
| **S1+6** | Client Characteristic Configuration (`0x2902`) | — | Read, Write | 2 | `0x0000`=off `0x0001`=notify | — | Enables/disables temp notifications (1 s period) |
| **S1+7** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1403` |
| **S1+8** | **Characteristic Value** | `0x1403` | Read, Write | 4 | 4-char ASCII (`XXXD`, D=tenth °C) | `temp_Target` | Brew boiler setpoint |
| **S1+9** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x140A` |
| **S1+10** | **Characteristic Value** | `0x140A` | Read, Write | 4 | 4-char ASCII (`XXXD`, D=tenth °C) | `sp_StemTemp` | Steam boiler setpoint |
| **S1+11** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1404` |
| **S1+12** | **Characteristic Value** | `0x1404` | Read, Write | 3 | 3-char ASCII (`XXD`, D=tenth %) | `prof_preInfusePwr` | Pre-infusion pump power |
| **S1+13** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1405` |
| **S1+14** | **Characteristic Value** | `0x1405` | Read, Write | 3 | 3-char ASCII (`XXD`, D=tenth s) | `prof_preInfuseTmr` | Pre-infusion duration |
| **S1+15** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1406` |
| **S1+16** | **Characteristic Value** | `0x1406` | Read, Write | 4 | 4-char ASCII (`XXXD`, D=tenth %) | `prof_InfusePwr` | Infusion pump power |
| **S1+17** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1407` |
| **S1+18** | **Characteristic Value** | `0x1407` | Read, Write | 3 | 3-char ASCII (`XXD`, D=tenth s) | `prof_InfuseTmr` | Infusion duration |
| **S1+19** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1408` |
| **S1+20** | **Characteristic Value** | `0x1408` | Read, Write | 4 | 4-char ASCII (`XXXD`, D=tenth %) | `Prof_DeclinePwr` | Declining pressure power |
| **S1+21** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1409` |
| **S1+22** | **Characteristic Value** | `0x1409` | Read, Write | 3 | 3-char ASCII (`XXD`, D=tenth s) | `Prof_DeclineTmr` | Declining pressure duration |

### Characteristic Summary — Brew Service

| UUID | Name | Char Declaration | Char Value | CCCD | CUDD | Properties | Val Len | Default |
|---|---|---|---|---|---|---|---|---|
| `0x1401` | Machine Status | S1+1 | S1+2 | S1+3 | — | R, Ntf | 10 B | `"          "` (spaces) |
| `0x1402` | Boiler Water Temp | S1+4 | S1+5 | S1+6 | — | R, Ntf | 4 B | `"0000"` (0.0 °C) |
| `0x1403` | Boiler Target Temp | S1+7 | S1+8 | — | — | R, W | 4 B | From NVM / `temp_Target` |
| `0x140A` | Steam Target Temp | S1+9 | S1+10 | — | — | R, W | 4 B | From NVM / `sp_StemTemp` |
| `0x1404` | Pre-Infusion Power | S1+11 | S1+12 | — | — | R, W | 3 B | From NVM / `prof_preInfusePwr` |
| `0x1405` | Pre-Infusion Time | S1+13 | S1+14 | — | — | R, W | 3 B | From NVM / `prof_preInfuseTmr` |
| `0x1406` | Infusion Power | S1+15 | S1+16 | — | — | R, W | 4 B | From NVM / `prof_InfusePwr` |
| `0x1407` | Infusion Time | S1+17 | S1+18 | — | — | R, W | 3 B | From NVM / `prof_InfuseTmr` |
| `0x1408` | Declining Power | S1+19 | S1+20 | — | — | R, W | 4 B | From NVM / `Prof_DeclinePwr` |
| `0x1409` | Declining Time | S1+21 | S1+22 | — | — | R, W | 3 B | From NVM / `Prof_DeclineTmr` |

> **R** = Read · **W** = Write (Write Without Response not used) · **Ntf** = Notify · **CUDD** = Characteristic User Description Descriptor — **not present** in this project (not configured in `add_char_param`)

---

## Service 2 — PID Service

**Service UUID:** `0x1500`  
**UUID Base:** `CUSTOM_PID_SERVICE_UUID_BASE` — `{0x22,0x21,0x20,...,0x11,0x00,0x00,0x00,0x00}` (LE byte order)  
**Registered in:** `ble_cus_init()` → `custom_value_PIDchar_add()`  
**Number of characteristics:** 7

### Service Attribute Table

| Handle Offset | Attribute Type | UUID | Properties | Len (bytes) | Data Format | `blEspressoProfile` Field | Notes |
|---|---|---|---|---|---|---|---|
| S2+0 | Primary Service Declaration (`0x2800`) | `0x1500` | — | 2 | UUID | — | Service boundary |
| **S2+1** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1501` |
| **S2+2** | **Characteristic Value** | `0x1501` | Read, Write | 4 | 4-char ASCII (`XXXD`, D=tenth) | `Pid_P_term` | Proportional gain |
| **S2+3** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1502` |
| **S2+4** | **Characteristic Value** | `0x1502` | Read, Write | 3 | 3-char ASCII (`XXD`, D=tenth) | `Pid_I_term` | Integral gain |
| **S2+5** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1503` |
| **S2+6** | **Characteristic Value** | `0x1503` | Read, Write | 4 | 4-char ASCII (`XXXD`, D=tenth) | `Pid_Imax_term` | Integral limit |
| **S2+7** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1504` |
| **S2+8** | **Characteristic Value** | `0x1504` | Read, Write | 1 | 1-char ASCII (`'0'`=off, `'1'`=on) | `Pid_Iwindup_term` | Anti-windup enable flag |
| **S2+9** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1505` |
| **S2+10** | **Characteristic Value** | `0x1505` | Read, Write | 3 | 3-char ASCII (`XXD`, D=tenth) | `Pid_D_term` | Derivative gain |
| **S2+11** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1506` |
| **S2+12** | **Characteristic Value** | `0x1506` | Read, Write | 4 | 4-char ASCII (`XXXD`, D=tenth) | `Pid_Dlpf_term` | D-term LPF cutoff |
| **S2+13** | Characteristic Declaration (`0x2803`) | — | Read, Write | 5 | Props + Handle + UUID | — | Declares `0x1507` |
| **S2+14** | **Characteristic Value** | `0x1507` | Read, Write | 4 | 4-char ASCII (`XXXD`, D=tenth) | `Pid_Gain_term` | Overall gain multiplier |

### Characteristic Summary — PID Service

| UUID | Name | Char Declaration | Char Value | CCCD | CUDD | Properties | Val Len | Default |
|---|---|---|---|---|---|---|---|---|
| `0x1501` | P Term | S2+1 | S2+2 | — | — | R, W | 4 B | From NVM / `Pid_P_term` (9.52156) |
| `0x1502` | I Term | S2+3 | S2+4 | — | — | R, W | 3 B | From NVM / `Pid_I_term` (0.3) |
| `0x1503` | I Max | S2+5 | S2+6 | — | — | R, W | 4 B | From NVM / `Pid_Imax_term` (100.0) |
| `0x1504` | I Windup | S2+7 | S2+8 | — | — | R, W | 1 B | `'0'` (disabled) |
| `0x1505` | D Term | S2+9 | S2+10 | — | — | R, W | 3 B | From NVM / `Pid_D_term` (0.0) |
| `0x1506` | D LPF | S2+11 | S2+12 | — | — | R, W | 4 B | From NVM / `Pid_Dlpf_term` |
| `0x1507` | Gain | S2+13 | S2+14 | — | — | R, W | 4 B | From NVM / `Pid_Gain_term` |

> **CUDD** — **not present** in either service (not configured; `p_user_desc_md` not set in `add_char_param`)

---

## Data Format Reference

All characteristic values use **ASCII char arrays** (not raw binary floats). Conversions are performed by:
- `fcn_FloatToChrArray(value, buf, intDigits, decDigits)` — float → ASCII
- `fcn_ChrArrayToFloat(buf, len)` — ASCII → float (on RX in `cus_evt_handler`)

| Format | Bytes | Example | Range |
|---|---|---|---|
| `XXD` (2 int + 1 dec) | 3 | `"356"` → 35.6 | 0.0 – 99.9 |
| `XXXD` (3 int + 1 dec) | 4 | `"0985"` → 98.5 | 0.0 – 999.9 |
| 10-char status string | 10 | `"BREW      "` | ASCII |
| 1-char bool | 1 | `'0'` / `'1'` | boolean |

> **No decimal point character** is stored in the wire format. The last digit is always the first decimal place (×10 encoding), except for the status string.

---

## BLE Write Flow (RX Path)

```
Mobile App writes char value
       ↓
SoftDevice S132 → BLE_GATTS_EVT_WRITE
       ↓
ble_cus.c: on_write()
  - matches p_evt_write->handle to known value_handle
  - builds ble_cus_evt_t with data pointer + length
       ↓
bluetooth_drv.c: cus_evt_handler()
  - calls fcn_ChrArrayToFloat() to parse ASCII → float
  - writes result into blEspressoProfile.*
  - sets flags: flg_BrewCfg (after Prof_DeclineTmr) / flg_PidCfg (after Pid_Gain_term)
```

## BLE Notification Flow (TX Path — Temperature only)

```
main.c: tf_ble_update flag (1000 ms period)
       ↓
bluetooth_drv.c: ble_update_boilerWaterTemp(blEspressoProfile.temp_Boiler)
  - float → int×10 → fcn_FloatToChrArray() → 4-char ASCII
  - sd_ble_gatts_hvx() on char 0x1402 value handle
       ↓
SoftDevice S132 → ATT Notification PDU → Mobile App
```

---

## Notes

1. **No Characteristic User Description (CUDD / 0x2901)** — the `char_user_desc` field in `ble_add_char_params_t` is not set for any characteristic. GATT discovery will not show user description strings.
2. **Two CCCDs only** — only `0x1401` (Machine Status) and `0x1402` (Boiler Temp) have notify property and therefore have a CCCD attribute. All other characteristics are Read+Write only.
3. **Write Without Response not used** — all writable characteristics use standard Write (ATT Write Request + Response), not Write Command.
4. **Handle values are runtime-assigned** — the SoftDevice assigns handles sequentially during `ble_cus_init()`. Discover them at runtime using GATT Primary Service Discovery followed by Characteristic Discovery, or read them from `ble_gatts_char_handles_t` struct members (`value_handle`, `cccd_handle`, `decl_handle`).
5. **Security** — all attributes set to `SEC_OPEN`. No bonding or encryption is required.
