//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "tempController.h"
#include "spi_Devices.h"
#include "solidStateRelay_Controller.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************
//#define TEMP_CTRL_SAMPLING_T    0.01f                         //Sampling Time in seconds
#define HWTMR_PERIOD_MS           1.0f                          //Period of the TMR is in miliseconds
#define HWTMR_PERIOD_US           (HWTMR_PERIOD_MS * 1009.0f)

//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
typedef struct
{
    nrf_drv_timer_t             hwTmr;              ///HW-Timer that will control Relay trigger
    nrfx_timer_event_handler_t  hwTmr_isr_handler;
    uint32_t                    tmrPeriod_us;
    uint32_t                    tmrPeriod_ticks;
    bool                        status;
} struct_HWTimer;

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************


//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
static PID_IMC_Block_fStruct sctrl_profile_main;
static PID_IMC_Block_fStruct sctrl_profile_phi1;
static PID_IMC_Block_fStruct sctrl_profile_phi2;
static struct_HWTimer sHwTmr_Miliseconds;
static volatile uint32_t milisTicks=0;

//*****************************************************************************
//
//			PRIVATE FUNCTIONS
//
//*****************************************************************************
tempCtrl_status_t fcn_loadI_ParamToCtrl_Temp(PID_IMC_Block_fStruct *ptrParam);

void fcn_initMilisecondHWTimer(void);

//*****************************************************************************
//
//			ISR HANDLERS FUNCTIONS
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	isr_HwTmr3_Period_EventHandler
 * Description: increment counter tick every 1ms
 *****************************************************************************/
void isr_HwTmr3_Period_EventHandler(nrf_timer_event_t event_type, void* p_context)
{
    //GPIO29 controls the LED_HeartBeat - for measurieng purposes.
    //nrf_drv_gpiote_out_toggle(29);
    milisTicks++;
}

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	fcn_initCntrl_Temp
 * Description: 
 * Return:      N/A
 *****************************************************************************/
tempCtrl_status_t fcn_initCntrl_Temp(void)
{
  //PID PARAMETERS VALUE SETUP
  //------------------------------------------------------------------------------
  sctrl_profile_main.P_TERM_CTRL       = ACTIVE;
  sctrl_profile_main.Kp                = TEMP_CTRL_KP;
  sctrl_profile_main.I_TERM_CTRL       = ACTIVE;
  sctrl_profile_main.Ki                = TEMP_CTRL_KI;
  sctrl_profile_main.IntegralLimit     = TEMP_CTRL_HIST_LIMIT;
  sctrl_profile_main.I_ANTIWINDUP_CTRL = ACTIVE;

  sctrl_profile_main.D_TERM_CTRL       = ACTIVE;
  sctrl_profile_main.D_TERM_FILTER_CTRL= NOT_ACTIVE;
  sctrl_profile_main.Kd                = TEMP_CTRL_KD;
 
  sctrl_profile_main.OutputLimit       = TEMP_CTRL_MAX;

  //PID PARAMETERS PHASE 1
  //------------------------------------------------------------------------------
  sctrl_profile_phi1.P_TERM_CTRL       = NOT_ACTIVE;
  sctrl_profile_phi1.Kp                = 0.0f;

  sctrl_profile_phi1.I_TERM_CTRL       = sctrl_profile_main.I_TERM_CTRL;
  sctrl_profile_phi1.Ki                = sctrl_profile_main.Ki * 6.5f;
  sctrl_profile_phi1.IntegralLimit     = sctrl_profile_main.IntegralLimit;

  sctrl_profile_phi1.I_ANTIWINDUP_CTRL = NOT_ACTIVE;
  sctrl_profile_phi1.D_TERM_CTRL       = NOT_ACTIVE;
  sctrl_profile_phi1.D_TERM_FILTER_CTRL= NOT_ACTIVE;
  sctrl_profile_phi1.Kd                = 0.0f;
  sctrl_profile_phi1.OutputLimit       = 0.0f;

  //PID PARAMETERS PHASE 1
  //------------------------------------------------------------------------------
  sctrl_profile_phi2.P_TERM_CTRL       = NOT_ACTIVE;
  sctrl_profile_phi2.Kp                = 0.0f;

  sctrl_profile_phi2.I_TERM_CTRL       = sctrl_profile_main.I_TERM_CTRL;
  sctrl_profile_phi2.Ki                = sctrl_profile_main.Ki * 2.0f;
  sctrl_profile_phi2.IntegralLimit     = sctrl_profile_main.IntegralLimit;

  sctrl_profile_phi2.I_ANTIWINDUP_CTRL = NOT_ACTIVE;
  sctrl_profile_phi2.D_TERM_CTRL       = NOT_ACTIVE;
  sctrl_profile_phi2.D_TERM_FILTER_CTRL= NOT_ACTIVE;
  sctrl_profile_phi2.Kd                = 0.0f;
  sctrl_profile_phi2.OutputLimit       = 0.0f;


  //TIMER SECTION TO TRACK TIME IN MILISECONDS
  //------------------------------------------------------------------------------
  fcn_initMilisecondHWTimer();
  return TEMPCTRL_INIT_OK;
}

/*****************************************************************************
 * Function: 	fcn_loadPID_ParamToCtrl_Temp
 * Description: This function load/copy the value from blEspressoProfile (bleSpressoUserdata_struct)
                into private PID boiler temp. controller variable: sctrl_profile_main
 *****************************************************************************/
tempCtrl_status_t fcn_loadPID_ParamToCtrl_Temp(bleSpressoUserdata_struct *prt_profData)
{
  //PID PARAMETERS VALUE COPY
  //------------------------------------------------------------------------------
  if( prt_profData->Pid_P_term == 0.0f )
  {
    sctrl_profile_main.P_TERM_CTRL            = NOT_ACTIVE;
    sctrl_profile_main.Kp                     = 0.0f;
  }else{
    sctrl_profile_main.P_TERM_CTRL            = ACTIVE;
    sctrl_profile_main.Kp                     = prt_profData->Pid_P_term;
  }
  if( prt_profData->Pid_I_term == 0.0f )
  {
    sctrl_profile_main.I_TERM_CTRL            = NOT_ACTIVE;
    sctrl_profile_main.Ki                     = 0.0f;
    sctrl_profile_main.IntegralLimit          = 0.0f;
    sctrl_profile_main.I_ANTIWINDUP_CTRL      = NOT_ACTIVE;
  }else{
    sctrl_profile_main.I_TERM_CTRL            = ACTIVE;
    sctrl_profile_main.Ki                     = prt_profData->Pid_I_term;
    sctrl_profile_main.IntegralLimit          = prt_profData->Pid_Imax_term;
    sctrl_profile_main.I_ANTIWINDUP_CTRL      = prt_profData->Pid_Iwindup_term;
  }
  if( prt_profData->Pid_D_term == 0.0f )
  {
    sctrl_profile_main.D_TERM_CTRL            = NOT_ACTIVE;
    sctrl_profile_main.Kd                     = 0.0f;
    sctrl_profile_main.D_TERM_FILTER_CTRL     = NOT_ACTIVE;
  }else{
    sctrl_profile_main.D_TERM_CTRL            = ACTIVE;
    sctrl_profile_main.Kd                     = prt_profData->Pid_D_term;
    sctrl_profile_main.D_TERM_FILTER_CTRL     = NOT_ACTIVE;
  }
  return TEMPCTRL_LOAD_OK;
  /*This code line is not part of this function's scope
  sctrl_profile_main.feedPIDblock.SetPoint = 45.0f;
  */
}

/*****************************************************************************
 * Function: 	fcn_loaddSetPoint_ParamToCtrl_Temp
 * Description: Loads a new Setpoint determined by the second fcn paramater into the Temp Ctrl. 
 *****************************************************************************/
tempCtrl_LoadSP_t fcn_loaddSetPoint_ParamToCtrl_Temp(bleSpressoUserdata_struct *prt_profData, tempCtrl_LoadSP_t Setpoint)
{
  if( Setpoint == SETPOINT_BREW)
  {
    prt_profData->temp_Target = prt_profData->sp_BrewTemp;
  }else{}
  if( Setpoint == SETPOINT_STEAM)
  {
    prt_profData->temp_Target = prt_profData->sp_StemTemp;
  }else{} 
  return TEMPCTRL_SP_LOAD_OK;
}

/*****************************************************************************
 * Function: 	fcn_loadI_ParamToCtrl_Temp_Phi1
 * Description: Loads only the I gain into the mainCtrl during Phase-1 (Pump Active)
 *****************************************************************************/
tempCtrl_status_t fcn_loadIboost_ParamToCtrl_Temp(bleSpressoUserdata_struct *prt_profData)
{
  sctrl_profile_main.Ki = prt_profData->Pid_Iboost_term;
  return TEMPCTRL_I_LOAD_OK;
}
/*****************************************************************************
 * Function: 	fcn_loadI_ParamToCtrl_Temp_Phi2
 * Description: Loads only the I gain into the mainCtrl during Phase-2 (time after pump deactivation)
 *****************************************************************************/
tempCtrl_status_t fcn_multiplyI_ParamToCtrl_Temp(bleSpressoUserdata_struct *prt_profData, float factor)
{
  sctrl_profile_main.Ki = (float)(prt_profData->Pid_I_term * factor);
  return TEMPCTRL_I_LOAD_OK;
}

/*****************************************************************************
 * Function: 	fcn_startTempCtrlSamplingTmr
 * Description: 
 *****************************************************************************/
void fcn_startTempCtrlSamplingTmr(void)
{
  nrf_drv_timer_enable((nrfx_timer_t const *)&sHwTmr_Miliseconds.hwTmr);
  sHwTmr_Miliseconds.status = ACTIVE;
}

/*****************************************************************************
 * Function: 	fcn_stopTempCtrlSamplingTmr
 * Description: 
 *****************************************************************************/
void fcn_stopTempCtrlSamplingTmr(void)
{
  nrf_drv_timer_disable((nrfx_timer_t const *)&sHwTmr_Miliseconds.hwTmr);
  sHwTmr_Miliseconds.status = NOT_ACTIVE;
}

/*****************************************************************************
 * Function: 	fcn_updateTemperatureController
 * Description: 
 *****************************************************************************/
float fcn_updateTemperatureController(bleSpressoUserdata_struct *prt_profData)
{
  sctrl_profile_main.feedPIDblock.ProcessVariable  = (float)prt_profData->temp_Boiler;
  sctrl_profile_main.feedPIDblock.SetPoint         = (float)prt_profData->temp_Target;
  sctrl_profile_main.feedPIDblock.TimeMilis        = (uint32_t)milisTicks;
  return (float)fcn_update_PIDimc_typeA((PID_IMC_Block_fStruct *)&sctrl_profile_main);

  //fcn_boilerSSR_pwrUpdate((uint16_t)sctrl_profile_main.Output);
}

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	fcn_initMilisecondHWTimer
 * Description: This function init the HW-timer no.3
                to track units or tens of miliseconds.
                WARNING: counter is defined by: HWTMR_PERIOD_US
 *****************************************************************************/
 void fcn_initMilisecondHWTimer(void)
 {
    sHwTmr_Miliseconds.hwTmr               = (nrf_drv_timer_t)NRF_DRV_TIMER_INSTANCE(3);
    sHwTmr_Miliseconds.hwTmr_isr_handler   = isr_HwTmr3_Period_EventHandler;
    sHwTmr_Miliseconds.status              = false;

    //TIMER SECTION TO SETUP SAMPLING TIME FOR PID CONTROLLER
    //------------------------------------------------------------------------------
    uint32_t err_code = NRF_SUCCESS;
    //Configure TIMER_HW intance
    nrf_drv_timer_config_t pid_timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    pid_timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    pid_timer_cfg.frequency = NRF_TIMER_FREQ_1MHz;
    //sHwTmr_Miliseconds.tmrPeriod_us = (uint32_t)((TEMP_CTRL_ITERATION_T+0.007797f) *1000.0f*1000.0f) ; //time per second
    sHwTmr_Miliseconds.tmrPeriod_us = (uint32_t)(HWTMR_PERIOD_US) ; 

    err_code = nrf_drv_timer_init((nrfx_timer_t const * const)&sHwTmr_Miliseconds.hwTmr, &pid_timer_cfg, sHwTmr_Miliseconds.hwTmr_isr_handler);
    APP_ERROR_CHECK(err_code);

    sHwTmr_Miliseconds.tmrPeriod_ticks = nrf_drv_timer_us_to_ticks((nrfx_timer_t const * const)&sHwTmr_Miliseconds.hwTmr, sHwTmr_Miliseconds.tmrPeriod_us );

    nrf_drv_timer_extended_compare((nrfx_timer_t const * const)&sHwTmr_Miliseconds.hwTmr, 
                                   NRF_TIMER_CC_CHANNEL0,
                                   sHwTmr_Miliseconds.tmrPeriod_ticks, 
                                   NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                   true);
    /* Timer will be enabled via this function: fcn_startTemperatureController  */
    //nrf_drv_timer_enable(&sHwTmr_Miliseconds.hwTmr);
    //nrf_drv_timer_enable(&TIMER_HW2); 
 }