# BLEspresso — Copilot Memory & Project Instructions

> **Purpose:** Single-source reference for GitHub Copilot agents and human developers working on the GaggiaControllerX codebase. Covers architecture, conventions, module responsibilities, data flows, known issues, and testing strategy.

---

## 1. Project Identity

| Field | Value |
|---|---|
| Project | GaggiaControllerX ("BLEspresso") |
| Hardware | nRF52832 (Cortex-M4F), PCA10040 dev kit |
| SDK | nRF5 SDK 17.1.0 (`nRF5_SDK_17.1.0_ddde560`) |
| SoftDevice | S132 v7.x (BLE 5.0) |
| IDE | Segger Embedded Studio |
| Language | C (C99) |
| Root | `ble_espresso_app/` — all application source lives here |
| Domain | Espresso machine controller — boiler temp regulation, pump pressure profiling, BLE remote configuration |

---

## 2. Architecture — Five Layers

```
L4  Scheduler         main.c                          Init + 20 ms flag-based cooperative loop
L3  BLE               bluetooth_drv, ble_cus           SoftDevice mgmt, custom GATT services
L2  Application       BLEspressoServices,              Mode state machines, PID regulation,
                      tempController, PumpController,   3-stage pump profiling, NVM persistence
                      StorageController
L1  Drivers+Utils     spi_Devices, solidStateRelay_    HW abstraction (SPI, SSR, GPIO, PWM)
                      Controller, ac_inputs_drv,        + reusable math (PID, filters, numbers)
                      dc12Vouput_drv, x205_PID_Block,
                      x201_DigitalFilters, x04_Numbers
L0  HW & SDK          nRF5 SDK drivers, S132           Treated as black boxes
```

**Dependency rule:** Each layer depends only on the layer(s) below it. Never call upward.

---

## 3. Key Files (paths relative to `ble_espresso_app/`)

| File | Role |
|---|---|
| `main/main.c` | Init sequence (21 steps) + cooperative scheduler loop |
| `components/Application/BLEspressoServices.c` | Mode state machines: Classic, Profile, StepFunction. Owns `blEspressoProfile` |
| `components/Application/BLEspressoServices.h` | Defines `bleSpressoUserdata_struct` and `blEspressoProfile` extern |
| `components/Application/tempController.c` | PID-IMC Type A boiler controller with adaptive I-gain |
| `components/Application/tempController.h` | Default PID gains: Kp=9.52156, Ki=0.3, Kd=0.0 |
| `components/Application/PumpController.c` | Multi-stage pump ramp (solenoid→ramp1→hold1→ramp2→hold2→rampDown→hold3→stop) |
| `components/Application/StorageController.c` | Read/write `blEspressoProfile` to W25Q64 external flash (65 bytes, Page 0) |
| `components/Peripherals/spi_Devices.c` | Shared SPI bus: MAX31865 RTD temp sensor + W25Q64 NVM |
| `components/Peripherals/solidStateRelay_Controller.c` | 3× SSR: boiler (zero-cross), pump (phase-angle), solenoid (on/off) |
| `components/Peripherals/ac_inputs_drv.c` | Debounced AC switch sensing via zero-crossing ISR counting |
| `components/BLE_Services/ble_cus.c` | Custom GATT services (Brew `0x1400`, PID `0x1500`) — 17 characteristics |
| `components/BLE/bluetooth_drv.c` | BLE stack init, `cus_evt_handler` (BLE writes → `blEspressoProfile`), `ble_update_boilerWaterTemp` (notify) |
| `components/Utilities/x205_PID_Block.c` | Generic PID library: `fcn_update_PIDimc_typeA` (used), `typeB` (available) |
| `components/Utilities/x04_Numbers.c` | `fcn_ChrArrayToFloat`, `fcn_FloatToChrArray`, `fcn_Constrain_WithinFloats` |

---

## 4. Central Data Hub: `blEspressoProfile`

**Type:** `volatile bleSpressoUserdata_struct blEspressoProfile` (global, defined in `BLEspressoServices.c`)

This struct is the shared data bus connecting BLE, storage, sensors, and all controllers.

### Fields

| Category | Fields | Size |
|---|---|---|
| NVM metadata | `nvmWcycles`, `nvmKey` | 2 × uint32 |
| Temperature | `temp_Target`, `temp_Boiler`, `sp_BrewTemp`, `sp_StemTemp` | 4 × float |
| Brew profile | `prof_preInfusePwr/Tmr`, `prof_InfusePwr/Tmr`, `Prof_DeclinePwr/Tmr` | 6 × float |
| PID params | `Pid_P_term`, `Pid_I_term`, `Pid_Iboost_term`, `Pid_Imax_term`, `Pid_D_term`, `Pid_Dlpf_term`, `Pid_Gain_term` | 7 × float |
| PID flags | `Pid_Iwindup_term` | 1 × bool |

### Writers

| Who | What | When |
|---|---|---|
| `bluetooth_drv.c` (`cus_evt_handler`) | All user-configurable fields | On BLE GATT write |
| `StorageController` (`stgCtrl_ReadUserData`) | All fields from NVM | Boot |
| `main.c` | `temp_Boiler` | Every 100 ms (SPI read) |
| `BLEspressoServices` | `temp_Target` | On brew↔steam mode switch |
| `main.c` (compile-time) | Test defaults | When `SET_TEST_USERDATA_EN == 1` |

### Readers

| Who | Fields | When |
|---|---|---|
| `tempController` | `temp_Boiler`, `temp_Target`, `Pid_*` | Every 500 ms (PID update) |
| `PumpController` | `prof_*` | On brew start / BLE config change |
| `BLEspressoServices` | All | Mode logic, logging |
| `bluetooth_drv` | `temp_Boiler` | Every 1000 ms (BLE notification) |
| `StorageController` | All | NVM store on BLE trigger |
| `ble_cus` | All | GATT characteristic init values |

---

## 5. Scheduler Timing

20 ms `app_timer` (RTC1) tick sets boolean flags; main super-loop polls them.

| Flag | Period | Task |
|---|---|---|
| `tf_ReadButton` | 60 ms | `fcn_SenseACinputs_Sixty_ms()` — debounce AC switches |
| `tf_GetBoilerTemp` | ~100 ms | `spim_ReadRTDconverter()` → `blEspressoProfile.temp_Boiler` |
| `tf_svc_EspressoApp` | 100 ms | `fcn_service_ClassicMode()` or `ProfileMode()` (App mode) |
| `tf_svc_StepFunction` | 100 ms | `fcn_service_StepFunction()` (Tune mode) |
| `tf_ble_update` | 1000 ms | `ble_update_boilerWaterTemp()` — BLE notification |

### Background ISRs

| ISR | Period | Purpose |
|---|---|---|
| `isr_HwTmr3_Period_EventHandler` | 1 ms | PID delta-time tracking (`milisTicks`) |
| `isr_ZeroCross_EventHandler` | ~8.33 ms | SSR zero-cross control + switch sensing |
| `acinBrew_eventHandler` / `acinSteam_eventHandler` | ~8.33 ms | Edge counting for AC switch debounce |

---

## 6. Brew Modes

### Classic Mode (`fcn_service_ClassicMode`)
Simple on/off: brew switch → pump 100% + solenoid ON. Steam switch → change setpoint to steam temp. Temperature PID runs continuously.

**Adaptive I-gain:** Ki×6.5 during brew (phase 1), Ki×2.0 post-brew recovery (phase 2), Ki×1.0 when within 1 °C of target.

### Profile Mode (`fcn_service_ProfileMode`)
3-stage pressure profile with exponential ramp transitions:
1. **Pre-infusion** — ramp 0 → `prof_preInfusePwr` (growth table)
2. **Infusion** — ramp → `prof_InfusePwr` (growth table)
3. **Decline** — ramp down → `Prof_DeclinePwr` (decay table)

Ramp tables: `a_expGrowth[14]`, `a_expDecay[14]` — pre-computed lookup, 10 sub-steps per stage.

### Step Function Mode (`fcn_service_StepFunction`)
Diagnostic/PID tuning: both switches held at startup → 30 s delay → 100% heater power → log temperature every 0.5 s.

### Mode Selection
At boot: both Brew+Steam switches asserted → Tune mode. Otherwise → App mode (currently hardcoded to Classic; Profile is available but commented out in `main.c`).

---

## 7. Temperature Control — PID-IMC Type A

| Parameter | Default | Source |
|---|---|---|
| Kp | 9.52156 | `TEMP_CTRL_KP` or BLE write |
| Ki | 0.3 | `TEMP_CTRL_KI` or BLE write |
| Ki boost (brew) | Ki × 6.5 | `Pid_Iboost_term` |
| Ki recovery | Ki × 2.0 | Hardcoded multiplier |
| Kd | 0.0 | `TEMP_CTRL_KD` or BLE write |
| Output limit | 1000 (100.0%) | `TEMP_CTRL_MAX` |
| Integral limit | 100.0 | `TEMP_CTRL_HIST_LIMIT` or BLE write |
| Anti-windup | Clamp scheme | Reduce integral error when output saturates |

**PID function:** `fcn_update_PIDimc_typeA()` in `x205_PID_Block.c`
- Input: `ProcessVariable` (temp_Boiler), `SetPoint` (temp_Target), `TimeMilis` (1 ms ticks)
- Output: heater power 0–1000
- Delta-time: computed from millisecond tick difference (1 ms HW timer 3)

---

## 8. BLE Services

**Device name:** `"BLEspresso"` | **UUID base:** `f364adc9-b000-4042-ba50-05ca45bf8abc`

### Brew Service (0x1400) — 10 characteristics

| UUID | Name | Access | Data |
|---|---|---|---|
| 0x1401 | Machine Status | R + Notify | 10-char string |
| 0x1402 | Boiler Water Temp | R + Notify | 4-char ASCII (°C × 10) |
| 0x1403 | Boiler Target Temp | R + W | 4-char ASCII |
| 0x140A | Steam Target Temp | R + W | 4-char ASCII |
| 0x1404 | Pre-Infusion Power | R + W | 3-char ASCII (%) |
| 0x1405 | Pre-Infusion Time | R + W | 3-char ASCII (s) |
| 0x1406 | Infusion Power | R + W | 3-char ASCII |
| 0x1407 | Infusion Time | R + W | 3-char ASCII |
| 0x1408 | Decline Power | R + W | 3-char ASCII |
| 0x1409 | Decline Time | R + W | 3-char ASCII |

### PID Service (0x1500) — 7 characteristics

| UUID | Name | Access |
|---|---|---|
| 0x1501 | P Term | R + W |
| 0x1502 | I Term | R + W |
| 0x1503 | I Max | R + W |
| 0x1504 | I Windup | R + W |
| 0x1505 | D Term | R + W |
| 0x1506 | D LPF | R + W |
| 0x1507 | Gain | R + W |

**Data format:** All characteristics use ASCII char arrays, converted via `fcn_ChrArrayToFloat` / `fcn_FloatToChrArray`.

**Write flow:** SoftDevice → `ble_cus.c:on_write()` → `bluetooth_drv.c:cus_evt_handler()` → parse → write to `blEspressoProfile.*`

**Notification flow (temp):** `main.c` (1 s flag) → `ble_update_boilerWaterTemp(temp_Boiler)` → float→ASCII → GATT notify on char 0x1402

---

## 9. External Memory — W25Q64FV

| Property | Value |
|---|---|
| Part | W25Q64FV (Winbond, JEDEC `0xEF 0x4017`) |
| Capacity | 8 MB (64 Mbit) |
| Interface | SPI Mode 3, 1 MHz, shared bus with MAX31865 |
| CS Pin | GPIO 15 |

**Application uses only Page 0 (65 of 256 bytes):**

| Address | Field | Size |
|---|---|---|
| 0x00–0x03 | `nvmWcycles` (MSW=shot writes, LSW=ctrl writes) | 4 B |
| 0x04–0x07 | `nvmKey` = `0x00AA00AA` (magic, `0xFFFFFFFF`=empty) | 4 B |
| 0x08–0x27 | Shot Profile (temp_Target, brew params) | 32 B |
| 0x28–0x40 | Controller (PID params + Iwindup) | 25 B |
| 0x41–0xFF | Unused | 191 B |

**Write strategy:** Read-modify-write. Shot profile and controller sections are independent — writing one preserves the other.

---

## 10. Hardware Pin Map (PCA10040)

| Pin | Function | Conflict on DK |
|---|---|---|
| P0.11 | SPI MISO | — |
| P0.12 | SPI MOSI | — |
| P0.13 | SPI SCK | DK Button 1 |
| P0.14 | SPI CS (MAX31865) | DK Button 2 |
| P0.15 | SPI CS (W25Q64) | — |
| P0.17 | NVM Write Protect | — |
| P0.18 | NVM Hold | — |
| P0.19 | Boiler SSR output | DK LED 3 |
| P0.20 | Pump SSR output | DK LED 4 |
| P0.25 | Solenoid SSR output | SWDCLK (debug conflict!) |
| P0.26 | Zero-cross input | — |
| P0.27 | Brew switch input | — |
| P0.28 | Steam switch input | — |
| P0.29 | Debug LED / heartbeat | — |

---

## 11. Coding Conventions

- **Module pattern:** Each module = one `.c` + one `.h` with matching names
- **Function prefix:** `fcn_` for public functions, module-specific prefix (e.g., `spim_`, `stgCtrl_`, `fcn_service_`)
- **ISR naming:** `isr_` prefix + handler name (e.g., `isr_ZeroCross_EventHandler`)
- **State machines:** Use `StateMachineCtrl_Struct { sPrevious, sRunning, sNext }` from `x01_StateMachineControls.h`
- **Power values:** Fixed-point ×10: 0–1000 represents 0.0–100.0 %
- **Logging:** `NRF_LOG_*` macros, gated by `#if(NRF_LOG_ENABLED == 1)`
- **Conditional compilation:** `SET_TEST_USERDATA_EN`, `EXCLUDE_NVM_SECTION`, `SERVICE_PUMP_ACTION_EN`, `SERVICE_HEAT_ACTION_EN`
- **Utilities prefix:** `x01_`, `x02_`, ..., `x205_` — numbered utility library convention
- **BLE data format:** ASCII char arrays (not raw binary floats)

---

## 12. Known Issues & Safety Concerns

### Critical (must fix before AC power testing)

| ID | Issue | Impact |
|---|---|---|
| C1 | No watchdog timer | Firmware hang → SSR stuck ON → thermal runaway |
| C2 | P0.25 (solenoid) is SWDCLK | Debugger toggles solenoid during flash/debug |
| C3 | GPIO undefined during boot | SSR outputs may float HIGH for ~2 s before init |

### High (safety-related)

| ID | Issue | Impact |
|---|---|---|
| H1 | Division by zero in pump slope calc | BLE write of `Time_Rampup_ms = 0` → hard fault |
| H2 | No BLE input validation | Any float accepted — negative temps, zero timers |
| H3 | No temp sensor failure detection | Open/shorted RTD → PID runs on garbage data |
| H4 | No overheat protection in step-function mode | 100% power with no temp limit |
| H5 | No maximum brew time limit | Stuck brew switch → indefinite pump/heater operation |

### Medium (functional correctness)

| ID | Issue | Impact |
|---|---|---|
| M1 | No integral reset on setpoint change | Brew→steam jump → temperature overshoot |
| M2 | Timer period 1009 µs not 1000 µs | PID timing drifts ~0.9% |
| M3 | No NVM data validation | Corrupted flash → unsafe parameters loaded |
| M4 | No atomic BLE parameter update | Disconnect mid-write → mixed old/new profile |
| M5 | I-gain boost reapplied on rapid brew cycling | Integral accumulation without cooldown |

---

## 13. Testing Strategy

Six phases, sequential (each must pass before starting the next):

| Phase | Environment | Scope | Key Tools |
|---|---|---|---|
| 1 | Host PC (gcc + Unity) | Pure algorithms (PID, filters, numbers) | Unity framework |
| 2 | Host PC (gcc + Unity + FFF) | App logic with mocked HW | FFF fake functions, stub nRF headers |
| 3 | Host PC (Ceedling + CMock) | Same as 2, strict call ordering + coverage | Ceedling, CMock, gcov |
| 4 | nRF52-DK (bare, no external HW) | MCU peripherals + BLE | Unity on-target via RTT, bleak (Python BLE) |
| 5 | DK + breakout peripherals | Real SPI devices, simulated zero-cross | Logic analyzer, scope |
| 6 | Production board + Gaggia machine | Full system integration | Manual verification + serial log |

**Key test infrastructure:**
- `tests/stubs/` — minimal nRF SDK header stubs for host compilation
- FFF fakes for: `solidStateRelay_Controller`, `spi_Devices`, `ac_inputs_drv`, `bluetooth_drv`
- Unity output via SEGGER RTT on target
- Python `bleak` scripts for BLE service verification

---

## 14. Documentation Map

| Document | Path | Content |
|---|---|---|
| Technical Summary | `TECHNICAL_SUMMARY.md` | High-level architecture, scheduler, functional description |
| Test Plan | `TEST_PLAN.md` | 6-phase test strategy, known issues, verification criteria |
| Architecture Analysis | `docs/ArchitectureAnalysis.md` | AI analysis instructions, diagram conventions, SVG generation, + generated architecture section |
| External Memory Map | `docs/mem/external_mem.md` | W25Q64 byte-level address table + Mermaid block diagram |
| External Memory Diagram | `docs/mem/external_mem.mermaid` / `.svg` | Standalone Mermaid source + rendered SVG |
| Module Maps | `docs/architecture/modules/modules.md` | All modules: peripherals, utilities, BLE, application, main (tables + embedded Mermaid) |
| Module Diagrams | `docs/architecture/modules/modules-pid-control_diagram.mermaid/.svg` | PID-IMC Type A control loop |
| Module Diagrams | `docs/architecture/modules/modules-layer-view_diagram.mermaid/.svg` | main.c interaction — layer view |
| Module Diagrams | `docs/architecture/modules/modules-data-hub_diagram.mermaid/.svg` | `blEspressoProfile` writers/readers |
| System Design | `docs/architecture/system/system_design.md` | Layer view + component-interaction view (Mermaid embedded) |
| System Diagrams | `docs/architecture/system/system-layer-view_diagram.mermaid/.svg` | L0–L4 horizontal layer stack |
| System Diagrams | `docs/architecture/system/system-component-interaction_diagram.mermaid/.svg` | Data-centric hub view |
| System Diagrams | `docs/architecture/system/system-ble-lifecycle_diagram.mermaid/.svg` | BLE connection state machine |
| Flow Index | `docs/architecture/flows/index.md` | Index of 7 data flows |
| Individual Flows | `docs/architecture/flows/[flow-name]/` | Each flow: `README.md` + `[flow-name]_diagram.mermaid` + `[flow-name]_diagram.svg` |

---

## 15. Diagram File Conventions

All diagrams follow the rules defined in `docs/ArchitectureAnalysis.md § 3`.

### File Format Rules
- `.mermaid` files are **pure Mermaid syntax** — no markdown headers, no code fences
- Files start directly with the diagram type keyword (e.g., `flowchart LR`, `stateDiagram-v2`)
- Each `.mermaid` must have a matching `.svg` generated alongside it (same folder, same base name)

### Naming Convention

| Location | Pattern | Example |
|---|---|---|
| `docs/architecture/flows/[flow-name]/` | `[flow-name]_diagram.mermaid` | `brew-classic_diagram.mermaid` |
| `docs/architecture/modules/` | `modules-[description]_diagram.mermaid` | `modules-pid-control_diagram.mermaid` |
| `docs/architecture/system/` | `system-[description]_diagram.mermaid` | `system-layer-view_diagram.mermaid` |
| `docs/mem/` | `[name].mermaid` | `external_mem.mermaid` |

### Current Diagram Inventory (14 files)

**Flows** (`docs/architecture/flows/*/`):
- `ble-temp-notification_diagram` — SPI RTD → blEspressoProfile → BLE notify
- `ble-to-profile_diagram` — BLE GATT writes → blEspressoProfile fields
- `boiler-temp-control_diagram` — MAX31865 → PID-IMC → SSR boiler
- `brew-classic_diagram` — AC switches → Classic mode state machine → SSRs
- `brew-profile_diagram` — Brew switch → 3-stage pump ramp
- `nvm-persistence_diagram` — blEspressoProfile ↔ W25Q64 boot/store
- `profile-to-modules_diagram` — blEspressoProfile → controllers/storage

**Modules** (`docs/architecture/modules/`):
- `modules-pid-control_diagram` — PID-IMC Type A closed-loop diagram
- `modules-layer-view_diagram` — main.c interaction with all layers
- `modules-data-hub_diagram` — blEspressoProfile writers and readers

**System** (`docs/architecture/system/`):
- `system-layer-view_diagram` — L0–L4 layer stack (flowchart TB)
- `system-component-interaction_diagram` — data-centric hub view (flowchart LR)
- `system-ble-lifecycle_diagram` — BLE connection state machine (stateDiagram-v2)

**Memory** (`docs/mem/`):
- `external_mem` — W25Q64 address regions and StorageController mapping

### SVG Regeneration (Windows — run from workspace root)
```powershell
Get-ChildItem -Path "docs" -Recurse -Filter "*.mermaid" | ForEach-Object {
    $svg = [System.IO.Path]::ChangeExtension($_.FullName, ".svg")
    cmd /c "mmdc -i `"$($_.FullName)`" -o `"$svg`""
}
```
> PowerShell execution policy may block `mmdc.ps1` — use `cmd /c "mmdc ..."` as shown.

---

## 16. Quick Reference for AI Agents

When working on this codebase:

1. **All paths** are relative to `ble_espresso_app/` unless stated otherwise
2. **`blEspressoProfile`** is the central data hub — trace any data question through it
3. **No RTOS** — single-threaded cooperative loop; all code must be non-blocking
4. **Power values** are 0–1000 (fixed-point ×10 for 0.0–100.0 %)
5. **BLE data** is ASCII char arrays, not raw binary — use `fcn_ChrArrayToFloat` / `fcn_FloatToChrArray`
6. **Two BLE services:** Brew (0x1400, 10 chars) and PID (0x1500, 7 chars)
7. **PID type:** IMC Type A (`fcn_update_PIDimc_typeA`) with adaptive I-gain (6.5×/2×/1×)
8. **External flash** uses only 65 bytes of an 8 MB chip — Page 0, addresses 0x00–0x40
9. **Safety:** No watchdog, no input validation, no sensor failure detection — see Known Issues
10. **SPI bus shared** between MAX31865 (temp) and W25Q64 (NVM) — software CS selection
11. **DK pin conflicts:** P0.13-14 (buttons vs SPI), P0.19-20 (LEDs vs SSR), P0.25 (SWDCLK vs solenoid)
12. **Profile mode** exists but is commented out in `main.c` — Classic mode is active
13. **Step function mode** = both switches held at power-on (diagnostic/PID tuning)
14. **NVM key** `0x00AA00AA` at address 0x04 indicates valid stored data
15. **Diagrams:** 14 `.mermaid` files, each with a matching `.svg` — see Section 15 for naming rules
16. **Do not edit `.svg` files directly** — regenerate from `.mermaid` source using `mmdc`
