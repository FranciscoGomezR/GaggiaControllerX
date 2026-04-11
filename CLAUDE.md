# BLEspresso — Migration Briefing for AI Agent

## Goal

Migrate the `ble_espresso_app` project from **nRF5 SDK 17.1.0** (SoftDevice S132) to **nRF-Connect SDK (NCS)**, built on Zephyr RTOS.

### Requirements
1. Install NCS SDK and toolchain (via nRF Connect for VS Code extension or `nrfutil toolchain-manager`).
2. Create a new NCS/Zephyr application from the existing `ble_espresso_app/` source.
3. Build with 0 errors, 0 warnings.
4. **Flash to hardware and verify the firmware works correctly at runtime** (BLE advertising, temperature sensing, SSR control, pump control, NVM storage).
5. Use VS Code extensions: nRF Connect for VS Code, nRF Connect Visual Studio Code Extension Pack.
6. Update `MIGRATION.md` with the steps followed. Audience: human engineers.

### Documentation to support migration
- https://devzone.nordicsemi.com/nordic/nordic-blog/b/blog/posts/nrf-connect-sdk-and-nrf5-sdk-statement
- https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/index.html
- https://docs.nordicsemi.com/bundle/ncs-3.0.0/page/nrf/releases_and_maturity/migration_guides.html
- https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/releases_and_maturity/migration/migration_guide_1.x_to_2.x.html
- https://interrupt.memfault.com/blog/upgrading-from-nrf5-sdk-to-ncs
- https://www.byte-lab.com/from-nrf5-sdk-to-nrf-connect-sdk/
- https://www.nordicsemi.com/Products/Development-software/nRF-Connect-SDK

---

## HW Specification

| Parameter | Value |
|-----------|-------|
| MCU | nRF52832 **QFAB** |
| Package | QFN |
| FLASH | **256 KB** |
| RAM | **32 KB** |
| Debug probe | SEGGER J-Link Base |

> **QFAB memory is very tight.** The firmware must fit within 256 KB FLASH and 32 KB RAM.
> Use picolibc (not newlib), `-Os` optimization, and keep stack sizes small.

---

## Documentation

For technical references of the current code, go to: `GaggiaController\docs\`
and `GaggiaController\` to read all `.md` documents except `CLAUDE.md`.

---

## ⚠ CRITICAL — Previous Attempt History

### What has been tried and FAILED

#### Attempt 1: NCS v3.2.4 (April 2026)
- **Result: Compiled successfully but firmware DID NOT WORK at runtime.**
- All 34 source files were ported to `ble_espresso_app_ncs/`.
- Toolchain: NCS v3.2.4 at `C:\ncs\v3.2.4`, Zephyr 4.2.99, GCC 12.2.0.
- Board: `nrf52dk/nrf52832` (slash-qualifier syntax in NCS v2.7+).
- Build: 0 errors, 0 warnings. FLASH 203 KB / 256 KB, RAM 31.3 KB / 32 KB.
- 11 compilation fixes applied (missing includes, API renames, DTS fixes, memory optimization).
- Flashed via J-Link — **the application did not behave correctly on hardware.**
- The migrated code and SDK were deleted. Only documentation was preserved.
- See `MIGRATION.md` for the full API mapping tables, pin assignments, Kconfig settings, DTS overlay, and compilation fixes that were applied. This may be useful as a reference, but **do not blindly repeat the same approach** — it produced code that compiled but did not run.

#### What went wrong (likely causes to investigate)
- The migration was done as a bulk rewrite of all 34 files simultaneously, without incremental testing of each subsystem.
- BLE stack initialization, advertising parameters, and GATT service registration may not have been correctly translated from the SoftDevice model to Zephyr's BLE host.
- GPIO, SPI, I2C, and PWM peripheral initialization via Devicetree may have had subtle pin-mapping or configuration errors not caught at compile time.
- Interrupt-driven peripherals (zero-cross detection, SSR timing) may require different handling under Zephyr's interrupt model vs. the nRF5 SDK's GPIOTE/PPI approach.
- Timer-based scheduling (app_timer in nRF5 SDK) was replaced with Zephyr k_timer — timing behavior may differ.

### What HAS WORKED (according to the user)

> **The only successful migration was performed using NCS v2.0.0 and NCS v2.0.1.**
> The user reports that a previous AI agent session successfully migrated the project
> to these early NCS versions and the firmware worked correctly on hardware.

**Recommended approach:** Start with **NCS v2.0.0 or v2.0.1** where a working migration
is known to exist. If a newer NCS version is required, migrate incrementally from
v2.0.x → v2.1.x → ... following Nordic's official migration guides for each version step.

---

## Workspace State (as of April 2026)

| Item | Status |
|------|--------|
| `ble_espresso_app/` | ✅ Original nRF5 SDK 17.1.0 source — **untouched** |
| `ble_espresso_app_ncs/` | ❌ Deleted (was NCS v3.2.4 attempt) |
| `C:\ncs\` | ❌ Deleted (SDK + toolchain removed) |
| `MIGRATION.md` | ✅ Preserved — contains full API mapping tables from v3.2.4 attempt |
| `TECHNICAL_SUMMARY.md` | ✅ Preserved — architecture overview |
| `TEST_PLAN.md` | ✅ Preserved |
| `docs/` | ✅ Preserved — architecture diagrams, GATT table, memory map |
| VS Code extensions | ✅ nRF Connect for VS Code + Extension Pack still installed |
| SEGGER J-Link | ✅ Installed at `C:\Program Files\SEGGER\JLink_V876` |

No SDK or toolchain is currently installed. The agent must install one before building.

---

## Strategy for Next Attempt

1. **Install NCS v2.0.0 or v2.0.1** (the versions known to produce working firmware).
   - Use `nrfutil toolchain-manager` or the nRF Connect for VS Code extension to install.
   - Board syntax for NCS v2.0.x: `nrf52dk_nrf52832` (underscore, not slash).

2. **Migrate incrementally** — do NOT rewrite all 34 files at once.
   - Start with a minimal `main.c` that boots, blinks an LED, and prints to RTT/UART.
   - Add BLE advertising + one GATT service, flash, and verify on hardware.
   - Add peripherals one at a time: GPIO → SPI → I2C → PWM → timers.
   - Add application logic last.

3. **Test on hardware after each subsystem** — compilation alone is not sufficient.

4. **Read `MIGRATION.md`** for reference on API mappings (nRF5 SDK → NCS), but verify
   each mapping against the NCS v2.0.x documentation since APIs differ between versions.

5. **Memory budget**: QFAB has only 256 KB FLASH and 32 KB RAM. Monitor usage at each
   step with `west build -t rom_report` and `west build -t ram_report`.