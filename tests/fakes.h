/* =============================================================================
 * tests/fakes.h — FFF hardware-layer fake declarations for Phase 2
 *
 * Include this file from EXACTLY ONE .c file per test executable.
 * It defines DEFINE_FFF_GLOBALS and all hardware-driver fakes that are
 * shared across test modules.
 *
 * Application-layer fakes (tempController functions) are intentionally NOT
 * declared here, because test_temp_controller.c compiles the real
 * tempController.c. Application tests that need tempCtrl fakes declare them
 * inline in their own .c file AFTER including this header.
 * =============================================================================*/
#ifndef FAKES_H__
#define FAKES_H__

#include "fff.h"

/* Pull in the types used by the fake function signatures */
#include <stdint.h>
#include <stdbool.h>
#include "solidStateRelay_Controller.h"
#include "spi_Devices.h"
#include "ac_inputs_drv.h"

/* ---------------------------------------------------------------------------
 * One-time FFF global state (call counters, history arrays).
 * Expands to: unsigned int fff_call_history_idx; fff_call_t fff_call_history[];
 * --------------------------------------------------------------------------- */
DEFINE_FFF_GLOBALS;

/* ---------------------------------------------------------------------------
 * solidStateRelay_Controller fakes
 * --------------------------------------------------------------------------- */
FAKE_VOID_FUNC(fcn_boilerSSR_pwrUpdate, uint16_t);
FAKE_VOID_FUNC(fcn_pumpSSR_pwrUpdate,   uint16_t);
FAKE_VOID_FUNC(fcn_SolenoidSSR_On);
FAKE_VOID_FUNC(fcn_SolenoidSSR_Off);
FAKE_VALUE_FUNC(ssr_status_t, get_SolenoidSSR_State);
FAKE_VALUE_FUNC(ssr_status_t, fcn_initSSRController_BLEspresso);

/* ---------------------------------------------------------------------------
 * spi_Devices fakes (RTD temperature sensor + NVM)
 * --------------------------------------------------------------------------- */
FAKE_VALUE_FUNC(float,           f_getBoilerTemperature);
FAKE_VOID_FUNC (spim_ReadRTDconverter);
FAKE_VALUE_FUNC(bool,            spim_operation_done);
FAKE_VOID_FUNC (spim_init);
FAKE_VALUE_FUNC(spi_Tmp_status_t, spim_initRTDconverter);
FAKE_VALUE_FUNC(spi_nvm_status_t, spim_initNVmemory);
FAKE_VOID_FUNC (spi_NVMemoryRead,      uint32_t, uint8_t, uint32_t, uint8_t *);
FAKE_VOID_FUNC (spi_NVMemoryWritePage, uint32_t, uint8_t, uint32_t, uint8_t *);
FAKE_VOID_FUNC (spim_DevCommMng);

/* ---------------------------------------------------------------------------
 * ac_inputs_drv fakes
 * --------------------------------------------------------------------------- */
FAKE_VALUE_FUNC(acInput_status_t, fcn_GetInputStatus_Brew);
FAKE_VALUE_FUNC(acInput_status_t, fcn_GetInputStatus_Steam);
FAKE_VOID_FUNC (fcn_SenseACinputs_Sixty_ms);
FAKE_VALUE_FUNC(acInput_status_t, fcn_initACinput_drv);

/* ---------------------------------------------------------------------------
 * bluetooth_drv fakes
 * --------------------------------------------------------------------------- */
FAKE_VOID_FUNC(ble_update_boilerWaterTemp, float);

/* ---------------------------------------------------------------------------
 * Macro: reset all hardware-layer fakes between tests (call from setUp)
 * --------------------------------------------------------------------------- */
#define RESET_ALL_HW_FAKES() do { \
    RESET_FAKE(fcn_boilerSSR_pwrUpdate);        \
    RESET_FAKE(fcn_pumpSSR_pwrUpdate);          \
    RESET_FAKE(fcn_SolenoidSSR_On);             \
    RESET_FAKE(fcn_SolenoidSSR_Off);            \
    RESET_FAKE(get_SolenoidSSR_State);          \
    RESET_FAKE(fcn_initSSRController_BLEspresso);\
    RESET_FAKE(f_getBoilerTemperature);         \
    RESET_FAKE(spim_ReadRTDconverter);          \
    RESET_FAKE(spim_operation_done);            \
    RESET_FAKE(spim_init);                      \
    RESET_FAKE(spim_initRTDconverter);          \
    RESET_FAKE(spim_initNVmemory);              \
    RESET_FAKE(spi_NVMemoryRead);               \
    RESET_FAKE(spi_NVMemoryWritePage);          \
    RESET_FAKE(spim_DevCommMng);                \
    RESET_FAKE(fcn_GetInputStatus_Brew);        \
    RESET_FAKE(fcn_GetInputStatus_Steam);       \
    RESET_FAKE(fcn_SenseACinputs_Sixty_ms);     \
    RESET_FAKE(fcn_initACinput_drv);            \
    RESET_FAKE(ble_update_boilerWaterTemp);     \
    FFF_RESET_HISTORY();                        \
} while(0)

#endif /* FAKES_H__ */
