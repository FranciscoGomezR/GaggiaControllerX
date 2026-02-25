# Unit Testing Plan — GaggiaControllerX

## Test Strategy

Six sequential phases, each building on the previous. Each phase has one environment, one scope, and one set of pass criteria. A phase must pass before starting the next.

| Phase | Environment | Scope | What It Proves |
|---|---|---|---|
| 1 | Host PC (gcc + Unity) | Pure algorithms | Math is correct: PID, filters, clamping, conversions |
| 2 | Host PC (gcc + Unity + FFF) | Application logic with mocked HW | State machines and driver orchestration are correct |
| 3 | Host PC (Ceedling + CMock) | Same as Phase 2 | Call ordering is strict, no unintended side effects, coverage measured |
| 4 | Bare nRF52-DK (no external HW) | MCU peripherals + BLE | Real timers, GPIO, SoftDevice work on silicon |
| 5 | DK + breakout peripherals | SPI devices, simulated zero-cross | Real sensors respond, data integrity holds, SSR timing correct |
| 6 | Production board + Gaggia machine | Full system | Thermal control converges, brew cycles work, system is safe |

**Verification approach:** Each test defines a *stimulus*, an *expected response*, and a *pass criterion*. Tests pass only when the measured response satisfies the criterion.

---

## Frameworks and Tools

| Tool | Type | Used In | Purpose |
|------|------|---------|---------|
| **Unity** | C test framework | Phases 1–2 | Assertion macros (`TEST_ASSERT_FLOAT_WITHIN`, etc.), test runner generation |
| **FFF** | Header-only C library | Phase 2 | `FAKE_VOID_FUNC` / `FAKE_VALUE_FUNC` macros to replace HW-dependent functions with controllable fakes |
| **Ceedling + CMock** | Build system + mock generator | Phase 3 | Auto-generates mock implementations from header files; adds call-order verification and gcov coverage |
| **gcc** | Compiler | Phases 1–3 | Compiles application code and tests for x86 host execution |
| **arm-none-eabi-gcc** | Cross-compiler | Phases 4–6 | Compiles test firmware for nRF52832 (Cortex-M4) |
| **Segger Embedded Studio** | IDE / build system | Phases 4–6 | Builds on-target test firmware using existing project configuration |
| **nrfjprog** | Nordic CLI tool | Phases 4–6 | Flash hex, reset board, verify programming via J-Link |
| **SEGGER RTT** | Debug channel library | Phases 4–6 | Redirects Unity output (`UNITY_OUTPUT_CHAR`) from target to host over SWD |
| **JLinkRTTLogger** | Host-side RTT reader | Phases 4–6 | Captures RTT output to log file for automated pass/fail parsing |
| **bleak** | Python BLE library | Phases 4–6 | Scripted GATT read/write/notify for BLE service testing |
| **GitHub Actions** | CI runner (self-hosted) | All phases | Automates test execution; self-hosted runner required for Phases 4–6 (J-Link access) |

---

## Known Issues and Required Tests

Issues identified during code analysis. Each must have a dedicated test proving the fix works before moving to on-target phases.

### Critical — Must Fix Before AC Power Testing

| # | Issue | File (relative to `ble_espresso_app/`) | Lines | Test |
|---|-------|----------------------------------------|-------|------|
| C1 | **No watchdog timer** — firmware hang leaves SSR outputs HIGH → thermal runaway | `main/main.c` | — | Phase 4: Verify `NRF_WDT` starts during init. Inject infinite loop → confirm board resets within 1 s and SSR pins read LOW after reset. |
| C2 | **P0.25 SWD/solenoid conflict** — SWDCLK toggles solenoid relay during debug | `pca10040/s132/config/app_config.h:56` | 56 | Phase 5: With debugger attached, verify solenoid GPIO stays LOW during flash/reset. If not, remap pin or add isolation resistor. |
| C3 | **GPIO state undefined during boot** — SSR outputs may float HIGH for ~2 s before `fcn_initSSRController_BLEspresso()` runs | `main/main.c` | 178–323 | Phase 4: Scope P0.19, P0.20, P0.25 from POR to scheduler start. Pass: all pins LOW within 1 ms of reset. Fix: add `nrf_gpio_cfg_output` + `pin_clear` at top of `main()`. |

### High — Safety-Related

| # | Issue | File | Lines | Test |
|---|-------|------|-------|------|
| H1 | **Division by zero in pump slope calc** — BLE write of `Time_Rampup_ms = 0` causes hard fault | `components/Application/PumpController.c` | 320–339 | Phase 2 (FFF): Call `fcn_LoadNewPumpParameters()` with zero time fields → verify no crash, function returns error code. |
| H2 | **No BLE input validation** — any float accepted for temperature, PID gains, profile timers | `components/BLE/bluetooth_drv.c` | 645–777 | Phase 2: Write out-of-range values via fake BLE event (`temp_Target = -50`, `Pid_P_term = -1`, `prof_InfuseTmr = 0`) → verify values are clamped or rejected. |
| H3 | **No temperature sensor failure detection** — `f_getBoilerTemperature()` returns raw float, no error flag | `components/Peripherals/spi_Devices.c`, `components/Application/tempController.c` | 245–253 | Phase 2: Set fake `f_getBoilerTemperature` to return `0.0` (open sensor) or `400.0` (shorted) → verify PID output goes to 0 % and fault flag is set. |
| H4 | **No overheat protection in step-function mode** — 100 % heater power with no temperature limit or timeout | `components/Application/BLEspressoServices.c` | 1003 | Phase 2: Run step-function state machine with fake temperature rising past 150 °C → verify heater command drops to 0 %. |
| H5 | **No maximum brew time limit** — profile mode runs indefinitely if brew switch stays asserted | `components/Application/BLEspressoServices.c` | 478–881 | Phase 2: Hold fake `swBrew` asserted for 120 s of simulated time → verify state machine auto-stops pump and solenoid. |

### Medium — Functional Correctness

| # | Issue | File | Lines | Test |
|---|-------|------|-------|------|
| M1 | **No integral reset on setpoint change** — brew → steam jump causes overshoot | `components/Application/tempController.c` | 189–200 | Phase 2: Call `fcn_loaddSetPoint_ParamToCtrl_Temp()` with `SETPOINT_STEAM` after running PID at brew setpoint → verify integral history is zeroed. |
| M2 | **Timer period 1009 µs instead of 1000 µs** — PID timing drifts ~0.9 % | `components/Application/tempController.c` | 17 | Phase 4: Measure TIMER interrupt period with logic analyser or DWT cycle counter. Pass: period within ±0.1 % of intended value. |
| M3 | **No NVM data validation** — corrupted flash loads unsafe parameters | `main/main.c`, `components/Application/StorageController.c` | 235–285 | Phase 2: Feed `stgCtrl_ReadUserData()` with out-of-range values (e.g. `temp_Target = 999`) → verify system falls back to safe defaults. Phase 5: Write known-bad pattern to flash, reboot, verify safe defaults loaded. |
| M4 | **No atomic BLE parameter update** — disconnect mid-write leaves mixed old/new profile | `components/BLE/bluetooth_drv.c` | 645–777 | Phase 2: Simulate partial profile write (update time fields, skip power fields) → verify system uses either fully old or fully new profile, not a mix. |
| M5 | **I-gain boost reapplied on rapid brew cycling** — no minimum time between brew activations | `components/Application/BLEspressoServices.c` | 293 | Phase 2: Toggle fake `swBrew` ON/OFF 5 times in 2 s → verify I-gain multiplier is not applied more than once per cool-down period. |

### Low — Dev Kit Only

| # | Issue | File | Lines | Test |
|---|-------|------|-------|------|
| L1 | **DK LED conflict** — P0.19 (LED3) / P0.20 (LED4) overlap with boiler and pump SSR pins | `pca10040/s132/config/app_config.h` | 54–55 | Phase 4: Disable BSP LED init, toggle SSR pins → verify correct GPIO state with multimeter. |
| L2 | **DK button conflict** — P0.13 (Button 1) / P0.14 (Button 2) overlap with SPI_SCK and SPI_MOSI | `pca10040/s132/config/app_config.h` | 77–79 | Phase 5: Disable BSP button init, run SPI read → verify MAX31865 returns valid RTD value. |

---

## Phase 1: Pure Algorithm Tests (Host PC, Unity)

**Goal:** Verify that PID, filter, and math utility functions produce correct outputs for known inputs. No hardware, no mocks, no stubs — these modules use only standard C.

### Project Setup

```
tests/
├── Makefile
├── unity/                    # Unity framework (git submodule or copy)
│   ├── unity.c
│   ├── unity.h
│   └── unity_internals.h
├── test_pid_block.c
├── test_digital_filters.c
└── test_numbers.c
```

### Makefile

```makefile
CC = gcc
CFLAGS = -Wall -Wextra
CFLAGS += -I tests/unity
CFLAGS += -I ble_espresso_app/components/Utilities
CFLAGS += -I ble_espresso_app/components/Utilities/include
LDFLAGS = -lm
```

**Build example:** `gcc test_pid_block.c x205_PID_Block.c x201_DigitalFiltersAlgorithm.c x04_Numbers.c unity.c -lm -o test_pid`

### Test Cases

| Test File | Source Under Test | What to Test |
|---|---|---|
| `test_numbers.c` | `x04_Numbers.c` | `fcn_Constrain_WithinFloats` limits/saturation, `fcn_ChrArrayToFloat` round-trips, `fcn_FloatToChrArray` formatting |
| `test_digital_filters.c` | `x201_DigitalFiltersAlgorithm.c` | Step response of `lpf_rc_update`, corner frequency accuracy, `pfcn_RCFilterAlgorithm` steady-state convergence |
| `test_pid_block.c` | `x205_PID_Block.c` | P-only response, I accumulation & anti-windup, D kick rejection, `PIDimc_typeA` vs `typeB` output, output saturation, integral reset attenuator |

### Verification Criteria

| Function | Stimulus | Pass Criterion | Why It Matters |
|---|---|---|---|
| `fcn_Constrain_WithinFloats` | Values below, within, above limits | Clamped value correct; flag = -1/0/+1 | Prevents PID output from commanding unsafe power levels |
| `fcn_ChrArrayToFloat` / `fcn_FloatToChrArray` | Known floats (0.0, -1.5, 99.9) | Round-trip error < 0.01 | BLE characteristic data must survive serialization |
| `lpf_rc_update` | Unit step, known Fc and sample rate | Output reaches 63.2% within 1/Fc seconds (±5%) | D-term filter in PID must match design cutoff frequency |
| `pfcn_RCFilterAlgorithm` | Constant DC over 100 samples | Settles to input ±0.1% | Filter must converge, not drift or oscillate |
| `fcn_update_PID_Block` (P-only) | Error=10.0, Kp=2.0, Ki=Kd=0 | Output = 20.0 exactly | Proportional gain directly sets heater response magnitude |
| `fcn_update_PID_Block` (I) | Constant error over N steps | Output = Ki × error × dt × N (±1%) | Integral term eliminates steady-state temperature offset |
| `fcn_update_PID_Block` (I anti-windup) | Error sustained beyond IntegralLimit | HistoryError saturates; output ≤ OutputLimit | Without windup protection, heater overshoots massively after setpoint change |
| `fcn_update_PIDimc_typeA` | SP=100, PV=80 | Output > 0 (heat). SP=80, PV=100 → Output ≤ 0 | Sign error = heater runs when it should be off |
| `fcn_update_PIDimc_typeA` | Very large error | Output clamped to 1000.0 | Unclamped output could damage SSR driver |
| `fcn_PID_Block_ResetI` | Attenuator=0.5 after accumulation | HistoryError reduced by 50% | Used during brew→idle transition to prevent integral kick |

---

## Phase 2: Application Logic Tests (Host PC, Unity + FFF)

**Goal:** Verify that tempController, PumpController, and BLEspressoServices call the right driver functions, in the right order, with the right values. Hardware drivers are replaced with FFF fakes. Stub headers allow compilation without the nRF SDK.

### Additional Setup (added to Phase 1 structure)

```
tests/
├── ...                       # Phase 1 files
├── fff/                      # FFF framework (single header)
│   └── fff.h
├── stubs/                    # nRF SDK stub headers
│   ├── nrf.h
│   ├── nrf_drv_timer.h
│   ├── nrf_drv_gpiote.h
│   ├── nrf_drv_spi.h
│   ├── nrf_gpio.h
│   ├── nrf_log.h
│   ├── nrf_log_ctrl.h
│   ├── nrf_log_default_backends.h
│   ├── app_error.h
│   ├── boards.h
│   └── app_config.h
├── test_temp_controller.c
├── test_pump_controller.c
└── test_blespresso_services.c
```

### Stub Headers

Application modules `#include` nRF SDK headers that don't exist on x86. Stubs are minimal fake headers that provide only the types and macros needed to compile — no real implementation.

| Stub Header | What It Provides | Why Needed |
|---|---|---|
| `nrf.h` | Empty | Gates CMSIS includes; prevents missing-header error |
| `nrf_drv_timer.h` | `nrf_drv_timer_t`, `nrf_timer_event_t`, `NRF_DRV_TIMER_INSTANCE()` | `tempController.c` declares timer instances |
| `nrf_drv_gpiote.h` | `nrf_drv_gpiote_pin_t` (uint32_t), `nrf_gpiote_polarity_t` | `solidStateRelay_Controller.h` and `ac_inputs_drv.h` use these types in function signatures |
| `nrf_drv_spi.h` | `nrf_drv_spi_t`, `nrf_drv_spi_config_t` | `spi_Devices.h` declares SPI driver instance |
| `nrf_gpio.h` | Empty or pin config macros | Included transitively by several headers |
| `app_error.h` | `#define APP_ERROR_CHECK(x) ((void)(x))`, `#define NRF_SUCCESS 0` | Silences error-check macro; does nothing on host |
| `nrf_log.h` | `#define NRF_LOG_INFO(...)` | Silences log calls; does nothing on host |
| `boards.h` | Pin defines | Satisfies `#include` in peripheral headers |
| `app_config.h` | App config defines | Satisfies `#include` in peripheral headers |

### Makefile (extended)

```makefile
CFLAGS += -DTEST -DHOST -DSUPPRESS_INLINE_IMPLEMENTATION
CFLAGS += -I tests/stubs -I tests/fff
CFLAGS += -I ble_espresso_app/components/Application
CFLAGS += -I ble_espresso_app/components/Peripherals/include
```

Stubs are found via `-I tests/stubs` before any SDK path, so the compiler uses the fake headers.

### FFF Fake Declarations

```c
// solidStateRelay_Controller fakes
FAKE_VOID_FUNC(fcn_boilerSSR_pwrUpdate, uint16_t);
FAKE_VOID_FUNC(fcn_pumpSSR_pwrUpdate, uint16_t);
FAKE_VOID_FUNC(fcn_SolenoidSSR_On);
FAKE_VOID_FUNC(fcn_SolenoidSSR_Off);
FAKE_VALUE_FUNC(ssr_status_t, get_SolenoidSSR_State);

// spi_Devices fakes
FAKE_VALUE_FUNC(float, f_getBoilerTemperature);
FAKE_VOID_FUNC(spim_ReadRTDconverter);
FAKE_VALUE_FUNC(bool, spim_operation_done);

// ac_inputs_drv fakes
FAKE_VALUE_FUNC(acInput_status_t, fcn_GetInputStatus_Brew);
FAKE_VALUE_FUNC(acInput_status_t, fcn_GetInputStatus_Steam);

// nrf_drv_timer fakes
FAKE_VALUE_FUNC(uint32_t, nrf_drv_timer_init, void*, void*, void*);
FAKE_VOID_FUNC(nrf_drv_timer_enable, void*);
FAKE_VOID_FUNC(nrf_drv_timer_disable, void*);

// bluetooth_drv fakes
FAKE_VOID_FUNC(ble_update_boilerWaterTemp, float);
```

### Test Cases

| Test File | Source Under Test | Key Tests |
|---|---|---|
| `test_temp_controller.c` | `tempController.c` | PID gain loading, I-boost switching (1x→6.5x→2x), setpoint switching (brew↔steam), output power range [0–1000] |
| `test_pump_controller.c` | `PumpController.c` | State transitions: idle→ramp→keep→decline→stop, ramp slope calculation, `fcn_StartBrew`/`fcn_CancelBrew` resets |
| `test_blespresso_services.c` | `BLEspressoServices.c` | Classic mode: brew on/off. Profile mode: stage transitions. Step function: state sequence |

**Strategy for `tempController.c`:** `milisTicks` is `static`. Either add `#ifdef TEST` accessor or compile with `-DmilisTicks=test_milisTicks`.

**Strategy for `BLEspressoServices.c`:** `blEspressoProfile` is `extern volatile`. Provide it in the test file and populate before each test.

### Verification Criteria

| Test | Stimulus | Pass Criterion | Why It Matters |
|---|---|---|---|
| Gain loading | `blEspressoProfile.Pid_P_term = 9.5`, call load | Internal Kp == 9.5; returns `TEMPCTRL_LOAD_OK` | Wrong gain loaded → temperature oscillation or no response |
| I-boost | Call `fcn_loadIboost_ParamToCtrl_Temp()` | SSR power computed with Ki×6.5 | Without boost, brew extraction cools boiler faster than heater compensates |
| Setpoint switch | Call with `SETPOINT_STEAM` | PID SetPoint == `sp_StemTemp` | Wrong setpoint → steam too cold or boiler overheats |
| Output range | PV=20°C, SP=93°C (large error) | `fcn_boilerSSR_pwrUpdate` arg ∈ [0, 1000] | Out-of-range value could damage SSR or heater |
| Pump state transitions | `fcn_StartBrew()` then N × `fcn_PumpStateDriver()` | States progress idle→Ramp1→Keep1→...→Stop in order | Wrong sequence → pressure spike or pump stall |
| Pump ramp slope | preInfusePwr=30, InfusePwr=100 | Power arg increases linearly 300→1000 (±5%) | Non-linear ramp → espresso channeling or uneven extraction |
| Pump cancel | `fcn_CancelBrew()` mid-ramp | State=idle; power set to 0 | Stuck pump after button release → machine damage |
| Classic brew on | swBrew = `AC_SWITCH_ASSERTED` | pump called with 1000; solenoid ON called once | Pump without solenoid = dead-headed pump (damage) |
| Classic brew off | swBrew = `AC_SWITCH_DEASSERTED` | pump called with 0; solenoid OFF called once | Stuck outputs after release = water/heat runaway |
| Profile stage timing | `prof_preInfuseTmr = 5.0s` | Stage 1→2 at tick count matching 5s (±1 tick) | Wrong timing → pre-infusion too short or too long |
| Step function heat | Enter Mode_2b | Boiler SSR called with 1000 | 100% power needed for PID characterization step response |

### Build & Run

```bash
cd tests
make all      # builds all test binaries
make run      # executes all, prints Unity results
```

---

## Phase 3: Ceedling + CMock (Host PC, auto-generated mocks)

**Goal:** Replace manual FFF fakes with auto-generated CMock mocks. Adds strict call ordering, unexpected call detection, and code coverage. Runs the same test logic as Phase 2 with stricter verification.

### Prerequisites

```bash
gem install ceedling
ceedling new tests_ceedling
```

### Configure `project.yml`

```yaml
:project:
  :build_root: tests_ceedling/build
  :test_file_prefix: test_

:defines:
  :test:
    - TEST
    - HOST
    - SUPPRESS_INLINE_IMPLEMENTATION
    - NRF_LOG_ENABLED=0

:paths:
  :source:
    - ble_espresso_app/components/Utilities/**
    - ble_espresso_app/components/Application/**
  :include:
    - ble_espresso_app/components/Utilities/include
    - ble_espresso_app/components/Peripherals/include
    - ble_espresso_app/components/Application
    - tests_ceedling/stubs
  :test:
    - tests_ceedling/test/**

:cmock:
  :mock_prefix: mock_
  :enforce_strict_ordering: true
  :treat_externs: :include
  :plugins:
    - :ignore
    - :callback
    - :return_thru_ptr
```

### Stub Headers

Same stubs as Phase 2, placed in `tests_ceedling/stubs/`.

### Auto-Generated Mocks

CMock reads headers and generates `mock_*.h/.c` with `_Expect()`, `_ExpectAndReturn()`, and `_StubWithCallback()` for every function.

| Mock Target | Used By |
|---|---|
| `mock_solidStateRelay_Controller.h` | tempController, PumpController, BLEspressoServices |
| `mock_spi_Devices.h` | tempController, StorageController |
| `mock_ac_inputs_drv.h` | BLEspressoServices |
| `mock_bluetooth_drv.h` | BLEspressoServices |
| `mock_PumpController.h` | BLEspressoServices |
| `mock_tempController.h` | BLEspressoServices |
| `mock_StorageController.h` | BLEspressoServices |

### Example Test

```c
#include "unity.h"
#include "PumpController.h"
#include "mock_solidStateRelay_Controller.h"

void setUp(void) { fcn_initPumpController(); }
void tearDown(void) {}

void test_StartBrew_activates_pump(void) {
    bleSpressoUserdata_struct profile = { .prof_preInfusePwr = 50.0 };
    fcn_LoadNewPumpParameters(&profile);
    fcn_pumpSSR_pwrUpdate_Expect(500);
    fcn_StartBrew();
    fcn_PumpStateDriver();
}
```

### What Phase 3 Adds Over Phase 2

| Aspect | Phase 2 (FFF) | Phase 3 (CMock) |
|---|---|---|
| Mock creation | Manual `FAKE_*` macros | Auto-generated from headers |
| Call verification | Check `fake.call_count` | `_Expect()` with strict ordering |
| Unexpected calls | Silent (ignored) | Test fails immediately |
| Build system | Manual Makefile | Ceedling manages everything |
| Coverage | Manual gcov | `ceedling gcov:all` built-in |

### Verification Criteria

Same test cases and pass criteria as Phase 2, plus:

- **Call ordering:** CMock fails if `fcn_SolenoidSSR_On` is called before `fcn_pumpSSR_pwrUpdate` (or vice versa, depending on expected sequence).
- **No side effects:** Any driver call not explicitly expected causes immediate test failure.
- **Coverage target:** >80% line coverage on application modules, 100% on utility modules.

### Build & Run

```bash
ceedling test:all
ceedling test:test_PumpController
ceedling gcov:all
```

---

## Phase 4: Bare DK Tests (nRF52-DK, no external HW)

**Goal:** Verify that MCU peripherals initialize correctly on real silicon, timers fire at designed rates, and BLE stack advertises and serves GATT operations. Only USB connected — no sensors, no SSR loads.

### Board Compatibility

| Resource | App Usage | DK Status |
|---|---|---|
| SPI1 (P0.11-14) | MAX31865 + W25Q64 | Available on header P3 |
| TIMER1-3 | SSR + PID tick | Available (TIMER0 reserved by SoftDevice) |
| GPIOTE | Zero-cross, brew/steam ISRs | Enabled |
| P0.19-20 | SSR boiler/pump | **Conflict: DK LED3-4** — disable BSP LEDs |
| P0.13-14 | SPI SCK/MOSI | **Conflict: DK Button1-2** — disable BSP buttons |
| P0.25-28 | Solenoid, zero-cross, switches | Free on header |
| SoftDevice S132 | BLE 5.0 | Pre-supported |

**Fix conflicts:** Set `BSP_DEFINES_ONLY` or remap LED/button pins in `sdk_config.h`.

### Automation Toolchain (shared with Phase 5-6)

```
┌────────────┐    USB/JLink     ┌──────────────┐
│ Linux Host │──────────────────│ nRF52-DK     │
│            │                  │ (PCA10040)   │
│ nrfjprog   │ flash/reset      │              │
│ JLinkRTT   │ capture RTT      │ Unity tests  │
│ bleak      │ BLE scan/R/W     │ run on-target│
│ pytest     │ assert results   │              │
└────────────┘                  └──────────────┘
```

**Flash + RTT capture script:**

```bash
#!/bin/bash
set -euo pipefail
HEX="$1"
RTT_LOG=$(mktemp)
TIMEOUT=30

nrfjprog -f nrf52 --program "$HEX" --sectorerase --verify --reset
JLinkRTTLogger -Device NRF52832_XXAA -If SWD -Speed 4000 -RTTChannel 0 "$RTT_LOG" &
PID=$!

ELAPSED=0
while [ $ELAPSED -lt $TIMEOUT ]; do
    grep -q "Tests.*Failures.*Ignored" "$RTT_LOG" 2>/dev/null && break
    sleep 1; ELAPSED=$((ELAPSED + 1))
done
kill $PID 2>/dev/null; wait $PID 2>/dev/null

cat "$RTT_LOG"
FAILURES=$(grep -oP '\d+ Failures' "$RTT_LOG" | grep -oP '\d+')
[ "${FAILURES:-1}" -eq 0 ] && echo "PASS" && exit 0
echo "FAIL"; exit 1
```

### On-Target Test Firmware

```c
// test_main_bare_dk.c
#include "unity.h"
#include "SEGGER_RTT.h"

#define UNITY_OUTPUT_CHAR(c) SEGGER_RTT_PutChar(0, c)

extern void test_timer3_1ms_tick(void);
extern void test_gpiote_ssr_output_toggles(void);
extern void test_scheduler_20ms_tick(void);
extern void test_pid_output_within_limits(void);
extern void test_state_machine_init_to_idle(void);
extern void test_gpio_loopback(void);

int main(void) {
    // Init clocks, GPIO, timers — NO SoftDevice
    UNITY_BEGIN();
    RUN_TEST(test_timer3_1ms_tick);
    RUN_TEST(test_gpiote_ssr_output_toggles);
    RUN_TEST(test_scheduler_20ms_tick);
    RUN_TEST(test_pid_output_within_limits);
    RUN_TEST(test_state_machine_init_to_idle);
    RUN_TEST(test_gpio_loopback);
    UNITY_END();
    SEGGER_RTT_WriteString(0, "\nTESTS COMPLETE\n");
    while (1) { __WFE(); }
}
```

### BLE Tests (separate firmware with SoftDevice)

Validated from host via Python `bleak`:

```python
import asyncio, struct
from bleak import BleakClient, BleakScanner

DEVICE_NAME = "BLEspresso"
CHAR_BOILER_TEMP = "00001402-..."
CHAR_TARGET_TEMP = "00001403-..."

async def test_ble():
    dev = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10)
    assert dev, "BLEspresso not found"

    async with BleakClient(dev) as c:
        assert c.is_connected

        svc_uuids = [s.uuid for s in c.services]
        assert any("1400" in u for u in svc_uuids), "BLEspresso service missing"
        assert any("1500" in u for u in svc_uuids), "PID service missing"

        val = await c.read_gatt_char(CHAR_BOILER_TEMP)
        assert len(val) == 4

        await c.write_gatt_char(CHAR_TARGET_TEMP, struct.pack('<f', 95.0))
        readback = struct.unpack('<f', await c.read_gatt_char(CHAR_TARGET_TEMP))[0]
        assert abs(readback - 95.0) < 0.1

        received = asyncio.Event()
        await c.start_notify(CHAR_BOILER_TEMP, lambda _, d: received.set())
        await asyncio.wait_for(received.wait(), timeout=5)

asyncio.run(test_ble())
```

### Verification Criteria

| Test | Stimulus | Pass Criterion | Why It Matters |
|---|---|---|---|
| Timer 1ms tick | Read `milisTicks` after 1000ms delay | Count within ±2% of 1000 | PID loop timing drives control stability |
| Scheduler 20ms | Count flag sets over 1s | 50 ±1 flags set | Wrong rate → sensor reads or control updates at wrong interval |
| GPIO loopback | Write P0.19 high/low, read on P0.02 via jumper wire | Read matches write every time | GPIO misconfiguration → SSR never fires or stays stuck |
| State machine init | Flash and capture RTT | All "READY" messages, no "FAILED" | Init regression → system dead on startup |
| BLE advertising | Flash app, scan from host | `bleak` finds "BLEspresso" within 10s | SoftDevice broken → no remote control |
| GATT R/W | Write float32, read back | Error < 0.1 | BLE data path broken → can't configure machine |
| Notifications | Subscribe to boiler temp | Notification received within 5s | Temperature monitoring doesn't work |

**Why on-target:** Catches clock tree misconfiguration, interrupt priority conflicts between SoftDevice and app timers, GPIO electrical issues. Host tests assume the MCU works; Phase 4 verifies that assumption.

---

## Phase 5: Bench Tests (DK + Breakout Peripherals)

**Goal:** Verify SPI device communication, sensor accuracy, NVM data integrity, zero-cross ISR timing, and SSR output duty cycles using real peripheral ICs — but without AC power. Safe to run unattended.

### Required Hardware (~$30-35)

| Component | Est. Cost | Connects To | Tests Enabled |
|---|---|---|---|
| MAX31865 RTD breakout + PT100 | ~$15 | SPI1: P0.11-14 | SPI comms, temperature reading |
| W25Q64 flash breakout | ~$5 | SPI1 shared bus, CS: P0.15, WP: P0.17, HD: P0.18 | NVM storage R/W |
| 555 timer or function gen | ~$5-10 | P0.26 (zero-cross input) | Zero-crossing ISR timing |
| Push buttons (2x) | ~$1 | P0.27 (brew), P0.28 (steam) | Input debounce |
| LEDs + resistors (3x) | ~$1 | P0.19 (boiler), P0.20 (pump), P0.25 (solenoid) | Output verification |

### On-Target Test Firmware

```c
// test_main_bench.c
#include "unity.h"
#include "SEGGER_RTT.h"

#define UNITY_OUTPUT_CHAR(c) SEGGER_RTT_PutChar(0, c)

extern void test_spi_rtd_init(void);
extern void test_spi_rtd_read_temperature(void);
extern void test_spi_nvm_read_device_id(void);
extern void test_spi_nvm_write_read_page(void);
extern void test_zero_cross_isr_fires(void);
extern void test_ssr_boiler_duty_cycle(void);
extern void test_ssr_pump_ramp_sequence(void);
extern void test_brew_switch_debounce(void);
extern void test_pid_direction(void);
extern void test_i_boost_activation(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_spi_rtd_init);
    RUN_TEST(test_spi_rtd_read_temperature);
    RUN_TEST(test_spi_nvm_read_device_id);
    RUN_TEST(test_spi_nvm_write_read_page);
    RUN_TEST(test_zero_cross_isr_fires);
    RUN_TEST(test_ssr_boiler_duty_cycle);
    RUN_TEST(test_ssr_pump_ramp_sequence);
    RUN_TEST(test_brew_switch_debounce);
    RUN_TEST(test_pid_direction);
    RUN_TEST(test_i_boost_activation);
    UNITY_END();
    SEGGER_RTT_WriteString(0, "\nTESTS COMPLETE\n");
    while (1) { __WFE(); }
}
```

### Verification Criteria

| Test | Stimulus | Pass Criterion | Why It Matters |
|---|---|---|---|
| RTD init | Call `spim_initRTDconverter()` | Returns `TMP_INIT_OK` | SPI protocol or wiring error → no temperature data |
| Temperature read | Call `f_getBoilerTemperature()` at room temp | Returns 15–30°C (±5°C of ambient) | Wrong conversion coefficients → PID controls to wrong temperature |
| NVM init | Call `spim_initNVmemory()` | Returns `NVM_INIT_OK`, device ID = 0xEF/0x4017 | Wrong CS pin or SPI mode → storage not available |
| NVM write/read | Write 256-byte pattern, read back | 0 byte mismatches | Flash corruption → user settings lost on reboot |
| User data round-trip | `stgCtrl_Store...()` → `stgCtrl_Read...()` | All float fields match ±0.001 | Serialization bug → PID gains silently change |
| Zero-cross ISR | Count triggers over 1s (555 timer input) | 100±5 (50Hz) or 120±5 (60Hz) | Wrong edge → SSR phase control fails completely |
| SSR duty cycle | Set power=500, count LED on-time | ~50% duty (±10%) | Timer mismatch → heater gets wrong power |
| Pump ramp | Start brew, capture power sequence | Power values match expected slope ±5% | Wrong ramp → pressure spike or stall |
| Input debounce | Press button, check state timing | State changes after 60ms, not before | False triggers → phantom brew starts |
| PID direction | Target > PV | Output > 0 (heat). Target < PV → output = 0 | Sign error → heater on when should be off |
| I-boost | Call brew start | RTT shows Ki×6.5 during brew, Ki×2.0 after | Gain not applied → temperature drops during extraction |

**Why bench testing:** These tests use real ICs to verify SPI protocol, timing, and data integrity — things mocking can't catch. Simulated zero-cross validates ISR and SSR timing without AC mains risk.

---

## Phase 6: Production Tests (Board + Gaggia Machine)

**Goal:** Verify the complete system with real thermal dynamics, real AC power switching, and real water flow. This is the final gate before the machine operates unattended.

### Board Compatibility

The project targets PCA10040. A production board uses the same nRF52832 with identical pin mappings. No production-specific code paths exist.

| Aspect | DK (PCA10040) | Production Board |
|---|---|---|
| MCU | nRF52832 on-board | nRF52832 (same) |
| Pin assignments | `app_config.h` | Same `app_config.h` |
| SWD debug access | On-board J-Link | External J-Link via header/pogo pads |
| SSR outputs (P0.19-20) | LEDs or breakout | Real SSR gate drivers |
| Zero-cross (P0.26) | 555 timer | Real AC phase detector circuit |
| Brew/Steam (P0.27-28) | Push buttons | Real AC-isolated switches |

**SWD conflict:** P0.25 is used for both solenoid relay and SWDCLK. Production PCB must add a series resistor + test header, or a mux/jumper.

### Safety Requirements (mandatory)

The codebase has **no watchdog timer**. Test firmware that hangs with SSR outputs HIGH on AC hardware is a fire hazard.

```c
static void safety_init(void) {
    // 1. Force all SSR outputs OFF immediately
    nrf_gpio_cfg_output(outSSRboiler_PIN);
    nrf_gpio_cfg_output(outSSRpump_PIN);
    nrf_gpio_cfg_output(enSolenoidRelay_PIN);
    nrf_gpio_pin_clear(outSSRboiler_PIN);
    nrf_gpio_pin_clear(outSSRpump_PIN);
    nrf_gpio_pin_clear(enSolenoidRelay_PIN);

    // 2. Check reset reason — if watchdog, halt
    if (NRF_POWER->RESETREAS & POWER_RESETREAS_DOG_Msk) {
        SEGGER_RTT_WriteString(0, "WATCHDOG RESET — SAFETY HALT\n");
        while (1) { __WFE(); }
    }

    // 3. Enable watchdog (1s timeout)
    NRF_WDT->CONFIG = WDT_CONFIG_HALT_Pause | WDT_CONFIG_SLEEP_Pause;
    NRF_WDT->CRV = 32768;
    NRF_WDT->RREN = WDT_RREN_RR0_Enabled;
    NRF_WDT->TASKS_START = 1;
}

static void wdt_feed(void) { NRF_WDT->RR[0] = WDT_RR_RR_Reload; }
```

**Test classification by safety level:**

| Safety Level | Tests | Allowed On |
|---|---|---|
| **Read-only** | RTD temp read, NVM read, zero-cross counting | DK + Production HW |
| **Actuating (low risk)** | SSR toggle with LED loads | DK + bench breakouts only |
| **Actuating (high risk)** | SSR driving real heater/pump, solenoid valve | Production HW with watchdog + operator present |

### Prerequisites

- Safety layer implemented (above)
- All Phase 5 bench tests passing
- Operator physically present during actuating tests
- SWD debug header accessible

### On-Target Test Firmware

Single image with compile-time flag for actuating tests:

```c
// test_main_production.c
#include "unity.h"
#include "SEGGER_RTT.h"

#define UNITY_OUTPUT_CHAR(c) SEGGER_RTT_PutChar(0, c)

extern void test_spi_rtd_init(void);
extern void test_spi_rtd_read_temperature(void);
extern void test_spi_nvm_read_device_id(void);
extern void test_spi_nvm_write_read_page(void);
extern void test_zero_cross_isr_fires(void);
extern void test_brew_switch_debounce(void);

#ifdef ENABLE_ACTUATING_TESTS
extern void test_ssr_boiler_power_output(void);
extern void test_ssr_pump_ramp_sequence(void);
extern void test_solenoid_toggle(void);
extern void test_pid_closed_loop_convergence(void);
#endif

int main(void) {
    safety_init();

    UNITY_BEGIN();
    RUN_TEST(test_spi_rtd_init);             wdt_feed();
    RUN_TEST(test_spi_rtd_read_temperature); wdt_feed();
    RUN_TEST(test_spi_nvm_read_device_id);   wdt_feed();
    RUN_TEST(test_spi_nvm_write_read_page);  wdt_feed();
    RUN_TEST(test_zero_cross_isr_fires);     wdt_feed();
    RUN_TEST(test_brew_switch_debounce);     wdt_feed();

#ifdef ENABLE_ACTUATING_TESTS
    RUN_TEST(test_ssr_boiler_power_output);  wdt_feed();
    RUN_TEST(test_ssr_pump_ramp_sequence);   wdt_feed();
    RUN_TEST(test_solenoid_toggle);          wdt_feed();
    RUN_TEST(test_pid_closed_loop_convergence); wdt_feed();
#endif

    UNITY_END();
    nrf_gpio_pin_clear(outSSRboiler_PIN);
    nrf_gpio_pin_clear(outSSRpump_PIN);
    nrf_gpio_pin_clear(enSolenoidRelay_PIN);
    SEGGER_RTT_WriteString(0, "\nTESTS COMPLETE\n");
    while (1) { __WFE(); }
}
```

```bash
# Read-only tests (safe, automatable)
arm-none-eabi-gcc ... -o test_readonly.hex

# All tests including actuating (operator required)
arm-none-eabi-gcc ... -DENABLE_ACTUATING_TESTS -o test_production.hex
```

### Verification Criteria

| Test | Stimulus | Pass Criterion | Why It Matters |
|---|---|---|---|
| Real AC zero-cross | Count ISR over 1s from mains | 100±1 (50Hz) or 120±1 (60Hz); jitter < 100µs σ | Noisy zero-cross → SSR fires at wrong phase → flicker/EMI |
| SSR heater power | Set power=500, monitor boiler temp | Temperature rises; rate ∝ power (±20%) | Gate drive insufficient → SSR doesn't turn on |
| SSR pump drive | Set power=1000 | Motor runs, water flows | Phase-angle wrong → motor stalls |
| Solenoid actuation | Toggle P0.25 | Audible click; water routing changes | GPIO current too low → valve stuck |
| PT100 in boiler | Read RTD during heating | Tracks ambient→95°C; matches reference ±2°C | Conversion coefficients wrong → displayed temp offset |
| PID convergence | Set target=93°C, wait | Reaches target within 10min; overshoot < 3°C; steady-state ±1°C | Gains wrong for real thermal mass → oscillation |
| I-boost during brew | Activate brew 25s | Temp drop < 5°C; recovery to ±1°C within 60s | Gain too low → under-extracted espresso |
| Classic brew cycle | Press brew switch | Pump+solenoid on; both off on release; no stuck outputs | State machine bug → pump dead-heads or solenoid stuck |
| Profile brew cycle | Activate via BLE | RTT shows 3 stages matching configured power/duration ±5% | Ramp error → wrong pressure profile |
| Steam mode | Press steam switch | Setpoint → `sp_StemTemp`; heater power increases | Setpoint not loaded → steam too cold or overheats |
| NVM persistence | Write via BLE, power cycle, read back | All fields match | Flash sequence broken → settings reset every reboot |
| Full init | Flash app, capture RTT | All "READY", BLE advertising within 5s | Production HW difference breaks a driver |

**Why production testing:** This is the only phase that validates the complete system — real thermal dynamics, real AC switching, real water flow. Bench tests verify the electronics; production tests verify the *control system*: that PID gains suit the actual boiler, that SSR gate drive handles real loads, and that state machines work with real-world timing.

---

## CI Integration (GitHub Actions self-hosted runner)

Applies to Phases 4-6. External HW tests run only on runners with the hardware connected.

```yaml
# .github/workflows/hil.yml
name: HIL Tests
on: [push, pull_request]

jobs:
  bare-dk:
    runs-on: [self-hosted, nrf52-dk]
    steps:
      - uses: actions/checkout@v4
      - name: Verify board
        run: nrfjprog --ids
      - name: Flash & run bare DK tests
        run: ./scripts/hil_test.sh build/test_bare_dk.hex
      - name: Flash app & run BLE tests
        run: |
          nrfjprog -f nrf52 --program build/app_merged.hex --sectorerase --reset
          sleep 3
          python3 scripts/ble_test.py

  bench:
    runs-on: [self-hosted, nrf52-dk, has-rtd, has-nvm]
    steps:
      - uses: actions/checkout@v4
      - name: Flash & run bench tests
        run: ./scripts/hil_test.sh build/test_bench.hex
```

**Runner setup on Linux host:**
```bash
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="1366", MODE="0666"' | \
  sudo tee /etc/udev/rules.d/99-jlink.rules
sudo usermod -aG dialout,plugdev $USER
```

---

## Implementation Order

1. **Phase 1** — Pure algorithm tests. Validates test infra works. No stubs, no mocks.
2. **Phase 2** — Application logic tests with FFF fakes. Validates stub headers and mock strategy.
3. **Phase 3** — Migrate to Ceedling+CMock. Reuse stubs. Adds strict call verification and coverage.
4. **Phase 4** — Flash to bare DK. Validate timers, GPIO, state machines, BLE.
5. **Phase 5** — DK + breakout peripherals. Test SPI, NVM, zero-cross, SSR with LED loads.
6. **Phase 6** — Production board + Gaggia. Safety layer first, then read-only tests, then actuating tests with operator present.

## Files to Modify in Source (minimal)

| File | Change | Purpose |
|---|---|---|
| `tempController.c` | Add `#ifdef TEST` accessor for `milisTicks` | Allow tests to control time |
| `BLEspressoServices.c` | Guard `#include "bluetooth_drv.h"` with `#ifndef TEST` or add to stubs | Isolate BLE dependency |
| `sdk_config.h` | Enable `NRF_LOG_BACKEND_RTT_ENABLED 1` | RTT output for on-target tests |

No other source changes required. All isolation is done via stub headers and include path ordering.
