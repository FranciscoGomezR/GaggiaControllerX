/*
  time and ciount for tZCdelay are not implemented yet.
  */
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
#define T_ZEROCROSS_DELAY_DET   00
//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
/**
 * @brief Timer driver instance data structure.
 */
typedef struct
{
    uint16_t    tStep;
    uint16_t    tPeriod;
    uint16_t    tTrigger;
    uint16_t    tZCdelay;
    uint32_t    cPeriod;
    uint32_t    cTrigger;
    uint32_t    cZCdelay;
}struct_ssrTiming;

typedef struct
{
    nrf_drv_timer_t             hwTmr;                //HW-Timer that will control Relay trigger
    nrfx_timer_event_handler_t  hwTmr_isr_handler;
    uint8_t                     in_zCross;            //AC Zero-cross input pin
    uint8_t                     out_SSRelay;          //controller output pin
    nrfx_gpiote_evt_handler_t   zcross_isr_handler;
    struct_ssrTiming            sSRR_timing_us;
    uint8_t                     smTrigStatus;
    uint16_t                    srrPower;
    uint8_t                     ssrPWRstatus;
} struct_SSRinstance;

typedef struct
{
    uint8_t                     out_SSRelay;          //controller output pin
    uint8_t                     ssrState;
    uint8_t                     ssrPWRstatus;
} struct_OnOffSSRinstance;

typedef struct
{
    uint8_t                     out_SSRelay;          //controller output pin
    uint8_t                     cntPWRcycles;         
    uint8_t                     smTrigStatus;
    uint8_t                     srrPowerCnt;
    uint16_t                    srrPower;
    uint8_t                     ssrPWRstatus;
} struct_SSR_ZC_instance;

typedef struct
{
    uint8_t                     in_zCross;            //AC Zero-cross input pin
    nrfx_gpiote_evt_handler_t   zcross_isr_handler;
    bool                        status;
} struct_SSRcontroller;

enum{
  smS_Release=0,
  smS_Engage
};

enum{
  SSR_NOPWR=0,
  SSR_MIDPWR,
  SSR_FULLPWR,
  SSR_BELOWMIDPWR,
  SSR_ABOVEMIDPWR
};

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//****************************************************************************
volatile struct_SSRcontroller sSSRcontroller;
#if SSR_CTRL_BOILER_HEAT == ANGLE
  struct_SSRinstance sBoilderSSRdrv;
#endif
#if SSR_CTRL_BOILER_HEAT == ZERO_CROSS
  volatile struct_SSR_ZC_instance sBoilderSSRzc;
#endif
struct_SSRinstance sPumpSSRdrv;
struct_OnOffSSRinstance sSolenoidSSRdrv;


 //*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************
void fcn_initSSRController(struct_SSRcontroller * ptr_instance);
void fcn_createSSRinstance(struct_SSRinstance * ptr_instance);
void fcn_createOnOffSSRinstance(struct_OnOffSSRinstance *ptr_instance);

void fcn_boilerSSR_ctrlUpdate(void);
void fcn_SSR_pwrUpdate(struct_SSRinstance * ptr_instance, uint16_t outputPower);

void fcn_pumpSSR_ctrlUpdate(void);
void fcn_SSR_ctrlUpdate(struct_SSRinstance * ptr_instance);

//*****************************************************************************
//
//			ISR HANDLERS FUCNTIONS
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	isr_BoilderSSR_EventHandler
 * Description: Controls the SSR timing to trigger SSR 
 *****************************************************************************/
#if SSR_CTRL_BOILER_HEAT == ANGLE
void isr_BoilderSSR_EventHandler(nrf_timer_event_t event_type, void* p_context)
{
    //nrf_drv_timer_pause((nrfx_timer_t const *)&sBoilderSSRdrv.hwTmr);
    //nrf_drv_timer_clear((nrfx_timer_t const *)&sBoilderSSRdrv.hwTmr);
      switch (event_type)
      {
          case NRF_TIMER_EVENT_COMPARE0:
              nrf_drv_gpiote_out_set(sBoilderSSRdrv.out_SSRelay);
              break;
          default:
              //Do nothing.
              break;
      }
}
#endif

/*****************************************************************************
 * Function: 	isr_PumpSSR_EventHandler
 * Description: Controls the SSR timing to trigger SSR 
 *****************************************************************************/
void isr_PumpSSR_EventHandler(nrf_timer_event_t event_type, void* p_context)
{
      switch (event_type)
      {
          case NRF_TIMER_EVENT_COMPARE0:
              nrf_drv_gpiote_out_set(sPumpSSRdrv.out_SSRelay);
              break;
          default:
              //Do nothing.
              break;
      }
}

/*****************************************************************************
 * Function: 	isr_ZeroCross_EventHandler
 * Description: This event is asserted when AC is close/is at to 0V
 * Parameters:	
 * Return:
 *****************************************************************************/
void isr_ZeroCross_EventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  /* LOGIC BLOCK FOR BOILER SSR */
  #if SSR_CTRL_BOILER_HEAT == ANGLE
    if(sBoilderSSRdrv.ssrPWRstatus == SSR_MIDPWR)
    {
      if( sBoilderSSRdrv.smTrigStatus == smS_Release )
      {
        /* Disengage TRIAC/SSR  */
        nrf_drv_gpiote_out_clear(sBoilderSSRdrv.out_SSRelay);
        /* Update timers with new count/power percentage  */
        /* This function re-enables the timer if we enter from a 0% or 100% state */
        fcn_SSR_ctrlUpdate((struct_SSRinstance *)&sBoilderSSRdrv);
        /* Reset State machine for this new cycle  */
        sBoilderSSRdrv.smTrigStatus = smS_Engage;
      }else{
        /* A semi-cycle or 180deg phase as passed; driver continues engaging the SSR  */
        /* but prepare the state machine to update in the next semi-cyle  */
        sBoilderSSRdrv.smTrigStatus = smS_Release;
      }
    }else if(sBoilderSSRdrv.ssrPWRstatus == SSR_FULLPWR)
    {
      /* When SSR has to deliver 100% of power:  */ 
      /* driver will disable the timer and set 1 the output */
      nrf_drv_gpiote_out_set(sBoilderSSRdrv.out_SSRelay);
      nrf_drv_timer_disable(&sBoilderSSRdrv.hwTmr );  
    }else if(sBoilderSSRdrv.ssrPWRstatus == SSR_NOPWR)
    {
      /* When SSR has to deliver 0% of power:  */ 
      /* driver will disable the timer and set 0 the output */
      nrf_drv_gpiote_out_clear(sBoilderSSRdrv.out_SSRelay);
      nrf_drv_timer_disable(&sBoilderSSRdrv.hwTmr );  
    }
  #endif
  #if SSR_CTRL_BOILER_HEAT == ZERO_CROSS
    sBoilderSSRzc.cntPWRcycles++;
    if(sBoilderSSRzc.ssrPWRstatus == SSR_BELOWMIDPWR || sBoilderSSRzc.ssrPWRstatus == SSR_ABOVEMIDPWR)
    {
      if(sBoilderSSRzc.cntPWRcycles <= sBoilderSSRzc.srrPowerCnt)
      {
         if(sBoilderSSRzc.cntPWRcycles % 2)
         {
            nrf_drv_gpiote_out_clear(sBoilderSSRzc.out_SSRelay);
         }else{
            nrf_drv_gpiote_out_set(sBoilderSSRzc.out_SSRelay);
         }
      }else if(sBoilderSSRzc.ssrPWRstatus == SSR_ABOVEMIDPWR)
      {
         nrf_drv_gpiote_out_set(sBoilderSSRzc.out_SSRelay);
      }else{
        nrf_drv_gpiote_out_clear(sBoilderSSRzc.out_SSRelay);
      }
    }else if(sBoilderSSRzc.ssrPWRstatus == SSR_FULLPWR)
      {
        // When SSR has to deliver 100% of power:  
        nrf_drv_gpiote_out_set(sBoilderSSRzc.out_SSRelay);
      }else if(sBoilderSSRzc.ssrPWRstatus == SSR_NOPWR)
        {
          // When SSR has to deliver 0% of power: 
          nrf_drv_gpiote_out_clear(sBoilderSSRzc.out_SSRelay);
        }else{}
    //Reset Counter
    #if SSR_CTRL_BOILER_RATE == FREQ_1HZ_1PER
    if(sBoilderSSRzc.cntPWRcycles > 100)
    {
      sBoilderSSRzc.cntPWRcycles = 0;
    }
    #endif
    #if SSR_CTRL_BOILER_RATE == FREQ_2HZ_1PER
    if(sBoilderSSRzc.cntPWRcycles > 50)
    {
      sBoilderSSRzc.cntPWRcycles = 0;
    }
    #endif
  #endif 

  /* LOGIC BLOCK FOR PUMP SSR */
  if(sPumpSSRdrv.ssrPWRstatus == SSR_MIDPWR)
  {
    if( sPumpSSRdrv.smTrigStatus == smS_Release )
    {
      /* Disengage TRIAC/SSR  */
      nrf_drv_gpiote_out_clear(sPumpSSRdrv.out_SSRelay);
      /* Update timers with new count/power percentage  */
      /* This function re-enables the timer if we enter from a 0% or 100% state */
      fcn_SSR_ctrlUpdate((struct_SSRinstance *)&sPumpSSRdrv);
      /* Reset State machine for this new cycle  */
      sPumpSSRdrv.smTrigStatus = smS_Engage;
    }else{
      /* A semi-cycle or 180deg phase as passed; driver continues engaging the SSR  */
      /* but prepare the state machine to update in the next semi-cyle  */
      sPumpSSRdrv.smTrigStatus = smS_Release;
    }
  }else if(sPumpSSRdrv.ssrPWRstatus == SSR_FULLPWR)
  {
    /* When SSR has to deliver 100% of power:  */ 
    /* driver will disable the timer and set 1 the output */
    nrf_drv_gpiote_out_set(sPumpSSRdrv.out_SSRelay);
    nrf_drv_timer_disable(&sPumpSSRdrv.hwTmr );  
  }else if(sPumpSSRdrv.ssrPWRstatus == SSR_NOPWR)
  {
    /* When SSR has to deliver 0% of power:  */ 
    /* driver will disable the timer and set 0 the output */
    nrf_drv_gpiote_out_clear(sPumpSSRdrv.out_SSRelay);
    nrf_drv_timer_disable(&sPumpSSRdrv.hwTmr );  
  }

  /* LOGIC BLOCK FOR SOLENOID SSR */
  if(sSolenoidSSRdrv.ssrState == SSR_STATE_ENGAGE)
  {
    sSolenoidSSRdrv.ssrState = SSR_STATE_ACTIVE;
    nrf_drv_gpiote_out_set(sSolenoidSSRdrv.out_SSRelay);
  }else{}
}


//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	fcn_initSSRController_BLEspresso
 * Description: This function encapsulate public variable create before for 
 *              this driver.
 *              Calling this function simplfies reading in the main loop and 
 *              will make easy debugging
 *
 *****************************************************************************/
ssr_status_t fcn_initSSRController_BLEspresso(void)
{
  /*  Init SSR for the boier's resistance heater  */
  #if SSR_CTRL_BOILER_HEAT == ANGLE
    sBoilderSSRdrv.hwTmr            = (nrf_drv_timer_t)NRF_DRV_TIMER_INSTANCE(1);
    sBoilderSSRdrv.hwTmr_isr_handler= isr_BoilderSSR_EventHandler;
    sBoilderSSRdrv.out_SSRelay      = outSSRboiler_PIN;
    sBoilderSSRdrv.ssrPWRstatus     = SSR_NOPWR;
    fcn_createSSRinstance((struct_SSRinstance *)&sBoilderSSRdrv);
  #endif
  #if SSR_CTRL_BOILER_HEAT == ZERO_CROSS
    sBoilderSSRzc.out_SSRelay       = outSSRboiler_PIN;
    sBoilderSSRzc.srrPowerCnt       = 0;
    sBoilderSSRzc.ssrPWRstatus      = SSR_NOPWR;
    //Only GPIO is ocnfigured as Ouput
    //No need to cfg a timer and its ISR handler.
    ret_code_t err_code_gpio;
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
    err_code_gpio = nrf_drv_gpiote_out_init(sBoilderSSRzc.out_SSRelay, &out_config);
    APP_ERROR_CHECK(err_code_gpio);
  #endif
  /*  Init SSR for the pump's motor */
  sPumpSSRdrv.hwTmr                 = (nrf_drv_timer_t)NRF_DRV_TIMER_INSTANCE(2);
  sPumpSSRdrv.hwTmr_isr_handler     = isr_PumpSSR_EventHandler;
  sPumpSSRdrv.out_SSRelay           = outSSRpump_PIN;
  sPumpSSRdrv.ssrPWRstatus          = SSR_NOPWR;
  fcn_createSSRinstance((struct_SSRinstance *)&sPumpSSRdrv);
  /*  Init input to detect AC cross-zero  */
  sSSRcontroller.in_zCross          = inZEROCROSS_PIN;
  sSSRcontroller.zcross_isr_handler = isr_ZeroCross_EventHandler;
  fcn_initSSRController((struct_SSRcontroller *)&sSSRcontroller);
  /*  Init SSR's Pin that will drive Solenoid */
  sSolenoidSSRdrv.out_SSRelay       = enSolenoidRelay_PIN;
  sSolenoidSSRdrv.ssrState          = SSR_STATE_OFF;
  sSolenoidSSRdrv.ssrPWRstatus      = SSR_NOPWR;
  fcn_createOnOffSSRinstance(&sSolenoidSSRdrv);
  return SSR_DRV_INIT_OK;
}

/*****************************************************************************
 * Function: 	fcn_boilerSSR_pwrUpdate
 * Description: This function encapsulate public function that use public variables
 *              (now moved to prtivate). will makes it easy to read.
 *
 *****************************************************************************/
void fcn_boilerSSR_pwrUpdate( uint16_t outputPower)
{
  #if SSR_CTRL_BOILER_HEAT == ANGLE
    fcn_SSR_pwrUpdate((struct_SSRinstance *)&sBoilderSSRdrv, outputPower);
  #endif
  #if SSR_CTRL_BOILER_HEAT == ZERO_CROSS
    if(outputPower > 0 && outputPower <500)
    {
      #if SSR_CTRL_BOILER_RATE == FREQ_1HZ_1PER
      sBoilderSSRzc.srrPowerCnt = 2*(outputPower/10);
      #endif
      #if SSR_CTRL_BOILER_RATE == FREQ_2HZ_1PER
      sBoilderSSRzc.srrPowerCnt = (outputPower/10);
      #endif
      sBoilderSSRzc.ssrPWRstatus = SSR_BELOWMIDPWR;
    }else if(outputPower >= 500 && outputPower < 1000)
      {
        #if SSR_CTRL_BOILER_RATE == FREQ_1HZ_1PER
        sBoilderSSRzc.srrPowerCnt = 2*(100 - (outputPower/10)); 
        #endif
        #if SSR_CTRL_BOILER_RATE == FREQ_2HZ_1PER
        sBoilderSSRzc.srrPowerCnt = (100 - (outputPower/10));
        #endif
        sBoilderSSRzc.ssrPWRstatus = SSR_ABOVEMIDPWR;
      }else if(outputPower >= 1000)
        {
          #if SSR_CTRL_BOILER_RATE == FREQ_1HZ_1PER || SSR_CTRL_BOILER_RATE == FREQ_2HZ_1PER
          sBoilderSSRzc.srrPowerCnt = 0;
          sBoilderSSRzc.ssrPWRstatus = SSR_FULLPWR;
          #endif
        }else if(outputPower == 0)
          {
            #if SSR_CTRL_BOILER_RATE == FREQ_1HZ_1PER || SSR_CTRL_BOILER_RATE == FREQ_2HZ_1PER
            sBoilderSSRzc.srrPowerCnt = 0;
            sBoilderSSRzc.ssrPWRstatus = SSR_NOPWR;
            #endif
          }else{}
  #endif
}

/*****************************************************************************
 * Function: 	fcn_pumpSSR_pwrUpdate
 * Description: This function encapsulate public function that use public variables
 *              (now moved to prtivate). will makes it easy to read.
 *
 *****************************************************************************/
void fcn_pumpSSR_pwrUpdate( uint16_t outputPower)
{
    fcn_SSR_pwrUpdate((struct_SSRinstance *)&sPumpSSRdrv, outputPower);
}

/*****************************************************************************
 * Function: 	fcn_SolenoidSSR_On
 * Description: waits for an AC wave zero-cross event occuers to enable SSR
 *
 *****************************************************************************/
void fcn_SolenoidSSR_On(void)
{
  nrf_drv_gpiote_out_set(enSolenoidRelay_PIN);
  sSolenoidSSRdrv.ssrState = SSR_STATE_ENGAGE;
}

ssr_status_t get_SolenoidSSR_State(void)
{
  return (ssr_status_t)sSolenoidSSRdrv.ssrState;
}

/*****************************************************************************
 * Function: 	fcn_SolenoidSSR_Off
 * Description: Disable SSR's pin instantly, SSR will disengage in the next AC wave zero-cross
 *
 *****************************************************************************/
void fcn_SolenoidSSR_Off(void)
{
  nrf_drv_gpiote_out_clear(enSolenoidRelay_PIN);
  sSolenoidSSRdrv.ssrState = SSR_STATE_OFF;
}

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	fcn_boilerSSR_ctrlUpdate
 * Description: This function encapsulate public function that use public variables
 *              (now moved to prtivate). will makes it easy to read.
 *
 *****************************************************************************/
void fcn_boilerSSR_ctrlUpdate(void)
{
  #if SSR_CTRL_BOILER_HEAT == ANGLE
    fcn_SSR_ctrlUpdate((struct_SSRinstance *)&sBoilderSSRdrv);
  #endif
}

/*****************************************************************************
 * Function: 	fcn_pumpSSR_ctrlUpdate
 * Description: This function encapsulate public function that use public variables
 *              (now moved to prtivate). will makes it easy to read.
 *
 *****************************************************************************/
void fcn_pumpSSR_ctrlUpdate(void)
{
    fcn_SSR_ctrlUpdate((struct_SSRinstance *)&sPumpSSRdrv);
}

/*****************************************************************************
 * Function: 	fcn_initSSRController
 * Description: This function will init GPIO for: ZeroCross-Input
                Init the external interrupt for AC zero-crossing
 * Caveats:     optimize for 50Hz
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_initSSRController(struct_SSRcontroller * ptr_instance)
{
    ret_code_t err_code_gpio;
    //Zero Cross input with external interrupt enable
    //------------------------------------------------------------------------
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_NOPULL;
    in_config.sense = GPIOTE_CONFIG_POLARITY_LoToHi;    //Use this line when there is Schimtt gate before
    //in_config.sense = GPIOTE_CONFIG_POLARITY_HiToLo;  //Use this line when there is only an optocoupler.
    err_code_gpio = nrf_drv_gpiote_in_init(ptr_instance->in_zCross,
                                       &in_config, 
                                       ptr_instance->zcross_isr_handler);
    APP_ERROR_CHECK(err_code_gpio);
    nrf_drv_gpiote_in_event_enable(ptr_instance->in_zCross, true);
}

/*****************************************************************************
 * Function: 	fcn_createSSRinstance
 * Description: This function will init. GPIO for:  SSR-Output.
                It init HW-timer to control SSrelay trigger.
                It contains the functions to drive SSrelay from 0% to 100% AC cycle.
 * Caveats:     optimize for 50Hz
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_createSSRinstance(struct_SSRinstance * ptr_instance)
{
    ptr_instance->smTrigStatus            = smS_Release;
    ptr_instance->sSRR_timing_us.tStep    = POWER_MAX_VALUE/AC_PERCENT_STEP;
    ptr_instance->sSRR_timing_us.tZCdelay = ((POWER_MAX_VALUE/AC_PERCENT_STEP)
                                                      *T_ZEROCROSS_DELAY_DET);
    ptr_instance->sSRR_timing_us.tTrigger = 0;  //50% of power
    ptr_instance->sSRR_timing_us.tPeriod  = AC_CYCLE_PERIOD;
    ptr_instance->ssrPWRstatus            = SSR_NOPWR; 
    //TIMER SECTION
    //------------------------------------------------------------------------------
    uint32_t err_code = NRF_SUCCESS;
    //Configure TIMER_HW intance
    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
    timer_cfg.frequency = NRF_TIMER_FREQ_8MHz;
    err_code = nrf_drv_timer_init(&ptr_instance->hwTmr, &timer_cfg, ptr_instance->hwTmr_isr_handler);
    APP_ERROR_CHECK(err_code);

    //ptr_instance->sSRR_timing_us.cZCdelay = nrf_drv_timer_us_to_ticks(&ptr_instance->hwTmr, ptr_instance->sSRR_timing_us.tZCdelay );
    ptr_instance->sSRR_timing_us.cTrigger = nrf_drv_timer_us_to_ticks(&ptr_instance->hwTmr, ptr_instance->sSRR_timing_us.tTrigger );
    ptr_instance->sSRR_timing_us.cPeriod = nrf_drv_timer_us_to_ticks(&ptr_instance->hwTmr, ptr_instance->sSRR_timing_us.tPeriod );

    //nrf_drv_timer_compare(&ptr_instance->hwTmr,NRF_TIMER_CC_CHANNEL0,ptr_instance->sSRR_timing_us.cTrigger,true);
    //nrf_drv_timer_extended_compare(&ptr_instance->hwTmr, NRF_TIMER_CC_CHANNEL1, ptr_instance->sSRR_timing_us.cPeriod, NRF_TIMER_SHORT_COMPARE1_STOP_MASK, true);
    nrf_drv_timer_extended_compare(&ptr_instance->hwTmr, 
                                   NRF_TIMER_CC_CHANNEL0,
                                   ptr_instance->sSRR_timing_us.cTrigger, 
                                   NRF_TIMER_SHORT_COMPARE0_STOP_MASK,
                                   true);
    //Timer initialize but not enable

    //GPIOS SECTION
    //------------------------------------------------------------------------------
    ret_code_t err_code_gpio;
    //err_code_gpio = nrf_drv_gpiote_init(); -> Already init. in ac_inputs_drv
    //Output pin to control SSR
    //------------------------------------------------------------------------
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
    err_code_gpio = nrf_drv_gpiote_out_init(ptr_instance->out_SSRelay, &out_config);
    APP_ERROR_CHECK(err_code_gpio);
    //err_code_gpio = nrf_drv_gpiote_out_init(31, &out_config);
    //APP_ERROR_CHECK(err_code_gpio);
}

/*****************************************************************************
 * Function: 	fcn_createOnOffSSRinstance
 * Description: init GPIO as output that will drive a SSR in ON/OFF fashion
 * Return:
 *****************************************************************************/
void fcn_createOnOffSSRinstance(struct_OnOffSSRinstance *ptr_instance)
{
  //GPIOS SECTION TO CTRL SOLENOID RELAY
  //------------------------------------------------------------------------------
  ret_code_t err_code_gpio;
  //nrf_drv_gpiote_init -> Already init. in ac_inputs_drv
  //Output pin to control SSR
  //------------------------------------------------------------------------
  nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
  err_code_gpio = nrf_drv_gpiote_out_init(ptr_instance->out_SSRelay, &out_config);
  APP_ERROR_CHECK(err_code_gpio);

  nrf_drv_gpiote_out_clear(ptr_instance->out_SSRelay);
}

/*****************************************************************************
 * Function: 	fcn_SSR_Controller
 * Description: 
 * Caveats:
 * Parameters:	outputPower range: 0% - 100% = 0 - 1000
 * Return:
 *****************************************************************************/
void fcn_SSR_pwrUpdate(struct_SSRinstance * ptr_instance, uint16_t outputPower)
{
    if(outputPower > 0 && outputPower <1000)
    {
      ptr_instance->srrPower = outputPower;
      ptr_instance->ssrPWRstatus = SSR_MIDPWR;
      ptr_instance->sSRR_timing_us.tTrigger = ptr_instance->sSRR_timing_us.tPeriod - 
                                              ( ptr_instance->sSRR_timing_us.tStep * outputPower);
      //tDelay is inserted here: -( ptr_instance->sSRR_timing_us.tStep * tDelayPWR); or - ptr_instance->sSRR_timing_us.tZCdelay;
      
      ptr_instance->sSRR_timing_us.cTrigger = nrf_drv_timer_us_to_ticks(&ptr_instance->hwTmr, 
                                              ptr_instance->sSRR_timing_us.tTrigger );
      //nrfx_timer_clear(&ptr_instance->hwTmr); /*Testing Code line*/
      nrf_drv_timer_enable(&ptr_instance->hwTmr);
      nrf_drv_gpiote_in_event_enable(ptr_instance->in_zCross, true);
    }else{
        if(outputPower >= 1000)
        {
            ptr_instance->srrPower = 1000;
            ptr_instance->ssrPWRstatus = SSR_FULLPWR;
            nrf_drv_gpiote_out_set(ptr_instance->out_SSRelay); 
        }else{
          if(outputPower == 0)
          {
            ptr_instance->srrPower = 0;
            ptr_instance->ssrPWRstatus = SSR_NOPWR;
            nrf_drv_gpiote_out_clear(ptr_instance->out_SSRelay); 
          }else{}
        }
    }
}

/*****************************************************************************
 * Function: 	fcn_SSR_ctrlUpdate
 * Description: Stops the timer, updates new top count in the timer, finally re-enables it.
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_SSR_ctrlUpdate(struct_SSRinstance * ptr_instance)
{
    nrf_drv_timer_disable(&ptr_instance->hwTmr);
    //nrfx_timer_clear(&ptr_instance->hwTmr); /*Testing Code line*/
    nrf_drv_timer_extended_compare(&ptr_instance->hwTmr, 
                                    NRF_TIMER_CC_CHANNEL0, 
                                    ptr_instance->sSRR_timing_us.cTrigger, 
                                    NRF_TIMER_SHORT_COMPARE0_STOP_MASK, 
                                    true);
    nrf_drv_timer_enable(&ptr_instance->hwTmr);
}


/*
nrf_drv_gpiote_out_toggle(sSSRdrvConfig.out_SSRelay);
nrf_drv_gpiote_out_clear(sSSRdrvConfig.out_SSRelay);
nrf_drv_gpiote_out_set(sSSRdrvConfig.out_SSRelay);
*/