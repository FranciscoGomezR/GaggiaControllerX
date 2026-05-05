# GaggiaController
## Objective 3: Phase 2/3 and Phase 3 TDD
Continue TDD implementation with Phase 2/3 (remaining application issues) and Phase 3 (Ceedling + CMock migration).

### Phase 2/3 — Remaining Application Issues (Unity + FFF)
Issues deferred from Phase 2, now fully planned in TDD_TESTPLAN.md:
- **H2**: BLE input validation — extracted into new `ProfileValidator.c` (pure-logic, testable on host)
- **H5**: Maximum brew time limit — `MAX_BREW_TICKS` guard in `BLEspressoServices.c`
- **M5**: I-gain boost rapid cycling — revert phase2 recovery before re-boost in `cl_idle`/`prof_idle`
- **M3 fix**: NVM range validation — call `fcn_ValidateAndClampProfile()` in `stgCtrl_ReadUserData()`

New files to create:
- `ble_espresso_app/components/Application/ProfileValidator.c` + `.h`
- `tests/test_profile_validator.c`

Estimated new tests: ~23 (13 H2 + 3 H5 + 3 M5 + 4 M3)

### Phase 3 — Ceedling + CMock Migration
Migrate Phase 2/3 tests to Ceedling + CMock for:
- Auto-generated mocks from headers (replaces manual FFF fakes)
- Strict call ordering via `_Expect()` (fails on out-of-order calls)
- Unexpected call detection (any unmocked call = failure)
- Built-in gcov coverage reports

New directory: `tests_ceedling/` with `project.yml` and ported test files.
Both `tests/` (Makefile + FFF) and `tests_ceedling/` (Ceedling) coexist.

### Reference documents:
- TDD_TESTPLAN.md — full implementation plan (Phases 1–3)
- TEST_PLAN.md — master test strategy (Phases 1–6)
- tests/docs/phase1_test_report.md — Phase 1 results
- tests/docs/phase2_test_report.md — Phase 2 results
## End of Objective 3

## Objective 2: Apply TDD to GaggiaController <COMPLETED>
Phase 1 complete: 32/32 tests, BUG-001 fixed.
Phase 2 complete: 34/34 tests, H1/H3/H4/M1 fixed, M3 documented.
Total: 66/66 tests passing.

TDD plan created and documented in TDD_TESTPLAN.md.
Reference links used:
- https://medium.com/@encodedots/test-driven-development-in-c-complete-guide-to-tdd-with-c-781787935465
- https://snyk.io/articles/claude-skills-embedded-systems-engineers/
- https://www.beningo.com/claude-code-skills-embedded-developers/
## End of Objective 2

---
## Documentation
+   File: gaggiacontroller-build.md
    PAth: \GaggiaController\docs\build
    Description:  Contains build notes and what was fixed to move the project to the latest release of SEGGER embedded studio 8.26c + nRF52832-QFAB. 
    ### Fix 1 — SDK Paths, 
    ### Fix 2 — SEGGER ES 8.26c Compatibility,
    ### Fix 3 — FDS Init Failure (`0x860A = FDS_ERR_NO_PAGES`),
    ### Fix 4 — Debug Build Too Large for 92KB App Window
    - Memory map was corrected to matche MCU nRF52832-QFAB size from 512kB to 256kB.
      FLASH Mem used: 217KB of 256KB = 85%
      RAM Mem used: 37.5KB of 64KB = 59%

---
## Objective 1: Code migration to latest SEGGER release <COMPLETED_DO_NOT_EXECUTE>
### File generated: gaggiacontroller-build.md
This project was build on an older version of SEGGER embedded studio and the goal is to compile it successfuly in new verison from:  Release 8.26c Build 2026021300.61129 and  higher. Additionally, improve Segger embedded studio setup to make it easy to work on this project from two different computers by forcing the user to store the project in the same location/path for each computer but make more easy to manage its location and the location of the SDK via MACROS.
This Project relies in the SDK NRF52 environment: nRF5 SDK version: nRF5_SDK_17.1.0_ddde560
MCU target: nRF52832-QFAB
The project can be successfully build in an older verision of SEGGER embedde studio, which is the other computer. now we need to compile it in this computer.

1. build and compile project in this new version of SEGGER embedded studio: release 8.26c Build 2026021300.61129 
    + fix any broken links or libraries that may changed between releases.
    + Read section: ## Current compilation ERRORS to understand current compilation errors.
    + Do not play with memory size and boundaries

2. Implement a Long-term solution, using  macro to point to the SDK and project, e.g. to make it easy for two engineer working in different computers.
    + Assume current project and SDK location to create MACROS: 
        + Project: C:\WS\NRF\GaggiaController
        + SDK: C:\WS\NRF\nRF5_SDK_17.1.0_ddde560
    + User's suggest to implement this changes that related to SEGGER embedded studio is to modify file: ble_espresso_app_pca10040_s132.emProject, but check and confirm.

3. Read README.md for more information that will support your work toward the completion of objetive 1 and 2
## End of Objective 1