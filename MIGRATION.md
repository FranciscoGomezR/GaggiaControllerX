# Migration Guide: nRF5 SDK 17.1.0 → nRF-Connect SDK (NCS) v3.2.4

**Project:** BLEspresso – Gaggia Classic espresso machine controller  
**Author:** AI-assisted migration (GitHub Copilot)  
**Date:** April 2026  
**Target hardware:** nRF52832 QFAB, QFN package, 256 KB FLASH, 32 KB RAM, SEGGER J-Link Base

---

## 1. Background

The original firmware (in `ble_espresso_app/`) was built with the **nRF5 SDK 17.1.0** and the
**SoftDevice S132** v7.x BLE stack, using Segger Embedded Studio (SES) as the IDE.

The migrated firmware (in `ble_espresso_app_ncs/`) targets the **nRF-Connect SDK (NCS) v3.2.4**
built on Zephyr RTOS 4.2.99.  NCS replaces the nRF5 SDK and provides a unified build system,
hardware abstraction, and a full BLE host stack integrated into the OS.

---

## 2. Environment Setup

### Toolchain installed

| Component | Version | Location |
|-----------|---------|----------|
| NCS (west workspace) | 3.2.4 | `C:\ncs\v3.2.4\` |
| Toolchain bundle (cmake, ninja, arm-zephyr-eabi-gcc 12.2.0) | fd21892d0f | `C:\ncs\toolchains\fd21892d0f\` |
| West | 1.4.0 | Python module in toolchain |
| Zephyr SDK (GCC 12.2.0) | 0.17.0 | `C:\ncs\toolchains\fd21892d0f\opt\zephyr-sdk\` |

### Building from the command line

Open a PowerShell terminal in `c:\WS\NRF\GaggiaController` and run:

```powershell
# ── Step 1: Set environment variables (required once per terminal session) ──

$env:ZEPHYR_BASE            = "C:\ncs\v3.2.4\zephyr"
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = "C:\ncs\toolchains\fd21892d0f\opt\zephyr-sdk"
$env:PATH = @(
    "C:\ncs\toolchains\fd21892d0f\opt\bin",
    "C:\ncs\toolchains\fd21892d0f\opt\bin\Scripts",
    "C:\ncs\toolchains\fd21892d0f\opt\zephyr-sdk\arm-zephyr-eabi\bin",
    "C:\ncs\toolchains\fd21892d0f\nrfutil\bin",
    "C:\Program Files\SEGGER\JLink_V876"
) -join ";" -replace "$", ";$env:PATH"

# Verify (should print "West version: v1.4.0")
west --version

# ── Step 1b: Install nrfutil device command (first time only) ──
# Required for west flash / west debug / west attach (J-Link probe communication)
nrfutil install device

# ── Step 2: Build (pristine = clean rebuild) ──

west build ble_espresso_app_ncs `
    --build-dir ble_espresso_app_ncs\build_324 `
    --board nrf52dk/nrf52832 `
    --pristine

# ── Step 3: Flash (J-Link must be connected via USB) ──

west flash --build-dir ble_espresso_app_ncs\build_324

# ── Step 4: Debug (flash + launch GDB, halts at main) ──

west debug --build-dir ble_espresso_app_ncs\build_324

# ── (Alternative) Attach debugger without re-flashing ──

west attach --build-dir ble_espresso_app_ncs\build_324
```

> **Notes:**
> - `--pristine` forces a full clean rebuild. Omit it for incremental builds.
> - `west debug` flashes the firmware first, then opens an interactive GDB session
>   connected to the J-Link probe. Type `c` to continue, `b main` to set breakpoints.
> - `west attach` connects GDB to a running target without flashing — useful when
>   the firmware is already loaded and you want to inspect a live system.
> - Board uses the **slash-qualifier syntax** introduced in NCS v2.7+:
>   `nrf52dk/nrf52832` (not the old `nrf52dk_nrf52832`).

### Building from VS Code

1. Install the **nRF Connect for VS Code** extension pack (`nordic-semiconductor.nrf-connect-extension-pack`).
2. Open the workspace folder `C:\WS\NRF\GaggiaController`.
3. In the **nRF Connect** panel → **Applications**, click *Add existing application*
   and select `ble_espresso_app_ncs`.
4. Add a build configuration:  
   - Board: `nrf52dk/nrf52832` (new slash-qualifier syntax in NCS v3.x)  
   - Leave other options at defaults.
5. Use the **Build** and **Flash** buttons in the VS Code panel.

> **Tip:** The nRF Connect sidebar can be opened via the Activity Bar icon or
> with command `workbench.view.extension.nrf-connect`.

> **Disk cleanup:** NCS v2.0.0 SDK and toolchain bundles v2.0.0/v2.0.1 have
> been removed. Only `C:\ncs\v3.2.4\` and toolchain `fd21892d0f` remain on disk.

---

## 3. Project Structure (New)

```
ble_espresso_app_ncs/
├── CMakeLists.txt             ← Zephyr application entry, list of source files
├── Kconfig                    ← Minimal Kconfig wrapper (sources Kconfig.zephyr)
├── prj.conf                   ← Zephyr Kconfig flags (replaces sdk_config.h)
├── boards/
│   └── nrf52dk_nrf52832.overlay  ← Devicetree overlay (pin assignments, peripherals)
└── src/
    ├── main.c                 ← Application entry-point and scheduler loop
    ├── app/
    │   ├── BLEspressoServices.h/.c  ← Application service state machines
    │   ├── StorageController.h/.c   ← NOR flash persistence (W25Q64FV)
    │   ├── tempController.h/.c      ← Boiler PID temperature controller
    │   └── PumpController.h/.c      ← Pump pressure profile state machine
    ├── ble/
    │   ├── ble_cus.h/.c        ← Custom GATT service (BT_GATT_SERVICE_DEFINE)
    │   └── bluetooth_drv.h/.c  ← BLE stack init, advertising, connection handler
    ├── drv/
    │   ├── ac_inputs_drv.h/.c  ← AC zero-cross / switch GPIO ISR driver
    │   ├── solidStateRelay_Controller.h/.c  ← SSR phase-angle and burst driver
    │   ├── spi_Devices.h/.c    ← MAX31865 RTD + W25Q64FV NOR flash (SPI1)
    │   ├── dc12Vouput_drv.h/.c ← 12 V lamp dimmer (PWM0)
    │   ├── i2c_sensors.h/.c    ← I2C sensor stub (disabled, see §6)
    │   └── board_comp_drv.h/.c ← Heartbeat LED (P0.29)
    └── utils/                  ← Pure-C algorithm files (unchanged)
        ├── x01_StateMachineControls.h
        ├── x02_FlagValues.h
        ├── x03_MathConstants.h
        ├── x04_Numbers.c/.h
        ├── x201_DigitalFiltersAlgorithm.c/.h
        └── x205_PID_Block.c/.h
```

---

## 4. Pin Assignments (unchanged from original)

| Signal | GPIO | Function |
|--------|------|----------|
| inBREW | P0.27 | AC brew-switch opto input (interrupt) |
| inSTEAM | P0.28 | AC steam-switch opto input (interrupt) |
| inZEROCROSS | P0.26 | AC zero-cross detector (interrupt) |
| outSSRboiler | P0.19 | Boiler solid-state relay output |
| outSSRpump | P0.20 | Pump solid-state relay output |
| enSolenoid | P0.25 | Solenoid valve relay |
| enDC12V (PWM) | P0.3 | 12 V dimmer (PWM0 channel 0) |
| SPI1_SCK | P0.13 | MAX31865 + W25Q64 clock |
| SPI1_MOSI | P0.14 | SPI master-out |
| SPI1_MISO | P0.11 | SPI master-in |
| SPI1_CS (MAX31865) | P0.12 | Temperature sensor chip-select |
| SPI_NVM_CS (W25Q64) | P0.15 | NOR flash chip-select |
| UART_TX | P0.6 | Serial debug (115200 baud) |
| LED_HEARTBEAT | P0.29 | Debug heartbeat LED |

> **Note:** P0.26 (inZEROCROSS) and P0.27 (inBREW) conflict with the default
> Arduino I2C pins (SDA/SCL) of the nRF52DK board.  I2C is **disabled** in the
> devicetree overlay, and `i2c_sensors.c` contains stub implementations only.

---

## 5. API Migration Reference

The table below lists every nRF5 SDK API replaced and its Zephyr NCS equivalent.

### 5.1 Logging

| nRF5 SDK | Zephyr NCS |
|----------|-----------|
| `NRF_LOG_DEBUG("msg")` | `LOG_DBG("msg")` |
| `NRF_LOG_INFO("msg")` | `LOG_INF("msg")` |
| `NRF_LOG_ERROR("msg")` | `LOG_ERR("msg")` |
| `NRF_LOG_RAW_INFO("%s", str)` | `LOG_INF("%s", str)` |
| `NRF_LOG_FLUSH()` | (not needed – async logging) |
| `NRF_LOG_FLOAT_MARKER` / `NRF_LOG_FLOAT()` | `(double)val` cast directly in `LOG_*` |
| Per-module `log_init()` | `LOG_MODULE_REGISTER(name, level)` |

### 5.2 GPIO / Interrupts

| nRF5 SDK | Zephyr NCS |
|----------|-----------|
| `nrf_drv_gpiote_init()` | (automatic at boot) |
| `nrf_drv_gpiote_in_init(pin, cfg, cb)` | `gpio_pin_configure()` + `gpio_pin_interrupt_configure()` + `gpio_add_callback()` |
| `nrf_drv_gpiote_out_init(pin, cfg)` | `gpio_pin_configure(dev, pin, GPIO_OUTPUT)` |
| `nrf_drv_gpiote_out_toggle(pin)` | `gpio_pin_toggle(dev, pin)` |
| `GPIOTE_CONFIG_IN_SENSE_TOGGLE(...)` | `GPIO_INT_EDGE_BOTH` |
| ISR: `void h(nrf_drv_gpiote_pin_t, nrf_gpiote_polarity_t)` | ISR: `void h(const struct device*, struct gpio_callback*, uint32_t pins)` |
| Device: implicit | Device: `DEVICE_DT_GET(DT_NODELABEL(gpio0))` |

### 5.3 SPI

| nRF5 SDK | Zephyr NCS |
|----------|-----------|
| `NRF_DRV_SPI_INSTANCE(1)` | `DEVICE_DT_GET(DT_NODELABEL(spi1))` |
| `nrf_drv_spi_init()` | (device ready via DTS + `device_is_ready()` check) |
| `nrf_drv_spi_transfer(buf, len, rx, rlen)` | `spi_transceive(dev, &cfg, &tx_set, &rx_set)` |
| Manual CS via `nrf_gpio_pin_set/clear` | `gpio_pin_set(gpio_dev, CS_PIN, 0/1)` |

### 5.4 Hardware Timers → Zephyr Counter

| nRF5 SDK | Zephyr NCS |
|----------|-----------|
| `NRF_DRV_TIMER_INSTANCE(N)` | `DEVICE_DT_GET(DT_NODELABEL(timerN))` |
| `nrf_drv_timer_init()` | (auto via DTS; check `device_is_ready()`) |
| `nrf_drv_timer_enable()` | `counter_start(dev)` |
| `nrf_drv_timer_disable()` | `counter_stop(dev)` |
| `nrf_drv_timer_extended_compare(timer, val, flags, enable_irq)` | `counter_set_channel_alarm(dev, ch, &alarm_cfg)` |
| Callback: `void h(nrf_timer_event_t, void*)` | Callback: `void h(const struct device*, uint8_t ch, uint32_t ticks, void*)` |
| `nrf_drv_timer_us_to_ticks(val)` | `counter_us_to_ticks(dev, val)` |
| Requires `CONFIG_COUNTER_TIMER2=y` in prj.conf | — |

### 5.5 App Timer → Zephyr k_timer

| nRF5 SDK | Zephyr NCS |
|----------|-----------|
| `APP_TIMER_DEF(id)` | `K_TIMER_DEFINE(id, handler, stop_fn)` |
| `app_timer_init()` | (not needed) |
| `app_timer_create(&id, mode, handler)` | (defined at declaration) |
| `app_timer_start(id, ticks, ctx)` | `k_timer_start(&id, K_MSEC(ms), K_MSEC(ms))` |
| `app_timer_stop(id)` | `k_timer_stop(&id)` |
| Handler: `void h(void *ctx)` | Handler: `void h(struct k_timer *id)` |

### 5.6 BLE Stack (SoftDevice S132 → Zephyr BT Host)

| nRF5 SDK | Zephyr NCS |
|----------|-----------|
| `nrf_sdh_ble_enable()` + stack init chain | `bt_enable(NULL)` |
| `ble_gap_device_name_set()` | `CONFIG_BT_DEVICE_NAME` in prj.conf |
| `BLE_CUS_DEF()` + `BLE_GATTS_ATTR_*` | `BT_GATT_SERVICE_DEFINE()` with attribute array |
| `sd_ble_gatts_hvx()` | `bt_gatt_notify(conn, attr, data, len)` |
| `NRF_SDH_BLE_OBSERVER(conn_cb)` | `BT_CONN_CB_DEFINE(conn_callbacks)` |
| `ble_advertising_start()` | `bt_le_adv_start(&adv_param, ad, n, sd, n)` |
| `BLE_GAP_ADV_TYPE_*` | `BT_LE_ADV_PARAM()` macros |
| `pm_init()` + peer manager | `settings_load()` (bonding persisted via NVS) |
| `pm_peers_delete()` | `bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY)` |

### 5.7 PWM

| nRF5 SDK | Zephyr NCS |
|----------|-----------|
| `NRF_DRV_PWM_INSTANCE(0)` | `PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0))` |
| `nrf_drv_pwm_init()` | (automatic via DTS) |
| `nrf_drv_pwm_simple_playback()` | `pwm_set_dt(&spec, period_ns, pulse_ns)` |

### 5.8 Delay / Power Management

| nRF5 SDK | Zephyr NCS |
|----------|-----------|
| `nrf_delay_ms(N)` | `k_msleep(N)` |
| `nrf_delay_us(N)` | `k_usleep(N)` |
| `nrf_pwr_mgmt_run()` (main loop) | `k_yield()` (Zephyr idle handles sleep) |

### 5.9 NVM Storage

| nRF5 SDK | Zephyr NCS |
|----------|-----------|
| `nrf_fstorage` + `nrf_fstorage_sd` | SPI NOR flash (W25Q64FV) via `spi_Devices.c` |
| `fds_record_write()` | `spi_NVMemoryPageWrite()` |
| `fds_record_read()` | `spi_NVMemoryRead()` |

The project stores `bleSpressoUserdata_struct` (65 bytes) at page 0 of the
external W25Q64FV SPI NOR flash.  A 4-byte magic key (`0x00AA00AA`) at offset 4
validates the stored data.

---

## 6. Devicetree Overlay Details

The overlay at `boards/nrf52dk_nrf52832.overlay` configures:

1. **SPI1 pinctrl** – overrides default pins to match hardware (SCK=P0.13, MOSI=P0.14, MISO=P0.11).
2. **PWM0 pinctrl** – routes channel 0 to P0.3 (12 V dimmer) instead of the default P0.17 (dev kit LED3).
3. **i2c0 disabled** – prevents pin conflict with inZEROCROSS (P0.26) and inBREW (P0.27).
4. **timer2 and timer3 enabled** – these NRF peripherals are status = "okay" in the SoC DTSI but require explicit Kconfig (`CONFIG_COUNTER_TIMER2=y`, `CONFIG_COUNTER_TIMER3=y`) to bind the Zephyr counter driver.

### Why I2C is disabled

The nRF52832 default I2C (TWI0) uses P0.26 (SDA) and P0.27 (SCL).  In the custom hardware, those same pins are used for:
- P0.26 → `inZEROCROSS_PIN` (AC zero-cross interrupt)  
- P0.27 → `inBREW_PIN` (brew switch opto-coupler interrupt)

The `twi_init()` function in the original source was never called from `main()`, confirming I2C was unused at the time of migration.

---

## 7. Custom GATT Service UUIDs

The two custom BLE services use 128-bit manufacturer UUIDs derived from the
original `CUSTOM_SERVICE_UUID_BASE[]` array (little-endian byte order):

| Service | Handle in original | 128-bit UUID |
|---------|-------------------|-------------|
| BLEspresso (brew/boiler) | `BLE_SERVICE_BLEESPRESSO_UUID` (0x1400) | `00001400-00b0-4240-ba50-05ca45bf8abc` |
| PID controller | `BLE_SERVICE_PIDESPRESSO_UUID` (0x1500) | `00001500-00b0-4240-ba50-05ca45bf8abc` |

In Zephyr NCS these are expressed with `BT_UUID_128_ENCODE(...)` in `ble_cus.h`.

---

## 8. Key Differences and Caveats

### 8.1 BLE Connection Callbacks

In NCS v2.0.0 (Zephyr 3.0.99), `struct bt_conn_cb` does **not** have a
`le_phy_updated` member (that was added in a later NCS version).  The migration
removes that callback.  If PHY update logging is needed, register a callback
via `bt_le_set_auto_conn()` or add it when upgrading to a newer NCS version.

### 8.2 Counter / Hardware Timer API

Zephyr's counter driver in NCS v2.0.0 does not have `counter_reset()`.  The
timer value automatically resets to 0 when `counter_start()` is called.  The
migration replaces every `counter_reset()` call with just allowing the
subsequent `counter_start()` to reset the counter.

### 8.3 Floating-Point Math

Zephyr's **minimal libc** does not include all C99 single-precision math
functions (`sqrtf`, `fabsf`).  The build uses:

```kconfig
CONFIG_PICOLIBC=y
CONFIG_FPU=y
CONFIG_FPU_SHARING=y
```

`CONFIG_PICOLIBC=y` (default in Zephyr 4.x) provides all standard C99 math
functions while using significantly less RAM than newlib.  `CONFIG_FPU_SHARING=y`
ensures the FPU context is correctly saved/restored across context switches
(required when both the application and BLE kernel threads use floating-point).

`CONFIG_CBPRINTF_FP_SUPPORT=y` is also required for float formatting in
`LOG_INF("%f", ...)` style log macros.

### 8.4 Memory Usage (nRF52832 QFAB target)

| Region | Used | Target limit | Margin |
|--------|------|--------------|--------|
| FLASH | 203,068 B (198 KB) | 256 KB | +58 KB |
| SRAM | 32,042 B (31.3 KB) | 32 KB | +726 B |

The build board is `nrf52dk_nrf52832` which targets the full nRF52832 die
(512 KB / 64 KB on the DK).  The binary **fits** within the QFAB's smaller flash and
RAM limits.  Key memory optimizations applied:
- `CONFIG_PICOLIBC=y` instead of `CONFIG_NEWLIB_LIBC=y` (saves ~4 KB RAM)
- `CONFIG_MAIN_STACK_SIZE=2048` (reduced from 4096)
- `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=1024` (reduced from 2048)
- `CONFIG_HEAP_MEM_POOL_SIZE=1024` (reduced from 2048)
- `CONFIG_SIZE_OPTIMIZATIONS=y` (uses `-Os` compiler optimization)

---

## 9. Build Verification Results

```
Build successful (NCS v3.2.4 / Zephyr 4.2.99 / GCC 12.2.0):
[317/317] Linking C executable zephyr\zephyr.elf
Memory region    Used Size   Region Size   %age Used
       FLASH:    203068 B      504 KB       39.35%
         RAM:     32042 B       64 KB       48.89%
```

Fits within QFAB hardware limits (256 KB FLASH / 32 KB RAM): 198 KB FLASH used,
31.3 KB RAM used.

0 errors, 0 warnings.  All application source files compile cleanly under
`-std=c99 -Wall -mcpu=cortex-m4 -mthumb -mabi=aapcs -Os` (size-optimized build).

### 9.1 Compilation fixes applied

- **spi_Devices.h**: added `#include <stdbool.h>` / `#include <stdint.h>` (missing type `bool`)
- **main.c**: added `static uint32_t appModeToRun = machine_App;`, removed unused `userDataLoadedFlag`
- **ble_cus.c**: added `#include <stdio.h>` (for `snprintf`)
- **ble_cus.h**: added `ble_cus_on_connected()` / `ble_cus_on_disconnected()` declarations
- **bluetooth_drv.c**: `BT_LE_ADV_CONN` → `BT_LE_ADV_CONN_FAST_1` (NCS v3.x removed the short macro)
- **solidStateRelay_Controller.h**: added pin defines from original `app_config.h`
- **solidStateRelay_Controller.c**: removed `volatile` from `sSSRcontroller` struct
- **dc12Vouput_drv.h**: added `DRV_12VO_INIT_ERROR` to enum
- **DTS overlay**: added `pwm_12v` node under `pwmleds`
- **PumpController.c**: initialized `pumpStatus = PUMPCTRL_ERROR`
- **x04_Numbers.c**: `0.1` / `0.01` → `0.1f` / `0.01f` (fixes `-Wdouble-promotion`)

---

## 10. Flashing

Connect the SEGGER J-Link Base to the JTAG/SWD header of the custom PCB.

### From the command line

```powershell
# (run after sourcing NCS v3.2.4 toolchain env — see §2 Step 1)
west flash --build-dir ble_espresso_app_ncs\build_324
```

### From VS Code

Use the **Flash** button in the *nRF Connect* extension panel (after successful build).

> **No SoftDevice required:** Zephyr BLE uses its own BLE controller from the
> Nordic SDK libraries and does **not** use SoftDevice S132. There is no separate
> SoftDevice hex to flash.

---

## 11. Debugging

### 11.1 Command-line debug (GDB via J-Link)

```powershell
# Flash firmware and launch interactive GDB session
west debug --build-dir ble_espresso_app_ncs\build_324
```

This flashes the firmware, then opens `arm-zephyr-eabi-gdb` connected to the
J-Link probe. The target halts at the reset vector. Common GDB commands:

| Command | Action |
|---------|--------|
| `b main` | Set breakpoint at `main()` |
| `c` | Continue execution |
| `n` | Step over (next line) |
| `s` | Step into function |
| `p variable` | Print variable value |
| `bt` | Show backtrace |
| `info threads` | List Zephyr threads |
| `quit` | Exit GDB |

To attach to an already-running target **without re-flashing**:

```powershell
west attach --build-dir ble_espresso_app_ncs\build_324
```

### 11.2 VS Code debug (nRF Connect extension)

The nRF Connect VS Code extension uses J-Link GDB for source-level debugging
with breakpoints, variable watch, and RTOS thread view.

**prj.conf debug options** (already included):

```kconfig
CONFIG_DEBUG=y                 # umbrella debug flag (enables -g DWARF symbols)
CONFIG_DEBUG_THREAD_INFO=y     # expose Zephyr thread list to GDB
```

> **For stepping-friendly debugging**, temporarily replace `CONFIG_SIZE_OPTIMIZATIONS=y`
> with `CONFIG_DEBUG_OPTIMIZATIONS=y` in `prj.conf`. This compiles with `-Og`
> instead of `-Os`, keeping variables visible and preventing aggressive inlining.
> Rebuild after changing. Note: this increases FLASH/RAM usage.

**`.vscode/launch.json`** (create if not present):

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "nrf-connect",
            "request": "launch",
            "name": "Debug BLEspresso",
            "config": "${workspaceFolder}/ble_espresso_app_ncs/build_324",
            "runToEntryPoint": "main"
        }
    ]
}
```

**Steps:**

1. Build the project.
2. Connect J-Link Base via USB.
3. Press **F5** in VS Code (or *Run → Start Debugging*).
4. The debugger flashes, halts at `main()`, and shows source with variable inspection.

---

## 12. Serial Debug Output

Connect a USB-to-UART adapter to P0.6 (TX) / GND. Configure terminal at
**115200 baud, 8N1**.  The LOG backend outputs structured log messages to UART.

Sample startup output:
```
*** Booting Zephyr OS build v4.2.99-ncs-v3.2.4 ***
[00:00:00.001] <inf> main: DRV INIT SPI interface: READY
[00:00:00.005] <inf> main: DRV INIT SPI-Temperature Sensor: READY
[00:00:00.006] <inf> main: DRV INIT 12VDC output: SUCCESSFUL
...
[00:00:00.020] <inf> bt_hci_core: HW Platform: Nordic Semiconductor
[00:00:00.025] <inf> main: MACHINE ::Application Mode::
```

---

## 13. Step-by-Step Migration Summary

1. **Identify all nRF5 SDK API dependencies** — Review every `#include` and map
   to the NCS/Zephyr equivalent (see §5).

2. **Create the NCS project scaffold** — `CMakeLists.txt`, `Kconfig`, `prj.conf`.

3. **Write the Devicetree overlay** — Override SPI1 pins, PWM0 pin, disable I2C,
   enable timer2 and timer3 for counter driver.

4. **Copy pure algorithm files unchanged** — Files in `src/utils/` have no
   platform dependencies; only add `#include <math.h>` where needed.

5. **Port driver files** — Replace each nRF5 SDK peripheral driver call with the
   Zephyr driver API (see §5).  The ISR signatures change.

6. **Port the BLE layer** — Replace SoftDevice services with
   `BT_GATT_SERVICE_DEFINE`, `bt_enable`, `bt_le_adv_start`, `BT_CONN_CB_DEFINE`.

7. **Port the application layer** — `app_timer` → `k_timer`, `NRF_LOG` → `LOG_*`,
   `nrf_pwr_mgmt_run()` → `k_yield()`.  Application logic is unchanged.

8. **Add required Kconfig** — `CONFIG_COUNTER_TIMER2=y`, `CONFIG_COUNTER_TIMER3=y`,
   `CONFIG_NEWLIB_LIBC=y`, `CONFIG_FPU=y`, `CONFIG_FPU_SHARING=y`.

9. **Build and iterate** — Fix compilation errors (`counter_reset` → removed,
   `le_phy_updated` callback removed, function signatures aligned).

10. **Verify memory fit** — Adjust `CONFIG_MAIN_STACK_SIZE` and
    `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` to ensure SRAM fits within the QFAB's
    32 KB limit.

11. **Open in VS Code nRF Connect extension** — Install the nRF Connect for VS Code
    extension pack, open the application folder via the nRF Connect sidebar, and
    add it as an application pointing to the `build/` directory.

12. **Enable debug build** — Add `CONFIG_DEBUG_INFO=y`, `CONFIG_DEBUG_OPTIMIZATIONS=y`,
    and `CONFIG_DEBUG_THREAD_INFO=y` to `prj.conf`.  Configure `.vscode/launch.json`
    with `"type": "nrf-connect"` pointing to the build directory.

13. **Flash and debug** — Use `west flash` or press **F5** in VS Code to start a
    source-level debug session via the J-Link Base debugger.
