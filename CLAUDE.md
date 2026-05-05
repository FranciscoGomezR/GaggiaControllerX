# GaggiaControllerX - Project Summary

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
PID-IMC controller regulates boiler temperature via SSR zero-crossing control. Adaptive I-gain: [End-user programmable] boost during brew, [End-user programmable] during recovery, [End-user programmable] at steady state. 
Hardware timer provides 1ms tick; PID updates every ~100ms.

### Brew Modes

**Classic:** 
Brew switch activates pump (100%) + solenoid. Steam switch changes setpoint to steam temperature. Simple on/off operation.

**Profile:** 
Three-stage pressure profile with configurable power and duration per stage:
1. Pre-infusion (low pressure soak)
2. Infusion (full pressure extraction)
3. Decline (pressure taper)

Exponential ramp tables provide smooth transitions between stages.

**Step Function:**
Diagnostic mode (activated by holding both switches at startup). Applies 100% heater power and logs temperature for PID tuning.

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

## Code Convention
Code Convention section does not apply to any function or variable declared inside **nRF5 SDK 17.1.0** and **S132 SoftDevice**

### Coding Guidelines
- A u or U suffix shall be applied to all integer constants that are represented in an unsigned type
- The controlling expression of a #if or #elif preprocessing directive shall evaluate to 0 or 1
- The value returned by a function having non-void return type shall be used	
- An inline function shall be declared with the static storage class	
- Arrays shall not be partially initialized
- An element of an object shall not be initialized more than once

### Naming Convention
- variables and functions shall use: snake_case
- structure element shall use: camelCase

### Global Variables
- Global variables should be prepended with a 'g_'.
**DO NOT IMPLEMENT YET** - Global variables should be avoided whenever possible.
```
    For example:
    Logger  g_log;
    Logger *g_ptr_log;
```
### Include Units in Names
If a variable represents time, weight, or some other unit then include the unit in the name so developers can more easily spot problems. 
```
    For example:
    uint32 timeout_msecs;
    uint32 my_temp_degC;
```
- For seconds:  secs
- For miliseconds: msecs
- For nanoseconds: nsecs
- For power in percentage: pwr
- For electrical power: watt
- For temperature in Celcius: degC
- For temperature in Farenheit: degF

### Function Names
Usually every function performs an action, so the name should make clear what it does: check_for_errors() instead of error_check(), dump_data_to_file() instead of data_file(). This will also make functions and data objects more distinguishable. By making function names verbs and following other naming conventions programs can be read more naturally.

- Function names shall NOT use the prefix `fcn_`.
- Private functions shall be declared `static`.

    Suffixes are sometimes useful:
    max - to mean the maximum value something can have.
    cnt - the current count of a running count variable.
    key - key value.
    For example: retry_max to mean the maximum number of retries, retry_cnt to mean the current retry count.

    Prefixes are sometimes useful:
    is - to ask a question about something. Whenever someone sees Is they will know it's a question.
    get - get a value.
    set - set a value.
    For example: is_hit_retry_limit.

### Structure Names
- Structs names shall be nouns.
- Use **typedef struct** names end with: _t
```
    For example:
    typedef struct 
    {}ble_espresso_user_data_t;
```
- Struct Naming: All struct names must start with a capitalized letter and must have a suffix: _s 
```
    For example:
    ble_espresso_user_data_t Ble_espresso_profile_t;
```

### Pointer Variables
- place the * close to the variable name not pointer type
- all pointers name shall use prefix: ptr_
```
    For example:
    char *ptr_name= NULL;
```

### Arrays Names
Arrays names shall have a name and a suffix: _arr 
```
    For example:
    const float EXP_GROWTH_arr[14];
```

### Boolean Naming Conventions
Use prefixes like: is_, has_, can_, should_ or flag_ to make boolean variables.
```
    For example:
    bool is_boostI_phase1;
    bool flag_active;
```

### Enum Names
Labels All Upper Case with '_' Word Separators
This is the standard rule for enum labels. No comma on the last element.
Use **enum** names end with: _t
```
    For example:
    enum  {
        PIN_OFF,
        PIN_ON
    }pinStateType_t;
```

### Constants variables
Constants should be all caps with '_' separators.

### A Line Should Not Exceed 79 Characters
Lines should not exceed 79 characters.

### Add Comments to Closing Braces
Adding a comment to closing braces can help when you are reading code because you don't have to find the begin brace to know what is going on.
```
    For example:
    while(1) {
    if (valid) {
    
    } /* if valid */
    else {
    } /* not valid */

    } /* end forever */
```
