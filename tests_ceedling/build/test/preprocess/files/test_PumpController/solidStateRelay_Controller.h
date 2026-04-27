// CEEDLING NOTICE: This generated file only to be consumed by CMock

#ifndef _SOLIDSTATERELAY_CONTROLLER_H_ // Ceedling-generated include guard
#define _SOLIDSTATERELAY_CONTROLLER_H_

#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_drv_timer.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

typedef enum {
    SSR_DRV_INIT_OK = 0,
    SSR_DRV_INIT_ERROR,
    SSR_STATE_ENGAGE,
    SSR_STATE_ACTIVE,
    SSR_STATE_OFF
} ssr_status_t;
ssr_status_t fcn_initSSRController_BLEspresso(void);
void fcn_boilerSSR_pwrUpdate( uint16_t outputPower);
void fcn_pumpSSR_pwrUpdate( uint16_t outputPower);

void fcn_SolenoidSSR_On(void);
ssr_status_t get_SolenoidSSR_State(void);
void fcn_SolenoidSSR_Off(void);

extern void isr_ZeroCross_EventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

extern void isr_BoilderSSR_EventHandler(nrf_timer_event_t event_type, void* p_context);
extern void isr_PumpSSR_EventHandler(nrf_timer_event_t event_type, void* p_context);

#endif // _SOLIDSTATERELAY_CONTROLLER_H_
