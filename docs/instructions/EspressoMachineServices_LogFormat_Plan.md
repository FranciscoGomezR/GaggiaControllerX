# Plan: Update Serial Monitor Log Format — Classic and Profile Mode

## Context

The periodic serial monitor print (`TAG_SYS_MONITOR`) currently logs five fields with `Heating_Power`
before temperature columns and no brew-time field. The new format:
- Renames `Time_Miliseconds` → `System_Time_Miliseconds`
- Inserts `Brew_time` (from `g_Espresso_user_config_s.extractionTimeMsecs`) as the second column
- Moves `Heating_Power` to after `Boiler_Temp_DegC`

**Depends on:** `EspressoMachineServices_ExtractionTime_Plan.md` must be implemented first
so that `extractionTimeMsecs` exists in the struct.

---

## Old vs New Format

### Format header string (TAG_SYS_FORMAT)
```
Old: "%s;Time_Miliseconds;Heating_Power;Boiler_Target_DegC;Boiler_Temp_DegC;Pump_Power"
New: "%s;System_Time_Miliseconds;Brew_time;Boiler_Target_DegC;Boiler_Temp_DegC;Heating_Power;Pump_Power"
```

### Data row sprintf (TAG_SYS_MONITOR)
```
Old args order: TAG, service_tick*100, heatingPwr, boiler_target_degC, boiler_temp_degC, pumpPwr
New args order: TAG, service_tick*100, extractionTimeMsecs, boiler_target_degC, boiler_temp_degC, heatingPwr, pumpPwr
```

Old format string:
```c
"%s;%08d;%04d;%.1f;%.2f;%04d;"
```

New format string:
```c
"%s;%08d;%.1f;%.1f;%.2f;%04d;%04d;"
```

> `Brew_time` printed with `%.1f` — value is `extractionTimeMsecs / 1000.0f` (seconds, one decimal).
> Value is `0.0` while idle or during active brew (reset at start, stored at stop).
> After brew completes it holds the last shot duration in seconds.

---

## Buffer Size — `SVC_LOG_LEN`

**File:** `espressoMachineServices.c`, line 36:
```c
#define SVC_LOG_LEN   90
```

The new format header string is ~114 chars when expanded with TAG prefix.
Increase to **120**.

```c
#define SVC_LOG_LEN   120
```

Data row max length (~62 chars) comfortably fits within 120.

---

## Files to Modify

| File | Change |
|---|---|
| `ble_espresso_app/components/Application/espressoMachineServices.c` | All touch points below |

---

## Touch Points

### L0 — `SVC_LOG_LEN` define (~line 36)

```c
/* before */
#define SVC_LOG_LEN   90

/* after */
#define SVC_LOG_LEN   120
```

---

### L1 — Classic mode: add format header print at initialization (~line 197)

Classic mode `is_app_initialized` block currently prints only the mode-entry message.
Add the column header print after it, matching profile mode style:

```c
#if(NRF_LOG_ENABLED == 1)
    sprintf((char*)log_text_arr,"%s;Espresso Machine enters into ::CLASSIC MODE::;",
                      TAG_SYS_MSG);
    NRF_LOG_RAW_INFO("%s\n",log_text_arr);
    NRF_LOG_FLUSH();
    sprintf((char*)log_text_arr,
            "%s;System_Time_Miliseconds;Brew_time;Boiler_Target_DegC;Boiler_Temp_DegC;Heating_Power;Pump_Power",
            TAG_SYS_FORMAT);
    NRF_LOG_RAW_INFO("%s\n",log_text_arr);
    NRF_LOG_FLUSH();
#endif
```

---

### L2 — Classic mode: data row print (~line 222)

```c
/* before */
sprintf((char *)log_text_arr,"%s;%08d;%04d;%.1f;%.2f;%04d;",
                  TAG_SYS_MONITOR,
                  service_tick*100,
                  Classic_data_s.heatingPwr,
                  boiler_target_temp_degC,
                  boiler_temp_degC,
                  app_pump_pwr);

/* after */
sprintf((char *)log_text_arr,"%s;%08d;%.1f;%.1f;%.2f;%04d;%04d;",
                  TAG_SYS_MONITOR,
                  service_tick*100,
                  g_Espresso_user_config_s.extractionTimeMsecs / 1000.0f,
                  boiler_target_temp_degC,
                  boiler_temp_degC,
                  Classic_data_s.heatingPwr,
                  app_pump_pwr);
```

---

### L3 — Profile mode: format header string (~line 476)

```c
/* before */
sprintf((char *)log_text_arr,
        "%s;Time_Miliseconds;Heating_Power;Boiler_Target_DegC;Boiler_Temp_DegC;Pump_Power",
        TAG_SYS_FORMAT);

/* after */
sprintf((char *)log_text_arr,
        "%s;System_Time_Miliseconds;Brew_time;Boiler_Target_DegC;Boiler_Temp_DegC;Heating_Power;Pump_Power",
        TAG_SYS_FORMAT);
```

---

### L4 — Profile mode: data row print (~line 498)

```c
/* before */
sprintf((char *)log_text_arr,"%s;%08d;%04d;%.1f;%.2f;%04d;",
                  TAG_SYS_MONITOR,
                  service_tick*100,
                  Profile_data_s.heatingPwr,
                  boiler_target_temp_degC,
                  boiler_temp_degC,
                  Profile_data_s.pumpPwr);

/* after */
sprintf((char *)log_text_arr,"%s;%08d;%.1f;%.1f;%.2f;%04d;%04d;",
                  TAG_SYS_MONITOR,
                  service_tick*100,
                  g_Espresso_user_config_s.extractionTimeMsecs / 1000.0f,
                  boiler_target_temp_degC,
                  boiler_temp_degC,
                  Profile_data_s.heatingPwr,
                  Profile_data_s.pumpPwr);
```

---

## Summary of Touch Points

| # | Location | Action |
|---|---|---|
| L0 | `espressoMachineServices.c` line 36 | Increase `SVC_LOG_LEN` 90 → 120 |
| L1 | Classic `is_app_initialized` block ~line 197 | Add `TAG_SYS_FORMAT` column header print |
| L2 | Classic `SERVICE_MONITOR_TICK` block ~line 222 | Update data row sprintf — add `Brew_time`, reorder `Heating_Power` |
| L3 | Profile `is_app_initialized` block ~line 476 | Update `TAG_SYS_FORMAT` string — rename + reorder columns |
| L4 | Profile `SERVICE_MONITOR_TICK` block ~line 498 | Update data row sprintf — add `Brew_time`, reorder `Heating_Power` |

---

## Dependency

This plan assumes `extractionTimeMsecs` exists in `espresso_user_config_t` and is
accessible via `g_Espresso_user_config_s.extractionTimeMsecs`.
Implement `EspressoMachineServices_ExtractionTime_Plan.md` first.

---

## Verification

1. Build: no compile errors; `SVC_LOG_LEN = 120` prevents buffer overflow on format header.
2. Serial output column header matches new format string exactly.
3. Data rows: `System_Time_Miliseconds` = `service_tick * 100`, `Brew_time` = 0 at idle,
   `Brew_time` = last shot duration (ms) after brew completes.
4. `Heating_Power` and `Pump_Power` appear in correct order (after temperature columns).
