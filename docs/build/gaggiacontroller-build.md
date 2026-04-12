# GaggiaController Build Notes

## SDK Setup
- SDK: `C:/WS/NRF/nRF5_SDK_17.1.0_ddde560` (sibling of GaggiaController, NOT inside it)
- Project macro `SDK_ROOT` defined in .emProject macros attribute
- MCU: nRF52832-QFAB, SoftDevice S132

## Key Files
- Project file: `ble_espresso_app/pca10040/s132/ses/ble_espresso_app_pca10040_s132.emProject`
- Flash placement: `ble_espresso_app/pca10040/s132/ses/flash_placement.xml`
- Configs: `ble_espresso_app/pca10040/s132/config/` (sdk_config.h, app_config.h)

## SEGGER ES 8.26c Migration Fixes Applied
1. flash_placement.xml: Removed `size="0x4"` from `.text` and `.rodata` ProgramSections
2. Removed `SEGGER_RTT_Syscalls_SES.c` from project (fixes `__vfprintf.h` error)
3. Set `library_io_type="RTT"` in Common configuration
4. Replaced all 207 relative SDK paths (`../../../../nRF5_SDK_17.1.0_ddde560`) with `$(SDK_ROOT)` macro
5. Defined `SDK_ROOT=C:/WS/NRF/nRF5_SDK_17.1.0_ddde560` in project macros

## nRF52832-QFAB Memory Layout (256KB flash, 64KB RAM)
- FLASH total: 0x0–0x40000
- FLASH_PH_SIZE=0x40000, FLASH_START=0x26000 (after S132), FLASH_SIZE=0x17000
- FDS pages: 0x3D000–0x40000 (3 × 4KB virtual pages)
- RAM_START=0x20002270, RAM_SIZE=0x5d90

## Final Working Build Configuration
| Config | Optimization | printf level | Link segments |
|--------|-------------|-------------|--------------|
| Common | Level 2 for size | long + width/precision | FLASH1 256KB |
| Debug override | Optimize For Debug (-Og) | int, no width/precision | FLASH1 256KB |
| Release override | Optimize For Size (-Os) | (inherits long) | hex output |

- Debug: removed `DEBUG_NRF` from preprocessor (removes NRF_LOG rodata)
- Debug: printf/scanf set to "int" level (removes large RTL lookup tables from .rodata)

## For Different Computer
- Only change the `SDK_ROOT` value in the macros attribute of .emProject
- Both engineers must store project at same path OR add a PROJECT_ROOT macro similarly

## Migration Log — SEGGER ES 8.26c + nRF52832-QFAB

### Environment
- SDK: `C:/WS/NRF/nRF5_SDK_17.1.0_ddde560` — sibling of project folder, NOT inside it
- MCU: nRF52832-QFAB (256KB flash, 64KB RAM), SoftDevice S132 v7.2.0
- SEGGER Embedded Studio: Release 8.26c Build 2026021300.61129
- Key files modified:
  - `ble_espresso_app/pca10040/s132/ses/ble_espresso_app_pca10040_s132.emProject`
  - `ble_espresso_app/pca10040/s132/ses/flash_placement.xml`

---
### Fix 1 — SDK Paths
**Problem:** All 207 SDK references used relative path `../../../../nRF5_SDK_17.1.0_ddde560/...` resolving inside GaggiaController instead of the sibling SDK folder.
**Fix:** Added macro `SDK_ROOT=C:/WS/NRF/nRF5_SDK_17.1.0_ddde560` to `macros=` attribute; replaced all 207 paths with `$(SDK_ROOT)/...`. To use on a different computer, change only `SDK_ROOT` in one place.

### Fix 2 — SEGGER ES 8.26c Compatibility
Three problems introduced by the new SES version:
1. `flash_placement.xml` had `size="0x4"` on `.text`/`.rodata` → linker "too large" error → **removed the `size` attribute**
2. `SEGGER_RTT_Syscalls_SES.c` → `__vfprintf.h: No such file or directory` → **removed file from project**
3. Library I/O type not configured → **set `library_io_type="RTT"` in Common config**

### Fix 3 — FDS Init Failure (`0x860A = FDS_ERR_NO_PAGES`)
**Problem:** Project used `FLASH_PH_SIZE=0x80000` (512KB / xxAA variant). Debug build at -O0 grew to ~131KB, extending past `0x40000`, overwriting FDS pages at `0x3D000–0x40000`. peer_manager surfaces this misleadingly as `NRF_ERROR_STORAGE_FULL`.
**Fix:** Corrected linker macros to QFAB 256KB: `FLASH_PH_SIZE=0x40000`, `FLASH_SIZE=0x17000`, all `FLASH1` segments to `0x00040000`.
Run **`nrfjprog -e`** before reflashing to erase corrupted FDS pages.

### Fix 4 — Debug Build Too Large for 92KB App Window
**Problem:** With correct 256KB limits, Debug build (~131KB at -O0) exceeded the 92KB app window.
**Fix in two stages:**
1. `gcc_optimization_level`: `None` (-O0) → `Optimize For Debug` (-Og); removed `DEBUG_NRF` from preprocessor (NRF_LOG format strings added ~20–30KB to `.rodata`). Result: `.text` fits, `.rodata`/`.data` still overflow.
2. Debug config overrides: `linker_printf_fmt_level`=`"int"`, `linker_printf_width_precision_supported`=`"No"`, `linker_scanf_fmt_level`=`"int"` — the `"long"` RTL variant adds ~10–20KB const tables that `-Og` does not strip (unlike `-Os`).

---

### What Did NOT Work
- **`DEBUG_NRF` in Debug:** adds ~20–30KB NRF_LOG format strings to `.rodata` → overflow
- **`-O0` (None) optimization:** produces ~131KB — exceeds 92KB app window
- **Raising `FLASH_SIZE`:** pushes code into FDS pages (re-introduces Fix 3 problem)
- **Relative SDK paths (`../../../../`):** break when project folder depth or SDK location differs
- **`SEGGER_RTT_Syscalls_SES.c` in project:** causes `__vfprintf.h` error in ES 8.26c+
- **`size="0x4"` on `.text`/`.rodata` in flash_placement.xml:** causes "too large" linker errors in ES 8.26c+

---

### Tools Installed
- **nrfutil v8.1.1** — `winget install NordicSemiconductor.nrfutil`
- **nrfjprog v10.24.2** — present with JLinkARM.dll v8.76
- Run `nrfjprog -e` from PowerShell with J-Link connected to erase device flash