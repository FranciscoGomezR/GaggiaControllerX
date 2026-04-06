# Data Flow Index — BLEspresso Controller

This index catalogs all documented data flows in the BLEspresso embedded system. Each flow is stored in its own folder with a `README.md` and a `[flow-name]_diagram.mermaid`.

## Flows

| # | Flow Name | Source | Sink | Path |
|---|---|---|---|---|
| 1 | [BLE Notifications — Temperature](ble-temp-notification/) | SPI MAX31865 → `blEspressoProfile` | BLE Notification (char `0x1402`) | `flows/ble-temp-notification/` |
| 2 | [BLE → blEspressoProfile](ble-to-profile/) | BLE GATT Write events | `blEspressoProfile` fields | `flows/ble-to-profile/` |
| 3 | [blEspressoProfile → Modules](profile-to-modules/) | `blEspressoProfile` | tempController, PumpController, StorageController | `flows/profile-to-modules/` |
| 4 | [Boiler Temperature Control](boiler-temp-control/) | MAX31865 RTD sensor | SSR Boiler Heater output | `flows/boiler-temp-control/` |
| 5 | [Brew Cycle — Classic Mode](brew-classic/) | AC Brew/Steam switches | Pump SSR + Solenoid SSR + Heater SSR | `flows/brew-classic/` |
| 6 | [Brew Cycle — Profile Mode](brew-profile/) | AC Brew switch | 3-stage Pump SSR ramp | `flows/brew-profile/` |
| 7 | [NVM Persistence](nvm-persistence/) | `blEspressoProfile` ↔ W25Q64 flash | Boot load + BLE-triggered store | `flows/nvm-persistence/` |
