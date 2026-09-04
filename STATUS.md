# STATUS Documents
This document tracks ToDo list on gaggia_controller_git, known bugs and failed attempts that developer shall not push again.
Everyt time a developer or AID agent works on an item in this list shall update STATUS.md and CHANGELOG.md doc (Link TODO item to its log entry inside CHANGELOG.md for good tracebility)

# TODO LIST
- 1[x]Update PID controller parameter from: 
        | UUID | Name | Char Declaration | Char Value | CCCD | CUDD | Properties | Val Len | Default |
        |---|---|---|---|---|---|---|---|---|
        | `0x1501` | P Term | S2+1 | S2+2 | — | — | R, W | 4 B | From NVM / `Pid_P_term` (9.52156) |
        | `0x1502` | I Term | S2+3 | S2+4 | — | — | R, W | 3 B | From NVM / `Pid_I_term` (0.3) |
        | `0x1503` | I Max | S2+5 | S2+6 | — | — | R, W | 4 B | From NVM / `Pid_Imax_term` (100.0) |
        | `0x1504` | I Windup | S2+7 | S2+8 | — | — | R, W | 1 B | `'0'` (disabled) |
        | `0x1505` | D Term | S2+9 | S2+10 | — | — | R, W | 3 B | From NVM / `Pid_D_term` (0.0) |
        | `0x1506` | D LPF | S2+11 | S2+12 | — | — | R, W | 4 B | From NVM / `Pid_Dlpf_term` |
        | `0x1507` | Gain | S2+13 | S2+14 | — | — | R, W | 4 B | From NVM / `Pid_Gain_term` |
        to
        | UUID | Name | Char Declaration | Char Value | CCCD | CUDD | Properties | Val Len | Default |
        |---|---|---|---|---|---|---|---|---|
        | `0x1501` | P Term | S2+1 | S2+2 | — | — | R, W | 4 B | From NVM / `pid_Pterm` (9.52156) |
        | `0x1502` | I Term | S2+3 | S2+4 | — | — | R, W | 3 B | From NVM / `pid_Iterm` (0.3) |
        | `0x1503` | I Max | S2+5 | S2+6 | — | — | R, W | 4 B | From NVM / `pid_Imaxterm` (100.0) |
        | `0x1504` | D Term | S2+7 | S2+8 | — | — | R, W | 3 B | From NVM / `pid_Dterm` (0.0) |
        | `0x1505` | P Boost | S2+9 | S2+10 | — | — | R, W | 3 B | From NVM / `pid_Pboost` (1.0) |
        | `0x1506` | I Boost | S2+11 | S2+12 | — | — | R, W | 3 B | From NVM / `pid_Iboost` (6.5) |
        remove UUID: 0x1507
        also update UUID define in ble_cus.h e.g.: BLE_CHAR_PID_P_TERM_UUID
        affected modules: ble_cus.h/ble_cus.c, ble_cus_init(), ble_cus_controller_char_add() and any other part of the code affected by this change
        identify if current logic/code is impacted by this change and suggest corrections

- 2[x]update documentation and comment's inside code that NVM_PARAM_MEM_KEY is fix and can not be modified by human or AI agent.
        Done: immutable note added at NVM_PARAM_MEM_KEY define in StorageController.c and NVM docs.
        NOTE: code value is 0x00AA00AA; CHANGELOG/ext_memory docs claim 0x00AB00AB - unresolved mismatch, key value left untouched.

- 3[x]remove pidIwindupTerm from espresso_user_config_t struct and any logic inside the project related to this element.
        Done: struct field, NVM serialize/deserialize (0x40 slot + USERDATA_PID_IWINDUPTERM), print line, main.c default removed.
        tempController temp_ctrl_set_pid_config() now forces isIAntiwindupEnabled = true when I term active.

- 4[x]update "StorageController.h" logic to match espresso_user_config_t struct data:
        struct element brewTempDegC is linked to 0x08 add -> change #define USERDATA_TARGETBOILER_TMP to USERDATA_BREW_TEMP
        struct element steamTempDegC is linked to 0x0C add -> change #define USERDATA_RSVD to USERDATA_STEAM_TEMP
        Done: defines live in StorageController.c (not .h). Renamed + load/save now (de)serialize brewTempDegC@0x08 and steamTempDegC@0x0C.
        CHANGELOG.md -> "2026-08-30 — NVM temp slots match struct (STATUS TODO #4)".
        NOTE: storage_print_user_config() still prints boilerTempSetpointDegC -> left for TODO #5. NVM page erase required in-field.

- 5[x]update storage_print_user_config() function based on changes from TODO item 3 and 4.
        Done: prints brewTempDegC/steamTempDegC (was boilerTempSetpointDegC + mislabelled steam line),
        no Integral Windup line, PID block = P/I/Imax/D/Pboost/Iboost, C89 comments, degC unit, %% fix.
        CHANGELOG.md -> "2026-08-30 — storage_print_user_config() refresh (STATUS TODO #5)".

- 6[x] Mobile App tries to write to BLE_CHAR: 0x1405 no response from 'bluetooth_drv' module 
        WRITE BLE CHAR-> 0x1404
        Serial Log:
                <debug> app: Event 5
                <debug> app: BLE -> New BREW Temperature
                <info> app:  BREW Preset Temp: 95 . 0 
        WRITE BLE CHAR-> 0x1405
        Serial Log:
                'Nothing'
        WRITE BLE CHAR-> 0x1406
        Serial Log:
                <debug> app: Event 7
                <info> app:  PreInfusion POWER: 71 
        WRITE BLE CHAR-> 0x1407
        Serial Log:
                <debug> app: Event 8
                <info> app:  PreInfusion TIME: 8 
        Done: root cause -> on_write() STEAM branch in ble_cus.c Confirmed fixed on hardware.
        CHANGELOG.md -> "2026-09-04 — Fix 0x1405 steam temp write dead branch (STATUS TODO #6)".

- 7[x]validate_float_in_range() min/max/safeDefault were hardcoded. Done: added #define MIN/MAX/DEFAULT table in espressoMachineServices.h
        CHANGELOG.md -> "2026-09-04 — Centralize validate_float_in_range() limits (STATUS TODO #7)".
        Note: profTaperingTmr (and previously brewTempDegC) have no BLE-write-time clamp at all — only StorageController.c validates them. Left as-is, flagged as follow-up — say if you want that added too.

- [ ]add a function inside "StorageController.h" that allows developer to erease the NVM_PARAM_MEM_KEY and the rest of the parameter from the NVM chip. Expose a wraper inside spi_devices.h module (use code under comment: "erase the entire sector: 4KB"). Then add the wrapper in StorageController module. As trigger, add a compile-time one-shot in espressoMachineServices.h with a macro #define ESPRESSO_CFG_ERASE_NVM_KEY   0   /* set 1, flash, boot once, set back to 0 */ 

- [ ]add a function inside "StorageController.h" that allows developer to load default values into the g_Espresso_user_config_s var (using method: init_result_flag = storage_has_user_config()& checking STORAGE_USERDATA_EMPTY) from three different (Tables or NVM) sources, first: FACTORY_DEFAULT_VALUES (from #defines), second: TEST_VALUES (from #defines) and third one is: normal flow logic, load values from NVM after every start. We shall add a new #define table for max. and min. values used by validate_clamp_data().
        -Propse a solution/logic that allows the developer using #defines in ESPRESSOMACHINESERVICES_H__ to select between three values sources. Implement easy logic and keep an eye on the FLASH memory footprint and any possible code that may break logic flow.
        -Propose a location to move, add the code and #defines into the best module to achieve the goal.
        Goal: control source data  loaded into g_Espresso_user_config_s. Control method via compile-time. 
        
        Comment: If source selected is not NVM then exclude from compilation any code that interact with reading the NVM.

- [ ]change PID controller parameter Length mentioned below: 
        | UUID | Name | Char Declaration | Char Value | CCCD | CUDD | Properties | Val Len | Default |
        |---|---|---|---|---|---|---|---|---|
        | `0x1502` | I Term | S2+3 | S2+4 | — | — | R, W | 3 B | From NVM / `pid_Iterm` (0.3) |
        | `0x1505` | P Boost | S2+9 | S2+10 | — | — | R, W | 3 B | From NVM / `pid_Pboost` (1.0) |
        | `0x1506` | I Boost | S2+11 | S2+12 | — | — | R, W | 3 B | From NVM / `pid_Iboost` (6.5) |
        to
        | UUID | Name | Char Declaration | Char Value | CCCD | CUDD | Properties | Val Len | Default |
        |---|---|---|---|---|---|---|---|---|
        | `0x1502` | I Term | S2+3 | S2+4 | — | — | R, W | 4 B | From NVM / `pid_Iterm` (0.3) |
        | `0x1505` | P Boost | S2+9 | S2+10 | — | — | R, W | 4 B | From NVM / `pid_Pboost` (1.0) |
        | `0x1506` | I Boost | S2+11 | S2+12 | — | — | R, W | 4 B | From NVM / `pid_Iboost` (6.5) |
        affected modules: ble_cus.h/ble_cus.c, ble_cus_init(), ble_cus_controller_char_add()
- [ ]Update serial data from: 
        -> "%s;Time_Miliseconds;Heating_Power;Boiler_Target_DegC;Boiler_Temp_DegC;Pump_Power"
        To
        -> "%s;System_Time_Miliseconds;Brew_time;Boiler_Target_DegC;Boiler_Temp_DegC;Heating_Power;Pump_Power",


# Development IDE version updates
Controller to automate Gaggia expresso machine

This Project relies in the SDK NRF52 environment.
nRF5 SDK version: nRF5_SDK_17.1.0_ddde560

This project was based on the follwing youtube video: https://www.youtube.com/watch?v=8drz4rDqswo&t=20s&ab_channel=nrf5dev


Trying to compile project in a SEGGER Embedded Studio -> Release 8.26c Build 2026021300.61129 or higher?
---------------------------------------------------------------------------------------------------------------------------------------------------------
Then you find the following problems with 
- SEGGER_RTT.c related and msg error: "__vfprintf.h: No such file or directory"

Inside flash_placement.xml, replace following lines:
 -     <ProgramSection alignment="4" load="Yes" name=".text" size="0x4" />  to     <ProgramSection alignment="4" load="Yes" name=".text" />
 -     <ProgramSection alignment="4" load="Yes" name=".rodata" size="0x4" />  to     <ProgramSection alignment="4" load="Yes" name=".rodata" />
Forum fix soruce:
- https://devzone.nordicsemi.com/f/nordic-q-a/89236/build-error
- https://devzone.nordicsemi.com/f/nordic-q-a/94689/stuck-at-building-blinky-example-text-is-larger-than-specified-size 

Then: remove the SEGGER_RTT_Syscalls_SES.c from project
Forum fix soruce: 
- https://devzone.nordicsemi.com/f/nordic-q-a/85919/__vfprintf-h-no-such-file-or-directory 

Then: Under 'Solution->Options->Common->Private Configurations->Library' changed 'Library I/O' to 'RTT'
Forum fix soruce:  
- https://devzone.nordicsemi.com/f/nordic-q-a/105785/library-heap-not-functional-when-migrating-from-ses-5-70a-to-ses-7-32

Trying to compile project from a new location?
Long-term solution, use macro to poin to the SDK, e.g.
Got to Solution/Project → Edit Options (Common) → Build → Project Macros
- SDK_ROOT = C:/WS/NRF/nRF5_SDK_17.1.0_ddde560
Make sure your project.emProject file also reflect this change
- $(SDK_ROOT)/components/
- etc

Forum fix soruce:  
- https://devzone.nordicsemi.com/f/nordic-q-a/62122/best-practices-for-starting-a-new-project-in-ses-based-on-example-application/253302#253302

