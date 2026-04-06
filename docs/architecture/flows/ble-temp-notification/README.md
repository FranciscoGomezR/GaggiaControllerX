# Flow: BLE Temperature Notifications

## Overview

This flow documents how boiler temperature data travels from the MAX31865 RTD sensor to a connected BLE client (mobile app) as a GATT notification.

## Trigger

The main scheduler sets `tf_GetBoilerTemp` every ~100 ms and `tf_ble_update` every 1000 ms.

## Data Path

1. **SPI Read (100 ms)** — `main.c` scheduler calls `spim_ReadRTDconverter()`
   - SPI transaction reads RTD ADC registers from MAX31865
   - Raw ADC → resistance → temperature via Callendar–Van Dusen quadratic
   - Result stored in private `rtdTemperature` (inside `spi_Devices.c`)
2. **Store in Profile** — `main.c` calls `f_getBoilerTemperature()` and writes to `blEspressoProfile.temp_Boiler`
3. **BLE Notification (1000 ms)** — `main.c` calls `ble_update_boilerWaterTemp(blEspressoProfile.temp_Boiler)`
   - Float → integer (×10) → ASCII 4-char array
   - `ble_cus_BoilerWaterTemperature_update()` sends GATT notification on char `0x1402`
4. **Mobile App** — receives notification, displays temperature

## Key Variables

| Variable | Module | Type | Description |
|---|---|---|---|
| `rtdTemperature` | spi_Devices.c | `float` (private) | Last computed boiler temp |
| `blEspressoProfile.temp_Boiler` | BLEspressoServices | `float` | Shared profile copy |
| Char `0x1402` | ble_cus | GATT | BLE notification characteristic |

## Timing

- SPI read: every ~100 ms (non-blocking state machine)
- BLE notification: every 1000 ms

## Diagram

See [ble-temp-notification_diagram.mermaid](ble-temp-notification_diagram.mermaid).
