# GaggiaControllerX - Technical Summary

## Overview

Embedded controller for a Gaggia Classic espresso machine running on an **nRF52832** (Cortex-M4) with the **nRF5 SDK 17.1.0** and **S132 SoftDevice** (BLE 5.0). Built with Segger Embedded Studio.

## Architecture

```
┌──────────────────────────────────────────────────┐
│                 BLE (SoftDevice S132)             │
│          "BLEspresso" Custom GATT Services       │
│     Brew profile params (0x1400) │ PID (0x1500)  │
└────────────────────┬─────────────────────────────┘
                     │
┌────────────────────▼─────────────────────────────┐
│              Application Layer                    │
│                                                   │
│  BLEspressoServices ─── Main state machine        │
│   ├─ Classic Mode  (on/off brew & steam)          │
│   ├─ Profile Mode  (pressure profiling)           │
│   └─ Step Fcn Mode (PID tuning/diagnostics)       │
│                                                   │
│  TempController ─── PID-IMC boiler regulation     │
│  PumpController ─── Multi-stage ramp profiles     │
│  StorageController ─ NVM parameter persistence    │
└────────┬──────────────┬──────────────┬───────────┘
         │              │              │
┌────────▼──────────────▼──────────────▼───────────┐
│              Peripheral Drivers                   │
│                                                   │
│  SPI:  MAX31865 RTD (boiler temp) + NVM flash     │
│  SSR:  Boiler heater │ Pump motor │ Solenoid      │
│  GPIO: Brew/Steam AC input sensing (debounced)    │
│  PWM:  12V high-side switch output                │
│  I2C:  TMP006 IR sensor (unused)                  │
└──────────────────────────────────────────────────┘
```

## Scheduler

A 20ms software timer drives all tasks via flags:

| Task              | Period | Description                    |
|-------------------|--------|--------------------------------|
| AC Input Sensing  | 60ms   | Debounce brew/steam switches   |
| Boiler Temp Read  | 100ms  | SPI read from MAX31865 RTD     |
| Espresso Service  | 100ms  | Run active mode state machine  |
| Step Function     | 100ms  | Tuning mode service            |

## Functional Description

### Temperature Control
PID-IMC controller regulates boiler temperature via SSR zero-crossing control. Adaptive I-gain: 6.5x boost during brew, 2x during recovery, 1x at steady state. Hardware timer provides 1ms tick; PID updates every ~100ms.

### Brew Modes

**Classic:** Brew switch activates pump (100%) + solenoid. Steam switch changes setpoint to steam temperature. Simple on/off operation.

**Profile:** Three-stage pressure profile with configurable power and duration per stage:
1. Pre-infusion (low pressure soak)
2. Infusion (full pressure extraction)
3. Decline (pressure taper)

Exponential ramp tables provide smooth transitions between stages.

**Step Function:** Diagnostic mode (activated by holding both switches at startup). Applies 100% heater power and logs temperature for PID tuning.

### BLE Interface
Two custom GATT services allow a mobile app to:
- Monitor boiler temperature (notifications)
- Set brew/steam temperature targets
- Configure pressure profile parameters (power & time per stage)
- Tune PID gains (P, I, I-max, D, D-LPF, Gain)

### Storage
User-configured parameters (brew profile + PID gains) persist to internal flash via `nrf_fstorage`.

## Key Files

| Path | Role |
|------|------|
| `main/main.c` | Init sequence + scheduler loop |
| `components/Application/BLEspressoServices.c` | Mode state machines |
| `components/Application/tempController.c` | PID temperature control |
| `components/Application/PumpController.c` | Pump ramp profiles |
| `components/Peripherals/solidStateRelay_Controller.c` | SSR phase/zero-cross control |
| `components/Peripherals/spi_Devices.c` | RTD + NVM SPI drivers |
| `components/Peripherals/ac_inputs_drv.c` | Brew/Steam switch sensing |
| `components/BLE_Services/ble_cus.c` | Custom BLE GATT services |
| `components/Utilities/x205_PID_Block.c` | PID algorithm |

All paths relative to `ble_espresso_app/`.
