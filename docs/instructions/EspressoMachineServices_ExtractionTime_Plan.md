#problem analysis
Done with analysis. Here are the identified causes, ordered by likelihood:

1. svcStartT type narrowing — code correctness bug (pre-existing, now exposed)

Classic_data_s.svcStartT and Profile_data_s.svcStartT are uint16_t. service_tick is uint32_t. After ~6,553 seconds of uptime (service_tick > 65535), the assignment truncates silently. The extractionTimeMsecs computation then wraps incorrectly. Not a memory fault but produces garbage values.

2. BSS growth — possible RAM overflow

espresso_user_config_t grew from 80 → 84 bytes (the new float field plus pre-existing 3-byte padding after bool pidIwindupTerm). Two global instances exist:

g_Espresso_user_config_s in espressoMachineServices.c
read_NvmData in bluetooth_drv.c
BSS grew by +8 bytes. On nRF52832 with S132 (~56KB application RAM), this is unlikely alone — but check the .map file for stack headroom. If RAM was already at its limit, 8 bytes can push the stack into the MPU-protected SD region and trigger NRF_FAULT_ID_APP_MEMACC.

3. PROFILE_MODE_PREINFUSE MAX_BREW_TICKS deviates from plan spec

The plan explicitly said this path "goes directly to PROFILE_IDLE and is NOT refactored." Current code redirects to PROFILE_MODE_STOP instead. This means pump and solenoid stay on for an extra 100ms tick before PROFILE_MODE_STOP shuts them down. Not a crash cause, but a behavioral deviation.

4. svr_duration_msecs assigned outside log guard — dead store if logging disabled

In both CLASSIC_MODE_1 ~line 337 and PROFILE_MODE_STOP ~line 763, svr_duration_msecs is now assigned unconditionally but only used inside #if NRF_LOG_ENABLED == 1. With logging off, this is a dead store — no crash, but generates a compiler warning and the local variable can be removed when logging is disabled.

To pin down the SOFTDEVICE fault precisely: open the .map file and check the distance between end of BSS/data and bottom of stack. If < ~512 bytes headroom, that's the problem.


## Context
The espresso machine services already compute shot duration locally in `svr_duration_msecs`, but only inside `#if NRF_LOG_ENABLED` guards — the value is never stored anywhere accessible. The goal is:
1. Add `float extractionTimeMsecs` to `espresso_user_config_t` to persist the shot duration (in milliseconds) across the brew cycle.
2. Eliminate duplicated brew-stop code scattered across Profile mode cases by consolidating it into `PROFILE_MODE_STOP`.

Tracking applies only while `swBrew == AC_SWITCH_ASSERTED`.

---

## Files to Modify

| File | Change |
|---|---|
| `ble_espresso_app/components/Application/espressoMachineServices.h` | Add field to struct |
| `ble_espresso_app/components/Application/espressoMachineServices.c` | Reset on start, compute+store on stop, profile refactor |

No changes to NVM storage, BLE characteristics, or BLE notifications — not requested.

---

## Step 1 — Add field to struct

**File:** `espressoMachineServices.h` — inside `espresso_user_config_t`, after `pidIwindupTerm`:

```c
bool  pidIwindupTerm;
float extractionTimeMsecs;
```

---

## Step 2 — Reset at brew START

At the existing `svcStartT` assignment in each mode, add a zero-reset on the same line block.

**Classic mode** — `CLASSIC_IDLE → CLASSIC_MODE_1` (~line 260):
```c
Classic_data_s.svcStartT = service_tick;
g_Espresso_user_config_s.extractionTimeMsecs = 0.0f;
```

**Profile mode** — `PROFILE_IDLE → PROFILE_MODE_PREINFUSE` (~line 559):
```c
Profile_data_s.svcStartT = service_tick;
g_Espresso_user_config_s.extractionTimeMsecs = 0.0f;
```

---

## Step 3 — Classic Mode: compute and store at every STOP

Formula (milliseconds, using existing pattern):
```c
g_Espresso_user_config_s.extractionTimeMsecs =
    (float)((service_tick - Classic_data_s.svcStartT) * SERVICE_BASE_TIME_MSECS);
```

The assignment is **unconditional** — placed before (outside) any `#if NRF_LOG_ENABLED` block. The log guard is for display only and must not gate the measurement.

| # | Location | Trigger | Placement |
|---|---|---|---|
| C1 | `CLASSIC_MODE_1` ~line 316 else branch | `swBrew` de-asserted | immediately before `#if NRF_LOG_ENABLED` at ~line 331 — outside the guard |
| C2 | `CLASSIC_MODE_1` ~line 301 `MAX_BREW_TICKS` block | auto-stop after 120s | before `break` at ~line 312 — no log guard present here |

---

## Step 4 — Profile Mode: refactor duplicate stop code into `PROFILE_MODE_STOP`

### Current problem
Three cases (`PROFILE_MODE_PREINFUSE`, `PROFILE_MODE_INFUSE`, `PROFILE_MODE_DECLINE`) each have an identical `else` body when `swBrew` de-asserts:

```c
if(swBrew == AC_SWITCH_ASSERTED)
{ ... }
else{
    Profile_data_s.is_active = false;
    Profile_data_s.is_boostI_phase1 = false;
    Profile_data_s.is_boostI_phase2 = true;
    temp_ctrl_scale_integral_gain(..., 2.0f);
    app_pump_pwr = PUMP_PWR_OFF;
    pump_ssr_pwr_update(app_pump_pwr);
    solenoid_ssr_off();
    Profile_service_status_s.sRunning = PROFILE_IDLE;
    #if NRF_LOG_ENABLED ... svr_duration_msecs ... #endif
}
```

`PROFILE_MODE_STOP` already contains this exact cleanup block (guarded by `is_active == true`), followed by a `swBrew` de-assert check that transitions to `PROFILE_IDLE`.

### Refactor

**In `PROFILE_MODE_PREINFUSE` else, `PROFILE_MODE_INFUSE` else, and `PROFILE_MODE_DECLINE` else:**
Replace the entire duplicated stop body with a single state jump:

```c
}else{
    Profile_service_status_s.sRunning = PROFILE_MODE_STOP;
}
```

**In `PROFILE_MODE_STOP` — `if(Profile_data_s.is_active == true)` block (~line 825):**
Add `extractionTimeMsecs` assignment **before** the `#if NRF_LOG_ENABLED` guard — unconditional, always executes regardless of logging:

```c
/* Always runs — outside log guard */
g_Espresso_user_config_s.extractionTimeMsecs =
    (float)((service_tick - Profile_data_s.svcStartT) * SERVICE_BASE_TIME_MSECS);

/* Log-only block — does NOT affect extractionTimeMsecs */
#if(NRF_LOG_ENABLED == 1)
    svr_duration_msecs = g_Espresso_user_config_s.extractionTimeMsecs / 1000.0f;
    sprintf(...)
    ...
#endif
```

> `svr_duration_msecs` inside the log block is derived from `extractionTimeMsecs` (÷1000 for display in seconds). The computation itself lives outside the guard.

`PROFILE_MODE_STOP` already handles:
- One-shot cleanup via `is_active == true` guard (pump off, solenoid off, I-gain reset)
- Wait for `swBrew` to de-assert → `PROFILE_IDLE`

Because `swBrew` is already de-asserted when control arrives from PREINFUSE/INFUSE/DECLINE, `PROFILE_MODE_STOP` will complete cleanup and transition to `PROFILE_IDLE` on the next service tick (100 ms later).

### `MAX_BREW_TICKS` auto-stop in `PROFILE_MODE_PREINFUSE` (~line 595)
This is a separate guard (not the `swBrew` else branch) — it goes directly to `PROFILE_IDLE` and is **not** refactored. Add `extractionTimeMsecs` assignment there before the `break`:

```c
g_Espresso_user_config_s.extractionTimeMsecs =
    (float)((service_tick - Profile_data_s.svcStartT) * SERVICE_BASE_TIME_MSECS);
...
break;
```

---

## Summary of Touch Points

| # | File / Location | Action |
|---|---|---|
| H1 | `espressoMachineServices.h` line 59 | Add `float extractionTimeMsecs` to struct |
| C-start | Classic `CLASSIC_IDLE` ~line 260 | Reset `extractionTimeMsecs = 0.0f` |
| C1 | Classic `CLASSIC_MODE_1` ~line 331 | Compute + store (swBrew de-assert) |
| C2 | Classic `CLASSIC_MODE_1` ~line 312 | Compute + store (MAX_BREW_TICKS) |
| P-start | Profile `PROFILE_IDLE` ~line 559 | Reset `extractionTimeMsecs = 0.0f` |
| P1 | Profile `PROFILE_MODE_PREINFUSE` else ~line 636 | Replace stop body → `PROFILE_MODE_STOP` |
| P2 | Profile `PROFILE_MODE_PREINFUSE` ~line 604 | Compute + store (MAX_BREW_TICKS) + keep → `PROFILE_IDLE` |
| P3 | Profile `PROFILE_MODE_INFUSE` else ~line 714 | Replace stop body → `PROFILE_MODE_STOP` |
| P4 | Profile `PROFILE_MODE_DECLINE` else ~line 793 | Replace stop body → `PROFILE_MODE_STOP` |
| P5 | Profile `PROFILE_MODE_STOP` `is_active` block ~line 836 | Compute + store `extractionTimeMsecs` |

---

## Key Reuse

- Formula reuses `svr_duration_msecs` pattern already in the file; `SERVICE_BASE_TIME_MSECS` (= 100) already defined at line 30.
- `PROFILE_MODE_STOP` cleanup block already correct — no duplication needed.
- `svcStartT` fields already set at brew start — no new bookkeeping.

---

## Verification

1. Build: no compile errors; struct size increase of 4 bytes is non-breaking (field not mapped to NVM).
2. Classic swBrew de-assert: hold ~25s, release → `extractionTimeMsecs ≈ 25000.0`.
3. Classic auto-stop: hold >120s → `extractionTimeMsecs ≈ 120000.0`.
4. Profile full run to `PROFILE_MODE_STOP`: `extractionTimeMsecs` matches total profile time.
5. Profile mid-stage abort (release swBrew during any stage): state transitions through `PROFILE_MODE_STOP`, `extractionTimeMsecs` captures partial elapsed time.
6. Log check: existing `svr_duration_msecs` (`/1000.0f` = seconds) should equal `extractionTimeMsecs / 1000.0f`.
