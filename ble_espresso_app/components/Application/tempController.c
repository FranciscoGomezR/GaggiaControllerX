
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

//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
volatile PID_Block_fStruct sBoilerTempCtrl;
volatile struct_PIDtimer sPIDtimer;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	InitClocks
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_initTemperatureController(PID_Block_fStruct *ptr_pidTempCtrl)
{
    sPIDtimer.hwTmr               = (nrf_drv_timer_t)NRF_DRV_TIMER_INSTANCE(3);
    sPIDtimer.hwTmr_isr_handler   = isr_TempController_EventHandler;
    sPIDtimer.status              = false;

    //TIMER SECTION
    //------------------------------------------------------------------------------
    uint32_t err_code = NRF_SUCCESS;
    //Configure TIMER_HW intance
    nrf_drv_timer_config_t pid_timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    pid_timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    pid_timer_cfg.frequency = NRF_TIMER_FREQ_1MHz;
    sPIDtimer.tmrPeriod_us = (uint32_t)((TEMP_CTRL_ITERATION_T+0.007797f) *1000.0f*1000.0f) ; //time per second

    err_code = nrf_drv_timer_init((nrfx_timer_t const * const)&sPIDtimer.hwTmr, &pid_timer_cfg, sPIDtimer.hwTmr_isr_handler);
    APP_ERROR_CHECK(err_code);

    sPIDtimer.tmrPeriod_ticks = nrf_drv_timer_us_to_ticks((nrfx_timer_t const * const)&sPIDtimer.hwTmr, sPIDtimer.tmrPeriod_us );

   nrf_drv_timer_extended_compare((nrfx_timer_t const * const)&sPIDtimer.hwTmr, 
                                   NRF_TIMER_CC_CHANNEL0,
                                   sPIDtimer.tmrPeriod_ticks, 
                                   NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                   true);
    //nrf_drv_timer_enable(&sPIDtimer.hwTmr);
    //nrf_drv_timer_enable(&TIMER_HW2);     //Timer initialize but not enable
    
    //PID SECTION
    //------------------------------------------------------------------------------
    ptr_pidTempCtrl->dt                     = TEMP_CTRL_ITERATION_T;

    ptr_pidTempCtrl->P_TERM_CTRL            = ACTIVE;
    ptr_pidTempCtrl->Kp                     = TEMP_CTRL_GAIN_P;
    ptr_pidTempCtrl->I_TERM_CTRL            = ACTIVE;
    ptr_pidTempCtrl->Ki                     = TEMP_CTRL_GAIN_I;
    ptr_pidTempCtrl->HistoryErrorLimit      = TEMP_CTRL_HIST_LIMIT;
    ptr_pidTempCtrl->I_ANTIWINDUP_CTRL      = ACTIVE;
    ptr_pidTempCtrl->Kwindup                = TEMP_CTRL_GAIN_WINDUP;

    ptr_pidTempCtrl->D_TERM_CTRL            = NOT_ACTIVE;
    ptr_pidTempCtrl->Kd                     = TEMP_CTRL_GAIN_D;
    ptr_pidTempCtrl->D_TERM_LP_FILTER_CTRL  = NOT_ACTIVE;
   
    ptr_pidTempCtrl->PID_OUTPUT_GAIN_CTRL   = NOT_ACTIVE;
    ptr_pidTempCtrl->OutputLimit            = TEMP_CTRL_MAX;

    ptr_pidTempCtrl->SetPoint = 45.0f;
    
    /*GPIOS SECTION - > FOR DEBUG PORPUSES
    ret_code_t err_code_gpio;
    //nrf_drv_gpiote_init -> Already init. in ac_inputs_drv
    //err_code_gpio = nrf_drv_gpiote_init();
    //APP_ERROR_CHECK(err_code_gpio);
    //Output pin to control SSR
    //------------------------------------------------------------------------
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
    err_code_gpio = nrf_drv_gpiote_out_init(20, &out_config);
    APP_ERROR_CHECK(err_code_gpio);*/
}

/*****************************************************************************
 * Function: 	fcn_startTemperatureController
 * Description: 
 *****************************************************************************/
void fcn_startTemperatureController(void)
{
  nrf_drv_timer_enable((nrfx_timer_t const *)&sPIDtimer.hwTmr);
  sPIDtimer.status = ACTIVE;
}

/*****************************************************************************
 * Function: 	fcn_startTemperatureController
 * Description: 
 *****************************************************************************/
void fcn_stopTemperatureController(void)
{
  nrf_drv_timer_disable((nrfx_timer_t const *)&sPIDtimer.hwTmr);
  sPIDtimer.status = NOT_ACTIVE;
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