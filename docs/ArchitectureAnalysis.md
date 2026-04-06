# AI Architecture Analysis

This document contains the instructions for an important task - using AI to define the architecture of this embedded system, so that it can be used by humans and AI agents to more easily understand the embedded system. A C module consist of .h file and a .c file with the same name.

## 1. Objective

Map out all mid-level drivers/modules inside ../Peripherals folder, ulities inside ../Utilities folder, all driver for BLE services/modules inside ../BLE folder and high-level layer/modules inside ../Application folder. Additionally, map out main.c inside ../main folder and how it interacts with all previously described-driver/modules.
Map out all data from flow input type peripherals to output type peripherals or to the BLE radio; Also data flow from BLE radio to output type peripherals. And all other dat flow not specified but detected by your analysis of this application. A flow should map out the end-to-process data read from low-level driver (SDK) to other low-level driver (SDK) or BLE driver (SDK), also considerting the flow from low-level driver (SDK) or BLE driver (SDK) to write to low-level driver (SDK). treat low-level driver (SDK) as a black box, SD used for this application is nRF5_SDK_17.1.0_ddde560 and it is well documented by Nordic.  

Flows should be documented in Mermaid format to allow AI agents to understand, for versioning (in git), and for easy visualization.

Moreover, Map out the registers of external memory (W25Q64) along with a clear identifcation of the address used by the StorageControlle module and the embedded application. Memory map should be documented in Mermaid format to allow AI agents to understand, for versioning (in git), and for easy visualization.

## 2. Requirements
1. Print external memory (W25Q64) table in .md format
    + **Output:** [`docs/mem/extenal_mem.md`](mem/extenal_mem.md)
    + **Layout — Table + Mermaid block diagram** (shows byte-level address map and logical sections)
2. Map out mid-level drivers/modules.
3. Map out ulities files.
4. Map out BLE services/modules.
5. Map out high-level drivers/modules.
6. Map out tempController and type of PID controller used by app.
    + **Layout 1 — Select best practice layout for control system using:** Mermaid 
7. Map out main and its interaction with all other mid-level, utilities, BLE services and high-level modules.
    + **Layout 1 — Layer View using:** Mermaid 
8. Map out var: blEspressoProfile interaction with disctinct modules

9. Print Maps from point 2 to 8 into one single file
    + **Output:** [`docs/architecture/modules/modules.md`](modules/modules.md)

10. BLE Notifications flow from mid-level drivers/modules to BLE services
    + Data read from spi_Devices into var: blEspressoProfile and then to BLE
11. Data flow - from var: blEspressoProfile to any module
    + Data read from var: blEspressoProfile into any module
12. Data flow - from modules to var: blEspressoProfile
    + Data wrtie var: blEspressoProfile from module to var.


13. Update this file's section: ## 4. Architecure; Describe the desing of this system on layers
    + **Output:** [## 4. Architecture](#4-architecure) below — layers description with data flow diagrams, BLE connection lifecycle, key design decisions, mains flow and timings.
14. Generate in docs/architecture a file that describes the desing of this system using to different layout: Layer and other to your choose (tell me which one you decided to use)
    + **Output:** [`docs/architecture/system/system_design.md`](architecture/system/system_design.md)
    + **Layout 1 — Layer View:** Mermaid flowchart TB — horizontal layers from HW/SDK up to Scheduler
    + **Layout 2 — Component-Interaction View (Data-Centric):** Mermaid flowchart LR — shows blEspressoProfile as central data hub with producers/consumers

15. Extract and generate the full GATT attribute table for this project
    + Clearly identify for each characteristic: **Characteristic Declaration**, **Characteristic Value**, **Client Characteristic Configuration Descriptor (CCCD)**, and **Characteristic User Description Descriptor (CUDD)**
    + Include: UUID (16-bit + full 128-bit base), Properties, Value length, Data format, corresponding `blEspressoProfile` field, and ATT handle offsets
    + **Output:** [`docs/ble/gatt_table.md`](ble/gatt_table.md)
    + **Diagram:** Create a GATT diagram in Mermaid `timeline` format, where each characteristic is a timeline step. Use information from `gatt_table.md` for each characteristic's UUID, properties, data length, format and `blEspressoProfile` field. Save as `gatt_table.mermaid` alongside `gatt_table.md`. **Does NOT follow § 3 diagram.mermaid Requirements** — uses `timeline` diagram type, no swimlane format.
    + **Image:** Generate a `.svg` image from the Mermaid file. Save as `gatt_table.svg` in the same folder.

The list of flows should live in ../flows/index.md and each individual flow should be defined in a separate folder.

# Where to find information

- TECHNICAL_SUMMARY.md contains describing the design of this system and domain knowledge

## 3. diagram.mermaid Requirements

**CRITICAL RULES:**

1. **File Format**: Must be pure Mermaid syntax with `.mermaid` extension
   - NO markdown headers
   - NO markdown code fences (no ` ```mermaid ` wrapper)
   - Starts directly with `flowchart LR`

2. **Use Swimlane Format**: `flowchart LR` (left-to-right with horizontal swimlanes)
   - Each repository is a horizontal swimlane (subgraph)
   - Flow progresses left to right
   - Swimlane labels should be prominent (use emoji for visibility)
   - Example: `subgraph systemA["🔧 systemA"]`

3. **Systems as Containers**:
   - Each repository MUST be a `subgraph` (horizontal swimlane)
   - Repository name is the subgraph label
   - Operations are nodes inside the subgraph
   - Use `direction LR` inside each subgraph

### Documentation Structure

Each flow must be documented in a separate folder with the following structure:

```
docs/architecture/flows/[flow-name]/
├── README.md                          # Each flow is documented in a dedicated folder (all content in one file)
└── [flow-name]_diagram.mermaid        # Mermaid diagram — named after the containing folder
```

**Naming convention:** The `.mermaid` file must be named `[flow-name]_diagram.mermaid`, where `[flow-name]` matches the folder name exactly (e.g., `brew-classic_diagram.mermaid` inside `brew-classic/`).

**IMPORTANT**: Use ONLY these two files. Do NOT create separate diagram-notes.md or other files. Keep all documentation consolidated in README.md for easier maintenance.

### README.md Contents

**Use the blueprint as a template**: `docs/architecture/flows/[redacted]/`

---

### SVG Generation

Every `.mermaid` file must have a matching `.svg` file generated alongside it (same folder, same base name). SVGs are produced with [`@mermaid-js/mermaid-cli`](https://github.com/mermaid-js/mermaid-cli) (`mmdc`).

**Install (once):**
```
npm install -g @mermaid-js/mermaid-cli
```

**Generate all SVGs** (run from workspace root `c:\WS\NRF\GaggiaController`):
```powershell
Get-ChildItem -Path "docs" -Recurse -Filter "*.mermaid" | ForEach-Object {
    $svg = [System.IO.Path]::ChangeExtension($_.FullName, ".svg")
    cmd /c "mmdc -i `"$($_.FullName)`" -o `"$svg`""
}
```

**Or single file:**
```
mmdc -i docs/architecture/flows/[flow-name]/[flow-name]_diagram.mermaid -o docs/architecture/flows/[flow-name]/[flow-name]_diagram.svg
```

**Scope:** Applies to all `.mermaid` files under:
- `docs/architecture/flows/[flow-name]/` — one file per flow folder
- `docs/architecture/modules/` — module map diagrams
- `docs/architecture/system/` — system design diagrams
- `docs/mem/` — memory map diagrams

**Rule:** Every `.mermaid` commit must include the regenerated `.svg`. Keep both files in sync.

> **Note (Windows):** `mmdc` is installed as a `.ps1` script. If PowerShell execution policy blocks it, invoke via `cmd /c "mmdc ..."` as shown above.

## 4. Architecture

### System Layers

The BLEspresso controller is organized into five distinct layers, each with clear responsibilities and dependency direction (top depends on bottom only):

| Layer | Name | Components | Responsibility |
|---|---|---|---|
| **L4** | Scheduler | `main.c` | System init, 20 ms cooperative flag scheduler, mode selection |
| **L3** | BLE Communication | `bluetooth_drv`, `ble_cus` | SoftDevice S132 management, custom GATT services (0x1400 Brew, 0x1500 PID), data serialization |
| **L2** | Application Controllers | `BLEspressoServices`, `tempController`, `PumpController`, `StorageController` | Mode state machines (Classic/Profile/StepFcn), PID-IMC boiler regulation, 3-stage pump profiling, NVM persistence |
| **L1** | Peripheral Drivers + Utilities | `spi_Devices`, `solidStateRelay_Controller`, `ac_inputs_drv`, `dc12Vouput_drv`, `x205_PID_Block`, `x201_DigitalFilters`, `x04_Numbers` | Hardware abstraction: SPI (MAX31865 + W25Q64), SSR control (zero-cross + phase-angle), AC input debounce, PWM. Reusable math algorithms. |
| **L0** | Hardware & SDK | nRF5 SDK 17.1.0, S132 SoftDevice | Nordic SPI/Timer/GPIO/PWM drivers, BLE stack. Treated as black boxes. |

### Central Data Bus: `blEspressoProfile`

All runtime configuration flows through a single global volatile struct (`bleSpressoUserdata_struct blEspressoProfile`). This struct is:

- **Written by:** BLE GATT write events (mobile app → `cus_evt_handler`), NVM boot load (`stgCtrl_ReadUserData`), main scheduler (temperature reads), mode state machines (setpoint switching)
- **Read by:** `tempController` (PV + SP for PID), `PumpController` (brew profile params), `BLEspressoServices` (mode logic), `bluetooth_drv` (BLE notifications), `StorageController` (NVM writes), `ble_cus` (characteristic init values)

### Data Flow Summary

```
Mobile App ──BLE write──→ blEspressoProfile ──→ tempController ──→ Boiler SSR
                              ↑                  ──→ PumpController ──→ Pump SSR
                              │                  ──→ BLEspressoServices ──→ Solenoid SSR
   W25Q64 NVM ──boot load────┘
                              │
   MAX31865 RTD ──SPI 100ms──→ temp_Boiler
                              │
                              └──→ bluetooth_drv ──BLE notify──→ Mobile App
```

### Scheduling Model

Cooperative super-loop with a 20 ms RTC-driven software timer (`app_timer`). The ISR sets boolean flags; the main loop polls them:

- **60 ms** — AC input debounce (count zero-crossings vs. threshold)
- **100 ms** — SPI temperature read (non-blocking 2-state machine), espresso service tick
- **500 ms** — (within service) PID update + heater SSR write + serial log
- **1000 ms** — BLE temperature notification to mobile app

Background ISRs: 1 ms HW timer (PID delta-time), ~8.3 ms zero-cross events (SSR + switch sensing).

### Temperature Control

PID-IMC Type A controller (`fcn_update_PIDimc_typeA`) with adaptive integral gain:
- **During brew** (pump active): Ki × 6.5 — compensates heat loss from cold water flow
- **Post-brew recovery**: Ki × 2.0 — accelerates return to setpoint
- **Steady state**: Ki × 1.0 — reverts when temp is within 1 °C of target
- **Anti-windup**: output-clamp scheme reduces integral accumulator when PID output saturates

### BLE Interface

Two custom GATT services under 128-bit UUID base `f364adc9-b000-4042-ba50-05ca45bf8abc`:
- **Brew Service (0x1400)**: 10 characteristics — machine status (notify), boiler temp (notify), target temps (R/W), 6× brew profile params (R/W)
- **PID Service (0x1500)**: 7 characteristics — P, I, I-max, I-windup, D, D-LPF, Gain (all R/W)

Connection lifecycle: Advertising (fast) → Connected → Characteristic R/W + Notifications (1 s temp) → Disconnect → re-Advertise.

### Mode Selection

At startup: if both Brew and Steam switches are held → **Tune mode** (step-function heater test for PID identification). Otherwise → **App mode** (Classic or Profile espresso operation).

### Generated Documentation

| Requirement | Output | Layout |
|---|---|---|
| 1. External memory map | [`docs/mem/external_mem.md`](mem/external_mem.md) | Table + Mermaid block diagram |
| 2–8. Module maps | [`docs/architecture/modules/modules.md`](architecture/modules/modules.md) | Tables + Mermaid flowcharts |
| 9. System design | [`docs/architecture/system/system_design.md`](architecture/system/system_design.md) | **Layout 1:** Layer View (Mermaid flowchart TB), **Layout 2:** Component-Interaction View (Mermaid flowchart LR, data-centric) |
| 10–12. Data flows | [`docs/architecture/flows/index.md`](architecture/flows/index.md) | Mermaid flowchart LR per flow |
| 15. GATT attribute table | [`docs/ble/gatt_table.md`](ble/gatt_table.md) | Full ATT table: Declaration + Value + CCCD + CUDD for all 17 characteristics across 2 services |
| 15. GATT diagram | [`docs/ble/gatt_table.mermaid`](ble/gatt_table.mermaid) / [`gatt_table.svg`](ble/gatt_table.svg) | `timeline` diagram — each characteristic as a step, grouped by service, showing UUID, properties, length, format, blEspressoProfile field |

