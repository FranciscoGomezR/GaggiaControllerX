# Flow: Brew Cycle — Classic Mode

## Overview

Documents the full data flow when a user pulls a shot in Classic Mode: from physical switch activation through pump/solenoid/heater control.

## Trigger

User flips the Brew or Steam AC switch on the Gaggia Classic.

## States

| State | Condition | Active Outputs |
|---|---|---|
| `cl_idle` | No switch | Heater only (PID) |
| `cl_Mode_1` | Brew ON | Heater + Pump 100% + Solenoid ON, Ki ×6.5 |
| `cl_Mode_2` | Steam ON | Heater (steam setpoint), no pump |
| `cl_Mode_3` | Both ON | Heater + Pump (steam wand purge) |

## Data Path

1. **AC Switch** — 60 Hz AC toggles sensed by `ac_inputs_drv` via GPIOTE interrupts
2. **Debounce** — `fcn_SenseACinputs_Sixty_ms()` called every 60 ms; ISR counter evaluated against threshold
3. **main.c** — scheduler reads switch status, passes to `fcn_service_ClassicMode(swBrew, swSteam)` every 100 ms
4. **BLEspressoServices** — state machine:
   - On brew assert: boost I-gain, activate solenoid, start pump at 100%
   - On brew release: stop pump, reduce I-gain to ×2.0, deactivate solenoid
   - Every 500 ms: read boiler temp, run PID, update heater SSR
5. **Outputs** — three SSR channels:
   - `fcn_boilerSSR_pwrUpdate()` — heater power
   - `fcn_pumpSSR_pwrUpdate()` — pump power
   - `fcn_SolenoidSSR_On/Off()` — solenoid valve

## Diagram

See [brew-classic_diagram.mermaid](brew-classic_diagram.mermaid).
