# Building BLEspresso with the nRF Connect VS Code Extension

**Project root:** `C:\WS\NRF\GaggiaController`  
**Target board:** nRF52832 QFAB (256 KB FLASH / 32 KB RAM)  
**SDK:** nRF Connect SDK v3.2.4 / Zephyr 4.2.99  
**Toolchain bundle:** `fd21892d0f` (GCC 12.2.0)

---

## Prerequisites

- VS Code with **nRF Connect for VS Code Extension Pack** installed
- NCS v3.2.4 workspace at `C:\ncs\v3.2.4\`
- Toolchain bundle at `C:\ncs\toolchains\fd21892d0f\`
- SEGGER J-Link Base connected to the target (for flash / debug)

---

## Step 1 — Open the nRF Connect panel

Click the **nRF Connect** icon in the VS Code Activity Bar (left side).  
It shows the Nordic Semiconductor logo.

---

## Step 2 — Add the application

In the **APPLICATIONS** section → click **+ Add existing application** →  
navigate to (relative to project root):

```
ble_espresso_app_ncs\
```

Full path: `C:\WS\NRF\GaggiaController\ble_espresso_app_ncs`

---

## Step 3 — Create a build configuration

Under the app entry, click **No build configurations → Add build configuration**.

| Field | Value |
|-------|-------|
| Board | `nrf52dk/nrf52832` |
| Build directory name | `build_324` |
| Kconfig fragment (prj.conf) | *(auto-detected — leave default)* |
| Everything else | leave at defaults |

Click **Build Configuration**.

---

## Step 4 — Wait for CMake configure + compilation

The extension runs two stages automatically:

1. **CMake configure** — generates the build system under:
   ```
   ble_espresso_app_ncs\build_324\
   ```
2. **Ninja compilation** — compiles all ~242 targets (clean build takes 2–3 min).

Progress is visible in **Output** panel → select **nRF Connect** from the dropdown.

---

## Step 5 — Verify the build result

When compilation finishes, the Output panel shows:

```
Memory region    Used Size   Region Size   %age Used
       FLASH:    196900 B      504 KB       38.15%
         RAM:     30400 B       64 KB       46.39%
```

Both regions fit within the nRF52832 QFAB hardware limits (256 KB FLASH / 32 KB RAM). ✓

The firmware binary is located at:
```
ble_espresso_app_ncs\build_324\ble_espresso_app_ncs\zephyr\zephyr.elf
ble_espresso_app_ncs\build_324\ble_espresso_app_ncs\zephyr\zephyr.hex
```

---

## Step 6 — Flash to target (J-Link connected)

**Option A — VS Code panel:**  
In the nRF Connect panel, under the build configuration → click **Flash**.

**Option B — nRF Connect SDK terminal** (terminal labeled `nRF v3.2.4`):
```powershell
west flash --build-dir ble_espresso_app_ncs\build_324
```

---

## Step 7 — Debug (source-level, optional)

Press **F5** in VS Code.

VS Code loads `.vscode\launch.json` which is pre-configured with:
- Type: `nrf-connect`
- ELF: `ble_espresso_app_ncs\build_324\ble_espresso_app_ncs\zephyr\zephyr.elf`

The J-Link Base connects, flashes the debug build, and opens the source-level
debugger with full Zephyr thread list support (`CONFIG_DEBUG_THREAD_INFO=y`).

---

## Incremental builds

After editing source files, use the **Build** button in the nRF Connect panel  
or press `Ctrl+Shift+B`. Only changed files recompile (~10–30 s).
