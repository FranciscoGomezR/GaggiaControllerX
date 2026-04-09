## Session Summary — April 8, 2026

### Goal
Build and flash the `ble_espresso_app` from VS Code without Segger Embedded Studio,
using the nRF5 SDK 17.1.0 toolchain and a J-Link probe.

---

### Environment discovered
| Item | Value |
|---|---|
| MCU | nRF52832_xxAA (Cortex-M4, FPU, 512 KB flash, 64 KB RAM) |
| SoftDevice | s132 v7.2.0 |
| SDK | `C:\WS\NRF\nRF5_SDK_17.1.0_ddde560` |
| ARM GCC | Arm GNU Toolchain 15.2.Rel1 — `C:\Program Files\Arm\GNU Toolchain mingw-w64-x86_64-arm-none-eabi\bin\` |
| J-Link | `C:\Program Files\SEGGER\JLink_V876\JLink.exe` |
| GNU Make | `C:\Users\franc\AppData\Local\Microsoft\WinGet\Packages\ezwinports.make_Microsoft.Winget.Source_8wekyb3d8bbwe\bin\make.exe` |

---

### Files created

| File | Purpose |
|---|---|
| `ble_espresso_app/pca10040/s132/armgcc/Makefile` | Full GCC build system — all sources, include paths, compiler/linker flags |
| `ble_espresso_app/pca10040/s132/armgcc/ble_espresso_gcc_nrf52.ld` | GNU linker script (FLASH origin `0x26000`, RAM origin `0x20002270`) |
| `ble_espresso_app/pca10040/s132/armgcc/flash_app.jlink` | J-Link Commander script: flash application only |
| `ble_espresso_app/pca10040/s132/armgcc/flash_softdevice.jlink` | J-Link Commander script: flash s132 softdevice |
| `ble_espresso_app/pca10040/s132/armgcc/flash_all.jlink` | J-Link Commander script: erase + softdevice + app |
| `ble_espresso_app/pca10040/s132/armgcc/erase.jlink` | J-Link Commander script: full chip erase |
| `.vscode/tasks.json` | VS Code tasks for build, clean, and all 3 flash scenarios |
| `.vscode/c_cpp_properties.json` | IntelliSense config for Cortex-M4 + s132 + SDK headers |

---

### Bugs fixed during setup

1. **`Makefile.windows` missing trailing `/`** on `GNU_INSTALL_ROOT` path
   (`C:\WS\NRF\nRF5_SDK_17.1.0_ddde560\components\toolchain\gcc\Makefile.windows`)
   → Added trailing `/` so the compiler path resolves correctly.

2. **`board_comp_drv.c` missing include** — `sleep_mode_enter()`, `ble_disconnect()`,
   `ble_restart_without_whitelist()` are declared in `bluetooth_drv.h` but that header
   was not included. Added `#include "bluetooth_drv.h"`.

3. **`StorageController.c` uses `mempcpy`** — GNU glibc extension not available in
   ARM newlib-nano. Replaced 4 calls with `memcpy` (return value unused in all cases).

4. **Linker script RAM size too small** — SES project value `0x5D90` was conservative.
   Actual available RAM from `0x20002270` to end of 64 KB is `0xDD90`. Updated linker script.

5. **GNU Make PATH for `rm`** — SDK's `Makefile.common` uses Unix `rm -rf` for clean.
   Since Git Bash ships `rm.exe` at `C:\Program Files\Git\usr\bin\`, added that path to
   `PATH` in the Makefile's `export PATH` line.

6. **SDK_ROOT relative depth** — SES project used 4 `../` levels; the armgcc folder
   is one level deeper, requiring 5 `../` levels to reach the SDK. Fixed in Makefile.

7. **VS Code tasks couldn't find `make`** — The integrated terminal inherited a stale
   PATH from before `winget` installed make. Fixed by adding the make install directory
   to `PATH` in the global `"options": { "env": { "PATH": ... } }` block in `.vscode/tasks.json`.
   This makes tasks self-contained regardless of the terminal session's PATH state.

8. **`%.1f` / `%.2f` printed empty in RTT log** — `--specs=nano.specs` (newlib-nano)
   strips float support from `printf`/`sprintf` by default to save code space.
   The SES project had `linker_printf_fmt_level="long"` which enabled it.
   Fixed by adding `-u _printf_float` to `LDFLAGS` in the Makefile.
   This matches the expected log format:
   `[System]  <Monitor:;00074500;0717;96.0;95.23;0000;`

---

### Build output
- Output: `ble_espresso_app/pca10040/s132/armgcc/_build/nrf52832_xxaa.hex` (~234 KB)
- Build command: `make -j4` (from the `armgcc/` directory)

---

### VS Code workflow
- **`Ctrl+Shift+B`** → default build task
- **Terminal → Run Task** → pick a flash scenario:

| Task label | When to use |
|---|---|
| `Flash: application only` | Normal development (softdevice already on chip) |
| `Flash: softdevice only (s132 v7.2.0)` | Once on a new or erased chip |
| `Flash: full (erase + softdevice + application)` | Recovery / production / clean slate |
| `Flash: erase chip only` | Mass-erase before reprogramming |

### tasks.json design notes (from VS Code docs)
- `"panel": "dedicated"` — each flash task gets its own persistent terminal pane
- `"dependsOrder": "sequence"` — build step must complete before J-Link runs
- `"showReuseMessage": false` — suppresses "press any key to close" banner
- `"detail"` field — shown in the Run Task Quick Pick picker
- Global `"options": { "env": { "PATH": ... } }` — injects make.exe dir into PATH
  for all tasks without requiring a terminal restart

---

### Still TODO
- Write automated tests that an AI agent can run and monitor for faster iteration
- Explore porting the project from nRF5 SDK to Zephyr RTOS
---
