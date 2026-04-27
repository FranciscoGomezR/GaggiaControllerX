/* Host-test stub: bluetooth_drv.h
   Minimal replacement that breaks the circular-include chain and provides
   only the declarations used by application code on the test path.
   Does NOT include any real BLE SDK headers. */
#ifndef BLUETOOTH_DRV_H__
#define BLUETOOTH_DRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* External variables declared here but defined in bluetooth_drv.c (not compiled in tests).
   They are only referenced from main.c paths not under test, so the linker
   will not demand definitions for them. */
extern volatile uint8_t  DataReceived[];
extern volatile uint32_t iTagertTemp;
extern volatile uint8_t  dataLen;
extern volatile uint8_t  flg_BrewCfg;
extern volatile uint8_t  flg_PidCfg;
extern volatile uint8_t  flg_ReadCfg;

/* Function declarations — bodies are either provided by FFF fakes in tests
   or simply never called on the host test path. */
void BLE_bluetooth_init(void);
void advertising_start(bool erase_bonds);
void sleep_mode_enter(void);
void ble_disconnect(void);
void ble_restart_without_whitelist(void);
void ble_update_boilerWaterTemp(float waterTemp);

#ifdef __cplusplus
}
#endif

#endif /* BLUETOOTH_DRV_H__ */
