# Changelog

All notable changes to GaggiaControllerX are logged here, newest first.

Format: one dated section per change set. Each entry states **what** changed,
**why**, and any **migration / action required** (flash erase, app update, etc.).
Keep it terse. Link source files with repo-relative paths.

Categories: `Added` `Changed` `Removed` `Fixed` `Docs` `Breaking`.

---

## 2026-09-05 — Unify config data-source selection (STATUS TODO #10)

### Changed
- `main.c`: the old three overlapping switches (`SET_TEST_USERDATA_EN`,
  `EXCLUDE_NVM_SECTION`, and the dead `LOAD_USERDATA_FROM_NVM_EN`) are
  replaced by one `#if/#elif/#elif` on `ESPRESSO_CFG_DATA_SOURCE`
  (`espressoMachineServices.h`): `FACTORY_DEFAULT_NO_NVM` and
  `TEST_VALUES_NO_NVM` inline-assign `g_Espresso_user_config_s` from the
  matching `#define` table and set `user_data_loaded_flag` directly, with no
  NVM code compiled in for either. `USER_DATA_NVM` keeps `storage_init()` /
  the erase one-shot / `storage_has_user_config()` / TODO #9's provisioning —
  now the only branch touching `spi_Devices`/`StorageController`.
- Pump-parameter and PID-config pushes into `PumpController`/`tempController`
  (previously dead — gated on the no-longer-defined
  `LOAD_USERDATA_FROM_NVM_EN`) now run off one runtime
  `user_data_loaded_flag == STORAGE_USERDATA_LOADED` check, uniform across
  all 3 sources. Fixes the regression flagged in TODO #9's follow-up note.

### Fixed
- `espressoMachineServices.h`: stray trailing `;` on `PID_P_TERM_TEST`
  removed — broke inline use of the macro in an expression.

### Removed
- Dead defines: `SET_TEST_USERDATA_EN`, main.c-local `EXCLUDE_NVM_SECTION`,
  and all remaining `LOAD_USERDATA_FROM_NVM_EN` references.

## 2026-09-05 — Factory-default NVM auto-provisioning (STATUS TODO #9)

### Added
- `main.c`: when `storage_has_user_config()` returns `STORAGE_USERDATA_EMPTY`,
  `g_Espresso_user_config_s` is populated from the `*_FACTORY_DEFAULT_*`
  constant table (`espressoMachineServices.h`) and persisted via
  `storage_save_shot_profile()` then `storage_save_controller_config()`, in
  that order — the first call writes the NVM key on its first-write path, the
  second must run after it so it preserves the just-written real bytes
  instead of erased-flash (`0xFF`) filler for the other half of the record.
  Finishes with `storage_load_user_config()` to sync `nvmKey`/`nvmWcycles`
  bookkeeping and run the existing `validate_clamp_data()` pass.
- No new `StorageController`/`spi_Devices` functions added — built entirely
  from functions that already existed, per the TODO's constraint.

### Follow-up (not yet done, tracked as STATUS TODO #10)
- `main.c` lines guarded by `LOAD_USERDATA_FROM_NVM_EN` (pushing pump
  parameters / PID gains into `PumpController`/`tempController` at boot)
  reference a macro no longer defined in `espressoMachineServices.h` — it was
  replaced by `ESPRESSO_CFG_DATA_SOURCE` /
  `FACTORY_DEFAULT_NO_NVM`/`TEST_VALUES_NO_NVM`/`USER_DATA_NVM` ahead of TODO
  #10. An undefined macro in `#if` evaluates to 0, so those blocks currently
  never compile in on either branch — pump/PID config is not reaching the
  controllers at boot regardless of data source. Pre-existing on disk, not
  introduced by this change; left for TODO #10's selector rewire.

## 2026-09-04 — NVM param erase capability (STATUS TODO #8)

### Added
- `spi_Devices.c`/`.h`: public wrapper `spi_NVMemoryErasePage(uint32_t page)`
  around the existing (now `static`) `spi_NVMemoryEraseSector()` — converts a
  page number to its containing 4KB sector, so callers stay in page terms
  like `spi_NVMemoryRead()` / `spi_NVMemoryWritePage()` already do.
- `StorageController.c`/`.h`: `storage_erase_user_config()` — erases the NVM
  param sector (`NVM_PARAM_MEM_KEY` + the rest of `espresso_user_config_t`)
  via `spi_NVMemoryErasePage(NVM_PARAM_PAGE_ADD)`. New
  `STORAGE_USERDATA_ERASED` status value.
- `espressoMachineServices.h`: `#define ESPRESSO_CFG_ERASE_NVM_KEY 0`
  compile-time one-shot trigger (set 1, flash, boot once, set back to 0).
- `main.c`: erase call wired into the `EXCLUDE_NVM_SECTION == 0` init block,
  right after `storage_init()` and before `storage_has_user_config()`, so the
  same boot session reads back `STORAGE_USERDATA_EMPTY` post-erase.

### Changed
- `spi_NVMemoryEraseSector()` changed from an unexposed module-global to a
  `static` (private) function — no longer directly callable from outside
  `spi_Devices.c`; use `spi_NVMemoryErasePage()` instead.

### Docs
- `NVM_PARAM_START_ADDR` / `NVM_PARAM_END_ADDR` in `StorageController.c`
  confirmed unreferenced anywhere in the codebase (0 bytes flash/RAM cost as
  plain `#define`s) — left in place per developer instruction, not wired to
  the erase path.

## 2026-09-04 — Centralize `validate_float_in_range()` limits (STATUS TODO #7)

### Changed
- `ble_espresso_app/components/Application/espressoMachineServices.h`: added a
  `MIN` / `MAX` / `DEFAULT` `#define` triplet per `espresso_user_config_t`
  float field (temperature setpoints, brew profile power/timers, PID gains).
- `StorageController.c` `validate_clamp_data()` and `bluetooth_drv.c` BLE RX
  event handlers (22 `validate_float_in_range()` calls total) now reference
  these `#define`s instead of hardcoding their own literals per call site.
  `StorageController.c` values kept as the canonical source.

### Fixed
- The two call sites had drifted apart for the same field. Corrected on the
  BLE write path (`bluetooth_drv.c`) to match `StorageController.c`:
  `boilerTempSetpointDegC` (default 93.0 -> 95.5), `brewTempDegC`
  (10.0–99.5/95.0 -> 20.0–110.0/95.0), `steamTempDegC`
  (99.5–140.0/125.0 -> 100.0–160.0/110.0), `profPreInfusePwr` (default
  50.0 -> 75.0), `profPreInfuseTmr` (default 3.0 -> 8.0), `profInfuseTmr`
  (default 25.0 -> 8.0), `profTaperingPwr` (default 60.0 -> 85.0).

### Follow-up (not yet done)
- `profTaperingTmr` and (previously) `brewTempDegC` had no BLE-write-time
  clamp at all — only `StorageController.c` clamps them. Not added here;
  scope was collapsing existing duplicated calls onto one source, not adding
  new validation call sites.

## 2026-09-04 — Fix 0x1405 steam temp write dead branch (STATUS TODO #6)

### Fixed
- `on_write()` in `ble_espresso_app/components/BLE_Services/ble_cus.c`: the
  `0x1405` (steam preset temp) branch tested
  `p_cus->brew_temp_char_handles.value_handle` — a copy-paste leftover from the
  `0x1404` brew branch immediately above it — instead of
  `p_cus->steam_temp_char_handles.value_handle`. Since the branches are an
  `else if` chain, every write to `0x1405` fell through unmatched:
  `BLE_MACHINE_STEAM_TEMP_CHAR_RX_EVT` never fired, no log, no app response.
  Matches the previously noted bug in the 2026-08-31 GATT doc resync entry.
- Corrected the handle compare to `steam_temp_char_handles.value_handle`.
- Verified downstream `BLE_MACHINE_STEAM_TEMP_CHAR_RX_EVT` case in
  `ble_espresso_app/components/BLE/bluetooth_drv.c` already writes
  `steamTempDegC` correctly — no change needed there.
- Confirmed fixed on hardware (0x1405 write now logs and applies).

### Follow-up (not yet done)
- `bluetooth_drv.c` `BLE_MACHINE_BREW_TEMP_CHAR_RX_EVT` case calls
  `validate_float_in_range()` on `steamTempDegC` instead of `brewTempDegC`.
- `NRF_LOG_INFO` for both BREW and STEAM preset temp has two `%d` format
  specifiers but only one arg passed — decimal part missing from the log line.

## 2026-08-31 — GATT table doc resync to `ble_cus_init()`

### Docs
- `docs/ble/gatt_table.md` + `.mermaid` regenerated from `ble_cus.c/.h`:
  - Brew Service `0x1400` now 11 chars. New UUID map: `0x1403` boiler setpoint,
    `0x1404` brew preset, `0x1405` steam preset, pre-infusion..declining shifted
    to `0x1406`–`0x140B` (was `0x1404`–`0x140A`, steam at `0x140A`).
  - Both services share `CUSTOM_SERVICE_UUID_BASE`; `CUSTOM_PID_SERVICE_UUID_BASE`
    documented as dead code.
  - Data-hub field names -> `g_Espresso_user_config_s` (`boilerTempDegC`,
    `boilerTempSetpointDegC`, `brew/steamTempDegC`, `prof*`, `pid*Term`).
  - Helper names corrected: `ble_cus_espresso_char_add()` /
    `ble_cus_controller_char_add()` / `characteristic_add()`.
  - Defaults column from `StorageController.c` `validate_float_in_range()`.
  - Noted `on_write()` bug: `0x1405` steam branch tests `brew_temp_char_handles`.
- `docs/ble/gatt_table.svg` is now stale — regenerate from `.mermaid`.

## 2026-08-30 — NVM temp slots match struct (STATUS TODO #4)

### Changed
- `StorageController.c` NVM param map: `0x08` now stores `brewTempDegC`,
  `0x0C` now stores `steamTempDegC` (was the volatile working value
  `boilerTempSetpointDegC` at `0x08` and an unused reserved word at `0x0C`).
- Defines renamed: `USERDATA_TARGETBOILER_TMP` -> `USERDATA_BREW_TEMP`,
  `USERDATA_RSVD` -> `USERDATA_STEAM_TEMP`.
- `storage_load_user_config()` deserializes both presets; `storage_save_*()`
  serializes both presets.

### Migration
- In-field NVM param page must be erased — `0x0C` changes from `0x00000000`
  to a real float; `0x08` semantics changed.

### Docs
- `docs/ext_memory/external_mem.md` + `.mermaid`.

## 2026-08-30 — `storage_print_user_config()` refresh (STATUS TODO #5)

### Changed
- Boiler lines now print `brewTempDegC` ("Brew Temp") and `steamTempDegC`
  ("Steam Temp") — was the non-persisted `boilerTempSetpointDegC` plus a
  mislabelled steam line (STATUS TODO #4 follow-up).
- "Integral Windup" line already gone (TODO #3); no D LPF / Gain lines
  (TODO #1). PID block prints P / I / Imax / D / P boost / I boost.
- `degC` unit suffix; fixed `%%` on the "Declining Power" line.
- Comments converted to C89 `/* */` per CODE CONVENTION.

## 2026-08-30 — Drop `pidIwindupTerm`; freeze NVM key

### Removed
- Struct field `pidIwindupTerm` from `espresso_user_config_t`
  (`ble_espresso_app/components/Application/espressoMachineServices.h`).
- All related logic: NVM serialize/deserialize + `USERDATA_PID_IWINDUPTERM`
  (`0x40`) and the "Integral Windup" print line in
  `StorageController.c`; the `main.c` test-data default.
- `tempController.c` `temp_ctrl_set_pid_config()` now sets
  `isIAntiwindupEnabled = true` unconditionally when the I term is active
  (matches `temp_ctrl_init()` default). Anti-windup is no longer user-toggled.

### Changed
- NVM param page: `0x40` is now unused. Byte-map compaction and
  `NVM_PARAM_*_SIZE` review tracked under STATUS.md TODO item 4.

### Docs
- `NVM_PARAM_MEM_KEY` marked **FIXED / immutable** in `StorageController.c`
  and NVM docs — value must not be changed by a human or an AI agent.

## 2026-08-29 — PID Service characteristic rework

### Breaking
- **NVM key bumped** `0x00AA00AA` -> `0x00AB00AB`
  (`ble_espresso_app/components/Application/StorageController.c`).
  Reason: flash slots `0x38`/`0x3C` changed meaning. Already-provisioned units
  read as *empty* on boot (RAM defaults applied) and will **not re-persist** until
  the NVM parameter page is erased.
- **BLE PID Service (`0x1500`) layout changed** — mobile apps built against the old
  table must be updated.

### Removed
- BLE char `0x1504` I Windup, `0x1506` D LPF, `0x1507` Gain.
- Struct fields `pidDlpfTerm`, `pidGainTerm` (were dead — never fed to
  `pid_imc_compute()`).
- Enum values `PID_I_TERM_WINDUP_CHAR_RX_EVT`, `PID_D_TERM_LPF_CHAR_RX_EVT`,
  `PID_GAIN___CHAR_RX_EVT`; UUID defines `BLE_CHAR_PID_I_TERM_WINDUP_UUID`,
  `BLE_CHAR_PID_D_TERM_LPF_UUID`, `BLE_CHAR_PID_GAIN___UUID`.

### Changed
- D Term BLE char moved `0x1505` -> `0x1504`.
- `pidIwindupTerm` kept in struct + NVM (`0x40`) — still drives
  `isIAntiwindupEnabled` in `tempController.c` — but no longer has a BLE char;
  configurable via NVM / `main.c` default only.

### Added
- BLE char `0x1505` P Boost -> `pidPboostTerm` (range 0–10, default 1.0).
- BLE char `0x1506` I Boost -> `pidIboostTerm` (range 0–20, default 6.5).
  `pidIboostTerm` already existed and is consumed by
  `temp_ctrl_set_operational_integral_gain()`; it is now settable over BLE and
  persisted to NVM.
- NVM slots repurposed: `0x38` = `pidPboostTerm`, `0x3C` = `pidIboostTerm`.

### Follow-up (not yet done)
- `pidPboostTerm` has **no consumer** — `tempController` phase-1 setup still uses
  hardcoded literals (`kp = 0.0f`, `ki * 6.5f`). Wire `pidPboostTerm` /
  `pidIboostTerm` into `temp_ctrl_set_operational_integral_gain()` / phase-1 init.
- `STATUS.md` TODO: widen I Term / P Boost / I Boost wire length 3 B -> 4 B.
- Add a "stale key -> treat as first write" path in `storage_save_*` if in-field
  NVM migration is needed.

### Docs
- Updated: `docs/ble/gatt_table.md` + `.mermaid`,
  `docs/ext_memory/external_mem.md` + `.mermaid`,
  `docs/architecture/modules/modules.md` + data-hub diagram,
  `docs/architecture/ArchitectureAnalysis.md`,
  `docs/architecture/flows/ble-to-profile/` + `nvm-persistence/`,
  `docs/code_inventory/module_inventory.md`,
  `docs/instructions/copilot-memory-instructions.md`,
  `docs/instructions/EspressoMachineServices_ExtractionTime_Plan.md`,
  `docs/test_plan_docs/TDD_TESTPLAN.md`.
