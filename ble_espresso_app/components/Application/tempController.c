
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "tempController.h"
#include "spi_sensors.h"
#include "solidStateRelay_Controller.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************
//#define TEMP_CTRL_SAMPLING_T    0.01f                         //Sampling Time in seconds
#define HWTMR_PERIOD_MS           1.0f                          //Period of the TMR is in miliseconds
#define HWTMR_PERIOD_US           (HWTMR_PERIOD_MS * 1000.0f)

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
//volatile PID_Block_fStruct sBoilerTempCtrl;
//volatile struct_HWTimer sHwTmr_Miliseconds;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
static PID_Block_fStruct sBoilerTempCtrl;
static struct_HWTimer sHwTmr_Miliseconds;
static volatile uint32_t milisTicks=0;

//*****************************************************************************
//
//			PRIVATE FUNCTIONS
//
//*****************************************************************************
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
    nrf_drv_gpiote_out_toggle(29);
    milisTicks++;
}

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	fcn_initTemperatureController
 * Description: 
 * Return:      N/A
 *****************************************************************************/
void fcn_initTemperatureController(void)
{
    //PID PARAMETERS VALUE SETUP
    //------------------------------------------------------------------------------
    sBoilerTempCtrl.P_TERM_CTRL            = ACTIVE;
    sBoilerTempCtrl.Kp                     = TEMP_CTRL_GAIN_P;
    sBoilerTempCtrl.I_TERM_CTRL            = ACTIVE;
    sBoilerTempCtrl.Ki                     = TEMP_CTRL_GAIN_I;
    sBoilerTempCtrl.IntegralLimit           = TEMP_CTRL_HIST_LIMIT;
    sBoilerTempCtrl.I_ANTIWINDUP_CTRL      = ACTIVE;
    //sBoilerTempCtrl.Kwindup                = TEMP_CTRL_GAIN_WINDUP;

    sBoilerTempCtrl.D_TERM_CTRL            = NOT_ACTIVE;
    sBoilerTempCtrl.Kd                     = TEMP_CTRL_GAIN_D;
    sBoilerTempCtrl.D_TERM_LP_FILTER_CTRL  = NOT_ACTIVE;
    sBoilerTempCtrl.LPF_FCUTOFF_HZ         = 30.0f;
   
    sBoilerTempCtrl.OutputLimit            = TEMP_CTRL_MAX;

    sBoilerTempCtrl.feedPIDblock.SetPoint = 45.0f;

    fcn_PID_Block_Dterm_LPF_Init( (PID_Block_fStruct *)&sBoilerTempCtrl );
    
    //TIMER SECTION TO TRACK TIME IN MILISECONDS
    //------------------------------------------------------------------------------
    fcn_initMilisecondHWTimer();
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
 * Function: 	fcn_startTemperatureController
 * Description: 
 *****************************************************************************/
void fcn_updateTemperatureController(void)
{
    volatile uint32_t pidOuput;
    pidOuput = fcn_update_PID_Block(  f_getBoilerTemperature(),
                                      33.0f,
                                      milisTicks,
                                      (PID_Block_fStruct *)&sBoilerTempCtrl);
    /* TO DO - verify code up to this line of code  */
    fcn_boilerSSR_pwrUpdate((uint16_t)pidOuput);
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