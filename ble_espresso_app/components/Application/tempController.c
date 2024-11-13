
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "tempController.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************
#define CTRL_SAMPLING_T_US   (TEMP_CTRL_SAMPLING_T * 1000.0f * 1000.0f)
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
} struct_PIDsamplingTimer;

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
//volatile PID_Block_fStruct sBoilerTempCtrl;
//volatile struct_PIDsamplingTimer sPIDsamplingTmr;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
static PID_Block_fStruct sBoilerTempCtrl;
static struct_PIDsamplingTimer sPIDsamplingTmr;

//*****************************************************************************
//
//			ISR HANDLERS FUNCTIONS
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	isr_SamplingTime_EventHandler
 * Description: 
 * Parameters:	
 * Return:
 *****************************************************************************/
void isr_SamplingTime_EventHandler(nrf_timer_event_t event_type, void* p_context)
{
    nrf_drv_gpiote_out_toggle(29);
    //sBoilerTempCtrl.Input = f_getBoilerTemperature();
    //sBoilerTempCtrl.SetPoint = 45.0f;
   // fcn_PID_Block_Iteration((PID_Block_fStruct *)&sBoilerTempCtrl);
}

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	fcn_initTemperatureController
 * Description: This function init the HW-timer no.3
 * Return:      N/A
 *****************************************************************************/
void fcn_initTemperatureController(void)
{
    sPIDsamplingTmr.hwTmr               = (nrf_drv_timer_t)NRF_DRV_TIMER_INSTANCE(3);
    sPIDsamplingTmr.hwTmr_isr_handler   = isr_SamplingTime_EventHandler;
    sPIDsamplingTmr.status              = false;

    //TIMER SECTION TO SETUP SAMPLING TIME FOR PID CONTROLLER
    //------------------------------------------------------------------------------
    uint32_t err_code = NRF_SUCCESS;
    //Configure TIMER_HW intance
    nrf_drv_timer_config_t pid_timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    pid_timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    pid_timer_cfg.frequency = NRF_TIMER_FREQ_1MHz;
    //sPIDsamplingTmr.tmrPeriod_us = (uint32_t)((TEMP_CTRL_ITERATION_T+0.007797f) *1000.0f*1000.0f) ; //time per second
    sPIDsamplingTmr.tmrPeriod_us = (uint32_t)(CTRL_SAMPLING_T_US) ; 

    err_code = nrf_drv_timer_init((nrfx_timer_t const * const)&sPIDsamplingTmr.hwTmr, &pid_timer_cfg, sPIDsamplingTmr.hwTmr_isr_handler);
    APP_ERROR_CHECK(err_code);

    sPIDsamplingTmr.tmrPeriod_ticks = nrf_drv_timer_us_to_ticks((nrfx_timer_t const * const)&sPIDsamplingTmr.hwTmr, sPIDsamplingTmr.tmrPeriod_us );

    nrf_drv_timer_extended_compare((nrfx_timer_t const * const)&sPIDsamplingTmr.hwTmr, 
                                   NRF_TIMER_CC_CHANNEL0,
                                   sPIDsamplingTmr.tmrPeriod_ticks, 
                                   NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                   true);
    /* Timer will be enabled via this function: fcn_startTemperatureController  */
    //nrf_drv_timer_enable(&sPIDsamplingTmr.hwTmr);
    //nrf_drv_timer_enable(&TIMER_HW2);     
    
    //PID PARAMETERS VALUE SETUP
    //------------------------------------------------------------------------------
    sBoilerTempCtrl.dt                     = TEMP_CTRL_SAMPLING_T;

    sBoilerTempCtrl.P_TERM_CTRL            = ACTIVE;
    sBoilerTempCtrl.Kp                     = TEMP_CTRL_GAIN_P;
    sBoilerTempCtrl.I_TERM_CTRL            = ACTIVE;
    sBoilerTempCtrl.Ki                     = TEMP_CTRL_GAIN_I;
    sBoilerTempCtrl.HistoryErrorLimit      = TEMP_CTRL_HIST_LIMIT;
    sBoilerTempCtrl.I_ANTIWINDUP_CTRL      = ACTIVE;
    sBoilerTempCtrl.Kwindup                = TEMP_CTRL_GAIN_WINDUP;

    sBoilerTempCtrl.D_TERM_CTRL            = NOT_ACTIVE;
    sBoilerTempCtrl.Kd                     = TEMP_CTRL_GAIN_D;
    sBoilerTempCtrl.D_TERM_LP_FILTER_CTRL  = NOT_ACTIVE;
   
    sBoilerTempCtrl.PID_OUTPUT_GAIN_CTRL   = NOT_ACTIVE;
    sBoilerTempCtrl.OutputLimit            = TEMP_CTRL_MAX;

    sBoilerTempCtrl.SetPoint = 45.0f;
    
}

/*****************************************************************************
 * Function: 	fcn_startTemperatureController
 * Description: 
 *****************************************************************************/
void fcn_startTemperatureController(void)
{
  nrf_drv_timer_enable((nrfx_timer_t const *)&sPIDsamplingTmr.hwTmr);
  sPIDsamplingTmr.status = ACTIVE;
}

/*****************************************************************************
 * Function: 	fcn_startTemperatureController
 * Description: 
 *****************************************************************************/
void fcn_stopTemperatureController(void)
{
  nrf_drv_timer_disable((nrfx_timer_t const *)&sPIDsamplingTmr.hwTmr);
  sPIDsamplingTmr.status = NOT_ACTIVE;
}



//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	InitClocks
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
 // Public function 2 