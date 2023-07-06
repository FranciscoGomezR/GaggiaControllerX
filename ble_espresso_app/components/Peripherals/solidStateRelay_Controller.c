
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "solidStateRelay_Controller.h"

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
volatile struct_ssrController sSSRdrvConfig;

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
 * Function: 	fcn_initSsrController
 * Description: This function will GPIO for: ZeroCross-Input and SSR-Output.
                It init HW-timer to control SSrelay trigger.
                It contains the functions to drive SSrelay from 0% to 100% AC cycle.
 * Caveats:     optimize for 50Hz
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_initSsrController(struct_ssrController * ptr_instance)
{
    ptr_instance->smTrigStatus            = smS_Release;
    ptr_instance->sSRR_timing_us.tStep    = POWER_MAX_VALUE/AC_PERCENT_STEP;
    ptr_instance->sSRR_timing_us.tZCdelay = 200;
    ptr_instance->sSRR_timing_us.tTrigger = 0;  //50% of power
    ptr_instance->sSRR_timing_us.tPeriod  = AC_CYCLE_PERIOD;
    //TIMER SECTION
    //------------------------------------------------------------------------------
    uint32_t err_code = NRF_SUCCESS;
    //Configure TIMER_HW intance
    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    timer_cfg.frequency = NRF_TIMER_FREQ_8MHz;
    err_code = nrf_drv_timer_init(&ptr_instance->hwTmr, &timer_cfg, ptr_instance->hwTmr_isr_handler);
    APP_ERROR_CHECK(err_code);

    ptr_instance->sSRR_timing_us.cZCdelay = nrf_drv_timer_us_to_ticks(&ptr_instance->hwTmr, ptr_instance->sSRR_timing_us.tZCdelay );
    ptr_instance->sSRR_timing_us.cTrigger = nrf_drv_timer_us_to_ticks(&ptr_instance->hwTmr, ptr_instance->sSRR_timing_us.tTrigger );
    ptr_instance->sSRR_timing_us.cPeriod = nrf_drv_timer_us_to_ticks(&ptr_instance->hwTmr, ptr_instance->sSRR_timing_us.tPeriod );

    //nrf_drv_timer_compare(&ptr_instance->hwTmr,NRF_TIMER_CC_CHANNEL0,ptr_instance->sSRR_timing_us.cTrigger,true);
    //nrf_drv_timer_extended_compare(&ptr_instance->hwTmr, NRF_TIMER_CC_CHANNEL1, ptr_instance->sSRR_timing_us.cPeriod, NRF_TIMER_SHORT_COMPARE1_STOP_MASK, true);
    nrf_drv_timer_extended_compare(&ptr_instance->hwTmr, 
                                   NRF_TIMER_CC_CHANNEL0,
                                   ptr_instance->sSRR_timing_us.cTrigger, 
                                   NRF_TIMER_SHORT_COMPARE0_STOP_MASK,
                                   true);
    //nrf_drv_timer_enable(&TIMER_HW0);
    //Timer initialize but not enable

    //GPIOS SECTION
    //------------------------------------------------------------------------------
    ret_code_t err_code_gpio;
    //nrf_drv_gpiote_init -> Already init. in ac_inputs_drv
    //err_code_gpio = nrf_drv_gpiote_init();
    //APP_ERROR_CHECK(err_code_gpio);
    //Output pin to control SSR
    //------------------------------------------------------------------------
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
    err_code_gpio = nrf_drv_gpiote_out_init(ptr_instance->out_SSRelay, &out_config);
    APP_ERROR_CHECK(err_code_gpio);

    //err_code_gpio = nrf_drv_gpiote_out_init(31, &out_config);
    //APP_ERROR_CHECK(err_code_gpio);

    //Zero Cross input with external interrupt enable
    //------------------------------------------------------------------------
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_NOPULL;
    in_config.sense = GPIOTE_CONFIG_POLARITY_LoToHi;
    err_code_gpio = nrf_drv_gpiote_in_init(ptr_instance->in_zCross,
                                       &in_config, 
                                       ptr_instance->zcross_isr_handler);
    APP_ERROR_CHECK(err_code_gpio);
    nrf_drv_gpiote_in_event_enable(ptr_instance->in_zCross, true);
}

/*****************************************************************************
 * Function: 	fcn_SSR_Controller
 * Description: 
 * Caveats:
 * Parameters:	outputPower range: 0% - 100% = 0 - 1000
 * Return:
 *****************************************************************************/
void fcn_SSR_pwrUpdate(struct_ssrController * ptr_instance, uint16_t outputPower)
{
    //nrf_drv_timer_pause(&ptr_instance->hwTmr);
    //nrf_drv_timer_disable(&ptr_instance->hwTmr);
    if(outputPower >0 && outputPower <1000)
    {
      ptr_instance->sSRR_timing_us.tTrigger = ptr_instance->sSRR_timing_us.tPeriod - 
                                              ( ptr_instance->sSRR_timing_us.tStep * outputPower);

      ptr_instance->sSRR_timing_us.cTrigger = nrf_drv_timer_us_to_ticks(&ptr_instance->hwTmr, 
                                              ptr_instance->sSRR_timing_us.tTrigger );
      nrf_drv_timer_enable(&ptr_instance->hwTmr);
      nrf_drv_gpiote_in_event_enable(ptr_instance->in_zCross, true);
    }else{
      nrf_drv_timer_disable(&ptr_instance->hwTmr);
      nrf_drv_gpiote_in_event_disable(ptr_instance->in_zCross);
      if(outputPower == 1000)
      {
          nrf_drv_gpiote_out_set(ptr_instance->out_SSRelay);
          
      }else{
        if(outputPower == 0)
        {
          nrf_drv_gpiote_out_clear(ptr_instance->out_SSRelay);
        }else{}
      }
    }
    //nrf_drv_timer_resume(&ptr_instance->hwTmr);
    //nrf_drv_timer_enable(&ptr_instance->hwTmr);
}

/*****************************************************************************
 * Function: 	fcn_SSR_ctrlUpdate
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_SSR_ctrlUpdate(struct_ssrController * ptr_instance)
{
    nrf_drv_timer_disable(&ptr_instance->hwTmr);
    nrf_drv_timer_extended_compare(&ptr_instance->hwTmr, 
                                    NRF_TIMER_CC_CHANNEL0, 
                                    ptr_instance->sSRR_timing_us.cTrigger, 
                                    NRF_TIMER_SHORT_COMPARE0_STOP_MASK, 
                                    true);
    nrf_drv_timer_enable(&ptr_instance->hwTmr);
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


/*
nrf_drv_gpiote_out_toggle(sSSRdrvConfig.out_SSRelay);
nrf_drv_gpiote_out_clear(sSSRdrvConfig.out_SSRelay);
nrf_drv_gpiote_out_set(sSSRdrvConfig.out_SSRelay);
*/