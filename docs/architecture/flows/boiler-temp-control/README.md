# Flow: Boiler Temperature Control Loop

## Overview

End-to-end closed-loop temperature control: RTD sensor → temperature reading → PID controller → SSR heater output.

## Trigger

- Temperature read: `tf_GetBoilerTemp` flag every ~100 ms
- PID update: called within `fcn_service_ClassicMode()`/`fcn_service_ProfileMode()` every 500 ms (SVC_MONITOR_TICK)

## Data Path

1. **MAX31865 RTD Sensor** — continuously converts PT100 resistance to digital
2. **spi_Devices.c** — `spim_ReadRTDconverter()` (non-blocking state machine):
   - State 0: initiate SPI read of ADC register
   - State 1: on SPI complete, compute temperature via quadratic formula
   - Store result in private `rtdTemperature`
3. **main.c** — reads via `f_getBoilerTemperature()`, stores in `blEspressoProfile.temp_Boiler`
4. **BLEspressoServices** — `fcn_updateTemperatureController()`:
   - Feeds `temp_Boiler` as PV, `temp_Target` as SP, `milisTicks` as timestamp
   - Calls `fcn_update_PIDimc_typeA()` — computes P + I + D + anti-windup
   - Returns heater power (0–1000)
5. **solidStateRelay_Controller** — `fcn_boilerSSR_pwrUpdate(power)`:
   - Zero-cross mode: counts AC cycles on/off proportional to power
   - Drives boiler heater SSR GPIO

## Adaptive Integral Gain

| Phase | Trigger | Ki Multiplier |
|---|---|---|
| Steady state | Default | ×1.0 |
| Phase 1 (pump active) | Brew switch ON | ×6.5 (`Pid_Iboost_term`) |
| Phase 2 (recovery) | Brew switch OFF | ×2.0 |
| Return to steady | temp_Boiler within 1°C of target | ×1.0 |

## Diagram

See [boiler-temp-control_diagram.mermaid](boiler-temp-control_diagram.mermaid).
