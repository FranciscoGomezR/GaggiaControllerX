# Flow: Brew Cycle — Profile Mode

## Overview

Documents the 3-stage espresso pressure profile with exponential ramp transitions. Uses configurable power and duration per stage.

## Trigger

User asserts Brew switch while system is in Profile Mode.

## Stages

| Stage | Power Source | Duration Source | Ramp |
|---|---|---|---|
| Pre-Infusion | `prof_preInfusePwr` | `prof_preInfuseTmr` | Exponential growth (0 → target) |
| Infusion | `prof_InfusePwr` | `prof_InfuseTmr` | Exponential growth (pre → peak) |
| Decline | `Prof_DeclinePwr` | `Prof_DeclineTmr` | Exponential decay (peak → decline) |
| Halt | 0 | — | Exponential decay → stop |

## Ramp Tables

The `s_profile_data_t` struct contains two lookup tables (14 entries each):
- **`a_expGrowth[]`** — `[0.39, 0.63, 0.75, 0.86, 0.90, 0.95, 0.97, 0.98, 0.99, 1.00, ...]`
- **`a_expDecay[]`** — `[0.60, 0.37, 0.25, 0.13, 0.09, 0.05, 0.03, 0.02, 0.01, 0.00, ...]`

Each ramp is divided into 10 sub-steps. The tick count per sub-step = `(duration_ms / 100ms) / 10`.

## Data Path

1. **Brew switch ON** → `fcn_service_ProfileMode()` enters `prof_Mode`
2. **Pre-infusion** — solenoid ON, compute delta power, apply growth table
3. **Infusion** — ramp from pre-infusion to peak power via growth table
4. **Decline** — ramp down from peak to decline via decay table
5. **Halt** — ramp to zero, solenoid OFF
6. Each stage: power updates pump SSR via `fcn_pumpSSR_pwrUpdate()`
7. Temperature PID runs concurrently every 500 ms with adaptive I-gain

## Diagram

See [brew-profile_diagram.mermaid](brew-profile_diagram.mermaid).
