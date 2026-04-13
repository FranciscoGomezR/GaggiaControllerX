# GaggiaController
## Objective 2: Apply TDD to GaggiaController
I want to explore how to apply TDD (Test Driven Development) in this project that it is alsmot completed. However, I want to explore how can you create small tests to for bug detection, better modularity, refactoring safety, test's document, regression prevention.

Create a plan on how would you implement/apply TDD to this project and present it to me.
Use these links as starting point to understand what you need to implement:
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