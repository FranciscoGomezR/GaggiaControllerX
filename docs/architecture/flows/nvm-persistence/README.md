# Flow: NVM Persistence

## Overview

Documents the bidirectional data flow between `blEspressoProfile` and the W25Q64 external flash memory: boot-time load and BLE-triggered store.

## Boot Load Path (NVM → blEspressoProfile)

1. `stgCtrl_Init()` — reset NVM, read JEDEC ID, verify manufacturer
2. `stgCtrl_ChkForUserData()` — read bytes `0x04–0x07`, check for magic key `0x00AA00AA`
3. `stgCtrl_ReadUserData(&blEspressoProfile)` — read 65 bytes from Page 0, parse little-endian floats into struct fields
4. `fcn_LoadNewPumpParameters(&blEspressoProfile)` — propagate brew profile to PumpController
5. `fcn_loadPID_ParamToCtrl_Temp(&blEspressoProfile)` — propagate PID gains to tempController

## Store Path (blEspressoProfile → NVM)

### Shot Profile Store (triggered by `flg_BrewCfg = 1`)

1. Read current 65 bytes from NVM
2. Preserve Controller section (`0x28–0x40`)
3. Encode updated brew profile floats → bytes (`0x08–0x27`)
4. Increment shot profile write counter in `nvmWcycles` MSW
5. Sector erase + page write

### Controller Store (triggered by `flg_PidCfg = 1`)

1. Read current 65 bytes from NVM
2. Preserve Shot Profile section (`0x08–0x27`)
3. Encode updated PID floats → bytes (`0x28–0x40`)
4. Increment controller write counter in `nvmWcycles` LSW
5. Sector erase + page write

## Key Design Decision

Shot profile and controller data are stored independently — writing one section preserves the other via read-modify-write. This minimizes flash wear.

## Diagram

See [nvm-persistence_diagram.mermaid](nvm-persistence_diagram.mermaid).
