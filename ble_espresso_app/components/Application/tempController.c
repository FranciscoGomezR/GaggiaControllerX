/*******************************************************************************
 *
 *		INCLUDE FILE SECTION FOR THIS MODULE
 *
 ******************************************************************************/
#include "tempController.h"
/*#include "spi_Devices.h"*/
/*#include "solidStateRelay_Controller.h"*/

/*******************************************************************************
 *
 *		PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
 *
 ******************************************************************************/
/*#define TEMP_CTRL_SAMPLING_T    0.01f*/                       /* Sampling Time in seconds */
#define HWTMR_PERIOD_MSECS           1.0f                          /* Period of the TMR is in miliseconds */
#define HWTMR_PERIOD_USECS           (HWTMR_PERIOD_MSECS * 1009.0f)

/*******************************************************************************
 *
 *		PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
 *
 ******************************************************************************/
typedef struct
{
    nrf_drv_timer_t             hwTmr;              /* HW-Timer that will control Relay trigger */
    nrfx_timer_event_handler_t  hwTmr_isr_handler;
    uint32_t                    tmrPeriod_us;
    uint32_t                    tmrPeriod_ticks;
    bool                        is_active;
} hw_timer_t;

/*******************************************************************************
 *
 *		PUBLIC VARIABLES
 *
 ******************************************************************************/


/*******************************************************************************
 *
 *		PRIVATE VARIABLES
 *
 ******************************************************************************/
static pid_imc_block_t Profile_ctrl_main_s;
static pid_imc_block_t Profile_ctrl_phi1_s;
static pid_imc_block_t Profile_ctrl_phi2_s;
static hw_timer_t Hw_Tmr_msecs_s;
static volatile uint32_t elapsed_msecs=0;

/* ---- Test-only accessors -------------------------------------------------
 * These helpers are compiled ONLY when -DTEST is set (host-side unit tests).
 * They are never included in the firmware build.
 * ----------------------------------------------------------------------- */
#ifdef TEST
void     test_set_elapsed_msecs(uint32_t t)  { elapsed_msecs = t; }
uint32_t test_get_elapsed_msecs(void)        { return (uint32_t)elapsed_msecs; }
float    test_get_integral_error(void)    { return Profile_ctrl_main_s.historyError; }
#endif /* TEST */

/*******************************************************************************
 *
 *		PRIVATE FUNCTIONS
 *
 ******************************************************************************/
static void hw_timer_init_msecs_tick(void);

/*******************************************************************************
 *
 *		ISR HANDLERS FUNCTIONS
 *
 ******************************************************************************/
/*****************************************************************************
 * Function: 	temp_ctrl_sampling_timer_event_handler
 * Description: increment counter tick every 1ms
 *****************************************************************************/
void temp_ctrl_sampling_timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    /* GPIO29 controls the LED_HeartBeat - for measurieng purposes. */
    /*nrf_drv_gpiote_out_toggle(29);*/
    elapsed_msecs++;
}

/*******************************************************************************
 *
 *		PUBLIC FUNCTIONS SECTION
 *
 ******************************************************************************/
/*****************************************************************************
 * Function: 	temp_ctrl_init
 * Description: 
 * Return:      N/A
 *****************************************************************************/
tempCtrl_status_t temp_ctrl_init(void)
{
  /* PID PARAMETERS VALUE SETUP */
  /* ----------------------------------------------------------------------------- */
  Profile_ctrl_main_s.isPTermEnabled        = true;
  Profile_ctrl_main_s.kp                    = TEMP_CTRL_KP;
  Profile_ctrl_main_s.isITermEnabled        = true;
  Profile_ctrl_main_s.ki                    = TEMP_CTRL_KI;
  Profile_ctrl_main_s.integralLimit         = TEMP_CTRL_HIST_LIMIT;
  Profile_ctrl_main_s.isIAntiwindupEnabled  = true;
  Profile_ctrl_main_s.isDTermEnabled        = true;
  Profile_ctrl_main_s.kd                    = TEMP_CTRL_KD;

  Profile_ctrl_main_s.outputLimit           = TEMP_CTRL_MAX;

  /* PID PARAMETERS PHASE 1 */
  /* ----------------------------------------------------------------------------- */
  Profile_ctrl_phi1_s.isPTermEnabled        = false;
  Profile_ctrl_phi1_s.kp                    = 0.0f;

  Profile_ctrl_phi1_s.isITermEnabled        = Profile_ctrl_main_s.isITermEnabled;
  Profile_ctrl_phi1_s.ki                    = Profile_ctrl_main_s.ki * 6.5f;
  Profile_ctrl_phi1_s.integralLimit         = Profile_ctrl_main_s.integralLimit;

  Profile_ctrl_phi1_s.isIAntiwindupEnabled  = false;
  Profile_ctrl_phi1_s.isDTermEnabled        = false;
  Profile_ctrl_phi1_s.kd                    = 0.0f;
  Profile_ctrl_phi1_s.outputLimit           = 0.0f;

  /* PID PARAMETERS PHASE 2 */
  /* ----------------------------------------------------------------------------- */
  Profile_ctrl_phi2_s.isPTermEnabled        = false;
  Profile_ctrl_phi2_s.kp                    = 0.0f;

  Profile_ctrl_phi2_s.isITermEnabled        = Profile_ctrl_main_s.isITermEnabled;
  Profile_ctrl_phi2_s.ki                    = Profile_ctrl_main_s.ki * 2.0f;
  Profile_ctrl_phi2_s.integralLimit         = Profile_ctrl_main_s.integralLimit;

  Profile_ctrl_phi2_s.isIAntiwindupEnabled  = false;
  Profile_ctrl_phi2_s.isDTermEnabled        = false;
  Profile_ctrl_phi2_s.kd                    = 0.0f;
  Profile_ctrl_phi2_s.outputLimit           = 0.0f;


  /* TIMER SECTION TO TRACK TIME IN MILISECONDS */
  /* ----------------------------------------------------------------------------- */
  hw_timer_init_msecs_tick();
  return TEMP_CTRL_INIT_OK;
}

/*****************************************************************************
 * Function: 	temp_ctrl_set_pid_config
 * Description: This function load/copy the value from g_Espresso_user_config_s (espresso_user_config_t)
                into private PID boiler temp. controller variable: Profile_ctrl_main_s
 *****************************************************************************/
tempCtrl_status_t temp_ctrl_set_pid_config(espresso_user_config_t *ptr_prof_data)
{
  /* PID PARAMETERS VALUE COPY */
  /* ----------------------------------------------------------------------------- */
  if( ptr_prof_data->pidPTerm == 0.0f )
  {
    Profile_ctrl_main_s.isPTermEnabled        = false;
    Profile_ctrl_main_s.kp                     = 0.0f;
  }else{
    Profile_ctrl_main_s.isPTermEnabled        = true;
    Profile_ctrl_main_s.kp                     = ptr_prof_data->pidPTerm;
  }
  if( ptr_prof_data->pidITerm == 0.0f )
  {
    Profile_ctrl_main_s.isITermEnabled        = false;
    Profile_ctrl_main_s.ki                     = 0.0f;
    Profile_ctrl_main_s.integralLimit          = 0.0f;
    Profile_ctrl_main_s.isIAntiwindupEnabled  = false;
  }else{
    Profile_ctrl_main_s.isITermEnabled        = true;
    Profile_ctrl_main_s.ki                     = ptr_prof_data->pidITerm;
    Profile_ctrl_main_s.integralLimit          = ptr_prof_data->pidImaxTerm;
    Profile_ctrl_main_s.isIAntiwindupEnabled  = true;
  }
  if( ptr_prof_data->pidDTerm == 0.0f )
  {
    Profile_ctrl_main_s.isDTermEnabled        = false;
    Profile_ctrl_main_s.kd                     = 0.0f;
  }else{
    Profile_ctrl_main_s.isDTermEnabled        = true;
    Profile_ctrl_main_s.kd                     = ptr_prof_data->pidDTerm;
  }
  return TEMP_CTRL_LOAD_OK;
  /*This code line is not part of this function's scope
  Profile_ctrl_main_s.feedPidBlock.setPoint = 45.0f;
  */
}

/*****************************************************************************
 * Function: 	temp_ctrl_set_boiler_setpoint
 * Description: Loads a new Setpoint determined by the second fcn paramater into the Temp Ctrl. 
 *****************************************************************************/
tempCtrl_LoadSP_t temp_ctrl_set_boiler_setpoint(espresso_user_config_t *ptr_prof_data, tempCtrl_LoadSP_t Setpoint)
{
  if( Setpoint == SET_POINT_BREW)
  {
    ptr_prof_data->boilerTempSetpointDegC = ptr_prof_data->brewTempDegC;
  }else{}
  if( Setpoint == SET_POINT_STEAM)
  {
    ptr_prof_data->boilerTempSetpointDegC = ptr_prof_data->steamTempDegC;
  }else{}
  /* M1 fix: reset the integral accumulator whenever the setpoint changes.
   * Without this, accumulated integral from the previous operating mode
   * (e.g. brew) carries over to the new operating mode (e.g. steam),
   * causing an overshoot transient.
   * pid_imc_block_t stores the running integral in its historyError field. */
  Profile_ctrl_main_s.historyError = 0.0f;
  return SET_POINT_LOAD_OK;
}

/*****************************************************************************
 * Function: 	fcn_loadI_ParamToCtrl_Temp_Phi1
 * Description: Loads only the I gain into the mainCtrl during Phase-1 (Pump Active)
 *****************************************************************************/
tempCtrl_status_t temp_ctrl_set_operational_integral_gain(espresso_user_config_t *ptr_prof_data)
{
  Profile_ctrl_main_s.ki = ptr_prof_data->pidIboostTerm;
  return TEMP_CTRL_I_LOAD_OK;
}
/*****************************************************************************
 * Function: 	fcn_loadI_ParamToCtrl_Temp_Phi2
 * Description: Loads only the I gain into the mainCtrl during Phase-2 (time after pump deactivation)
 *****************************************************************************/
tempCtrl_status_t temp_ctrl_scale_integral_gain(espresso_user_config_t *ptr_prof_data, float factor)
{
  Profile_ctrl_main_s.ki = (float)(ptr_prof_data->pidITerm * factor);
  return TEMP_CTRL_I_LOAD_OK;
}

/*****************************************************************************
 * Function: 	temp_ctrl_start_sampling_timer
 * Description: 
 *****************************************************************************/
void temp_ctrl_start_sampling_timer(void)
{
  nrf_drv_timer_enable((nrfx_timer_t const *)&Hw_Tmr_msecs_s.hwTmr);
  Hw_Tmr_msecs_s.is_active = ACTIVE;
}

/*****************************************************************************
 * Function: 	temp_ctrl_stop_sampling_timer
 * Description: 
 *****************************************************************************/
void temp_ctrl_stop_sampling_timer(void)
{
  nrf_drv_timer_disable((nrfx_timer_t const *)&Hw_Tmr_msecs_s.hwTmr);
  Hw_Tmr_msecs_s.is_active = NOT_ACTIVE;
}

/*****************************************************************************
 * Function: 	temp_ctrl_update
 * Description: 
 *****************************************************************************/
float temp_ctrl_update(espresso_user_config_t *ptr_prof_data)
{
  /* H3 fix: detect open-circuit (reads near 0�C) or shorted sensor (reads > 200 C).
   * If temperature is outside the physically plausible operating range,
   * return 0 % output to shut the heater off rather than letting the PID
   * command 100 % power based on a bogus reading. */
  float boilerTemp = (float)ptr_prof_data->boilerTempDegC;
  if (boilerTemp < 5.0f || boilerTemp > 200.0f)
  {
    return 0.0f;  /* sensor fault safe default: no heating */
  }

  Profile_ctrl_main_s.feedPidBlock.processVariable  = boilerTemp;
  Profile_ctrl_main_s.feedPidBlock.setPoint         = (float)ptr_prof_data->boilerTempSetpointDegC;
  Profile_ctrl_main_s.feedPidBlock.timeMsecs        = (uint32_t)elapsed_msecs;
  return (float)pid_imc_compute((pid_imc_block_t *)&Profile_ctrl_main_s);

  /*boiler_ssr_pwr_update((uint16_t)Profile_ctrl_main_s.output);*/
}

/*******************************************************************************
 *
 *		PRIVATE FUNCTIONS SECTION
 *
 ******************************************************************************/

/*****************************************************************************
 * Function: 	hw_timer_init_msecs_tick
 * Description: This function init the HW-timer no.3
                to track units or tens of miliseconds.
                WARNING: counter is defined by: HWTMR_PERIOD_USECS
 *****************************************************************************/
 static void hw_timer_init_msecs_tick(void)
 {
    Hw_Tmr_msecs_s.hwTmr               = (nrf_drv_timer_t)NRF_DRV_TIMER_INSTANCE(3);
    Hw_Tmr_msecs_s.hwTmr_isr_handler   = temp_ctrl_sampling_timer_event_handler;
    Hw_Tmr_msecs_s.is_active              = false;

    /* TIMER SECTION TO SETUP SAMPLING TIME FOR PID CONTROLLER */
    /* ----------------------------------------------------------------------------- */
    uint32_t err_code = NRF_SUCCESS;
    /* Configure TIMER_HW intance */
    nrf_drv_timer_config_t pid_timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    pid_timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    pid_timer_cfg.frequency = NRF_TIMER_FREQ_1MHz;
    /*Hw_Tmr_msecs_s.tmrPeriod_us = (uint32_t)((TEMP_CTRL_ITERATION_T+0.007797f) *1000.0f*1000.0f);*/ /* time per second */
    Hw_Tmr_msecs_s.tmrPeriod_us = (uint32_t)(HWTMR_PERIOD_USECS);

    err_code = nrf_drv_timer_init((nrfx_timer_t const * const)&Hw_Tmr_msecs_s.hwTmr, &pid_timer_cfg, Hw_Tmr_msecs_s.hwTmr_isr_handler);
    APP_ERROR_CHECK(err_code);

    Hw_Tmr_msecs_s.tmrPeriod_ticks = nrf_drv_timer_us_to_ticks((nrfx_timer_t const * const)&Hw_Tmr_msecs_s.hwTmr, Hw_Tmr_msecs_s.tmrPeriod_us);

    nrf_drv_timer_extended_compare((nrfx_timer_t const * const)&Hw_Tmr_msecs_s.hwTmr,
                                   NRF_TIMER_CC_CHANNEL0,
                                   Hw_Tmr_msecs_s.tmrPeriod_ticks,
                                   NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                   true);
    /* Timer will be enabled via this function: temp_ctrl_start_sampling_timer */
    /*nrf_drv_timer_enable(&Hw_Tmr_msecs_s.hwTmr);*/
    /*nrf_drv_timer_enable(&TIMER_HW2);*/ 
 }