// Useful Links
// SES porject: https://www.youtube.com/watch?v=xQwX3yEcAEk&t=8961s&ab_channel=nrf5dev 
// Possible solution to SEs getting frozen: https://forum.segger.com/index.php/Thread/5576-SOLVED-Segger-Studio-frozen-on-Building-after-failed-attempted-at-GIT/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "boards.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "bluetooth_drv.h"
#include "BLEspressoServices.h"
#include "StorageController.h"

#include "log_drv.h"
#include "board_comp_drv.h"
#include "nrf_drv_timer.h"

#include "app_timer.h"
#include "i2c_sensors.h"
#include "spi_sensors.h"

#include "tempController.h"
#include "PumpController.h"
#include "ac_inputs_drv.h"
#include "solidStateRelay_Controller.h"
#include "dc12Vouput_drv.h"
#include "nrf_delay.h"
#include "app_error.h"


  volatile bool PrintTask_flag;
  volatile bool ReadSensors_flag;
  volatile bool OneSecond_flag;
  volatile bool LightSeq_flag;
  volatile bool PumpCtrl_flag;
  volatile uint16_t ssrPower=0;
  volatile uint16_t ssrPump=0;

  //  Create a timer identifier and statically allocate memory for the timer
  APP_TIMER_DEF(READTEMPSENSORS_TMR_ID);
  APP_TIMER_DEF(PRINTTEMPVALUES_TMR_ID);
  APP_TIMER_DEF(ONE_AC_CYCLE____TMR_ID);
  APP_TIMER_DEF(ONE_SECOND______TMR_ID);
  APP_TIMER_DEF(PUMP_CONTROLLER_TMR_ID);
  APP_TIMER_DEF(LIGHT_SEQ_______TMR_ID);

  //  Convert milliseconds to timer ticks.
  #define READTEMPSENSORS_TICKS   APP_TIMER_TICKS(200)
  #define PRINTTEMPVALUES_TICKS   APP_TIMER_TICKS(1000)
  #define ONE_AC_CYCLE____TICKS   APP_TIMER_TICKS(100)
  #define ONE_SECOND______TICKS   APP_TIMER_TICKS(1003)
  #define PUMP_CONTROLER__TICKS   APP_TIMER_TICKS(250)
  #define LIGHT_SEQ_______TICKS   APP_TIMER_TICKS(63)

  void fcn_initBLEspressoHwInterface_drv(void);
  void fcn_enSelenoidHwInterface_drv(uint32_t state);
  void fcn_en12VDCoutHwInterface_drv(uint32_t state);


/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
  static void ReadTempSns_swTmrHandler(void * p_context)
  {
    ReadSensors_flag = true;
  }

  static void PrintTempVal_swTmrHandler(void * p_context)
  {
    PrintTask_flag = true;
  }

  static void OneACcycle_swTmrHandler(void * p_context)
  {
      //bsp_board_led_invert(0);bsp_board_led_invert(1);
      fcn_ACinput_drv(&sControllerInputs);
  }

  static void OneSecond_swTmrHandler(void * p_context)
  {
      OneSecond_flag = true;
  }

  static void LightSeq_swTmrHandler(void * p_context)
  {
      LightSeq_flag = true;
  }

  static void PumpCtrl_swTmrHandler(void * p_context)
  {
      PumpCtrl_flag = true;
  }

  void acinSteam_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
  {
      sControllerInputs.Steam.Counter++;
  }
  void acinBrew_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
  {
      sControllerInputs.Brew.Counter++;
  }

/*****************************************************************************
 * Function: 	isr_ZeroCross_EventHandler
 * Description: 
 * Parameters:	
 * Return:
 *****************************************************************************/
void isr_ZeroCross_EventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if( sBoilderSSRdrv.smTrigStatus == smS_Release )
    {
      //nrf_drv_gpiote_out_set(31);
      fcn_SSR_ctrlUpdate((struct_SSRinstance *)&sBoilderSSRdrv);
      //nrf_drv_timer_enable((nrfx_timer_t const *)&sBoilderSSRdrv.hwTmr);
      sBoilderSSRdrv.smTrigStatus = smS_Engage;
      //nrf_drv_gpiote_out_clear(31);
      nrf_drv_gpiote_out_clear(sBoilderSSRdrv.out_SSRelay);
    }else{
      sBoilderSSRdrv.smTrigStatus = smS_Release;
    }

    if( sPumpSSRdrv.smTrigStatus == smS_Release)
    {
        if(sPumpSSRdrv.ssrPWRstatus == MIDPWR)
        {
            fcn_SSR_ctrlUpdate((struct_SSRinstance *)&sPumpSSRdrv);
            sPumpSSRdrv.smTrigStatus = smS_Engage;
            nrf_drv_gpiote_out_clear(sPumpSSRdrv.out_SSRelay);
        }else{
          if(sPumpSSRdrv.ssrPWRstatus == FULLPWR)
          {
              nrf_drv_gpiote_out_set(sPumpSSRdrv.out_SSRelay);
          }else{
              nrf_drv_gpiote_out_clear(sPumpSSRdrv.out_SSRelay);
          }
        }
    }else{
      sPumpSSRdrv.smTrigStatus = smS_Release;
    }
    /*
if(sPumpSSRdrv.status == ssrMIDPWR)
      {
        fcn_SSR_ctrlUpdate((struct_SSRinstance *)&sPumpSSRdrv);
        sPumpSSRdrv.smTrigStatus = smS_Engage;
        nrf_drv_gpiote_out_clear(sPumpSSRdrv.out_SSRelay);
      }else{
        if(sPumpSSRdrv.status == ssrFULLPWR)
        {
          nrf_drv_gpiote_out_clear(sPumpSSRdrv.out_SSRelay);
        }else{
          nrf_drv_gpiote_out_set(sPumpSSRdrv.out_SSRelay);
        }
      }
      */
}

/*****************************************************************************
 * Function: 	isr_BoilderSSR_EventHandler
 * Description: Controls the SSR timing to trigger SSR 
 *****************************************************************************/
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
 * Function: 	isr_SSRcontroller_EventHandler
 * Description: 
 * Parameters:	
 * Return:
 *****************************************************************************/
void isr_TempController_EventHandler(nrf_timer_event_t event_type, void* p_context)
{
    //nrf_drv_gpiote_out_toggle(20);
    sBoilerTempCtrl.Input = rtdTemperature;
    //sBoilerTempCtrl.SetPoint = 45.0f;
    fcn_PID_Block_Iteration((PID_Block_fStruct *)&sBoilerTempCtrl);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    // Create timers.
    /* YOUR_JOB: Create any timers to be used by the application.
                 Below is an example of how to create a timer.
                 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by
                 one.*/
       err_code = app_timer_create(&READTEMPSENSORS_TMR_ID, APP_TIMER_MODE_REPEATED, ReadTempSns_swTmrHandler);
       APP_ERROR_CHECK(err_code);
       err_code = app_timer_create(&PRINTTEMPVALUES_TMR_ID, APP_TIMER_MODE_REPEATED, PrintTempVal_swTmrHandler);
       APP_ERROR_CHECK(err_code);
       err_code = app_timer_create(&ONE_AC_CYCLE____TMR_ID, APP_TIMER_MODE_REPEATED, OneACcycle_swTmrHandler);
       APP_ERROR_CHECK(err_code);
       err_code = app_timer_create(&ONE_SECOND______TMR_ID, APP_TIMER_MODE_REPEATED, OneSecond_swTmrHandler);
       APP_ERROR_CHECK(err_code);

       err_code = app_timer_create(&PUMP_CONTROLLER_TMR_ID, APP_TIMER_MODE_REPEATED, PumpCtrl_swTmrHandler);
       APP_ERROR_CHECK(err_code);
       err_code = app_timer_create(&LIGHT_SEQ_______TMR_ID, APP_TIMER_MODE_REPEATED, LightSeq_swTmrHandler);
       APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting timers.
 */
static void application_timers_start(void)
{
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer.
       ret_code_t err_code;
       err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
       APP_ERROR_CHECK(err_code); */
    ret_code_t err_code;
    err_code = app_timer_start(READTEMPSENSORS_TMR_ID, READTEMPSENSORS_TICKS, NULL);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(PRINTTEMPVALUES_TMR_ID, PRINTTEMPVALUES_TICKS, NULL);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(ONE_AC_CYCLE____TMR_ID, ONE_AC_CYCLE____TICKS, NULL);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(ONE_SECOND______TMR_ID, ONE_SECOND______TICKS, NULL);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(PUMP_CONTROLLER_TMR_ID, PUMP_CONTROLER__TICKS, NULL);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_start(LIGHT_SEQ_______TMR_ID, LIGHT_SEQ_______TICKS, NULL);
    APP_ERROR_CHECK(err_code);
}

/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
int main(void)
{
    bool erase_bonds;
    // Initialize.
    log_init();
    ReadSensors_flag=false;
    PrintTask_flag=false;
    timers_init();
    spim_init();
    spim_initRTDconverter();
    NRF_LOG_DEBUG("DRV INIT: Temp. Sensor");
    NRF_LOG_FLUSH();

    fcn_initBLEspressoHwInterface_drv();
    fcn_initDC12Voutput_drv();
    NRF_LOG_DEBUG("DRV INIT: 12VDC output");
    NRF_LOG_FLUSH();

    sControllerInputs.Brew.pinID      = inBREW_PIN;
    sControllerInputs.Brew.Status     = false;
    sControllerInputs.Brew.smEvent    = smS_NoChange;
    sControllerInputs.Brew.nCycles    = 8;
    sControllerInputs.Brew.ext_isr_handler = acinBrew_eventHandler;
    sControllerInputs.Steam.pinID     = inSTEAM_PIN;
    sControllerInputs.Steam.Status    = false;
    sControllerInputs.Steam.smEvent   = smS_NoChange;
    sControllerInputs.Steam.nCycles    = 8;
    sControllerInputs.Steam.ext_isr_handler = acinSteam_eventHandler;
    fcn_initACinput_drv(&sControllerInputs);
    NRF_LOG_DEBUG("DRV INIT: AC Inputs");
    NRF_LOG_FLUSH();
    
    sBoilderSSRdrv.hwTmr               = (nrf_drv_timer_t)NRF_DRV_TIMER_INSTANCE(1);
    sBoilderSSRdrv.hwTmr_isr_handler   = isr_BoilderSSR_EventHandler;
    sBoilderSSRdrv.out_SSRelay         = outSSRboiler_PIN;
    fcn_createSSRinstance((struct_SSRinstance *)&sBoilderSSRdrv);
    NRF_LOG_DEBUG("DRV INIT: SSR for Boiler");
    NRF_LOG_FLUSH();

    sPumpSSRdrv.hwTmr                 = (nrf_drv_timer_t)NRF_DRV_TIMER_INSTANCE(2);
    sPumpSSRdrv.hwTmr_isr_handler     = isr_PumpSSR_EventHandler;
    sPumpSSRdrv.out_SSRelay           = outSSRpump_PIN;
    fcn_createSSRinstance((struct_SSRinstance *)&sPumpSSRdrv);
    NRF_LOG_DEBUG("DRV INIT: SSR for Pump");
    NRF_LOG_FLUSH();
  
    sSSRcontroller.in_zCross          = inZEROCROSS_PIN;
    sSSRcontroller.zcross_isr_handler = isr_ZeroCross_EventHandler;
    fcn_initSSRController((struct_SSRcontroller *)&sSSRcontroller);
    NRF_LOG_DEBUG("DRV INIT: AC Zero-cross.");
    NRF_LOG_FLUSH();

    fcn_initTemperatureController((PID_Block_fStruct *)&sBoilerTempCtrl);
   // fcn_startTemperatureController();
    NRF_LOG_DEBUG("DRV INIT: PID Boiler Controller");
    NRF_LOG_FLUSH();

    fcn_initPumpController();
    NRF_LOG_DEBUG("DRV INIT: Pump Controller");
    NRF_LOG_FLUSH();

    BLEspressoVar.TargetBoilerTemp = 20.00f;
    BLEspressoVar.BrewPreInfussionPwr = 55.55f;
    BLEspressoVar.Pid_Gain_term = 1.111f;
    
//FLASH storage example
//////////////////////////////////////////////////////////////////////////////////////////////
    fstorage_Init();
  
//////////////////////////////////////////////////////////////////////////////////////////////
    // Start execution.
    BLE_bluetooth_init();
    application_timers_start();
    advertising_start(erase_bonds);
    // Enter main loop.
    for (;;)
    {
        if(ReadSensors_flag == true)
        {
          ReadSensors_flag=false;
          spim_ReadRTDconverter();
        }else{}
        if(PrintTask_flag == true)
        {
          PrintTask_flag=false;
          ble_update_boilerWaterTemp(rtdTemperature);
          BLEspressoVar.ActualBoilerTemp=(float)rtdTemperature;
          NRF_LOG_INFO("\033[0;36m Temp: " NRF_LOG_FLOAT_MARKER "\r\n \033[0;40m", NRF_LOG_FLOAT(rtdTemperature));
        }else{}
        
        if(OneSecond_flag == true)
        {
          OneSecond_flag = false;
          //ssrPower = ssrPower +50;
          if(ssrPower>1009)
          {ssrPower=0;}
          fcn_SSR_pwrUpdate((struct_SSRinstance *)&sBoilderSSRdrv, ssrPower);
          NRF_LOG_INFO("\033[0;33m Heat Power: \033[0;40m \033[0;43m" NRF_LOG_FLOAT_MARKER "\033[0;40m \r\n", NRF_LOG_FLOAT((float)ssrPower/10.0f));
          NRF_LOG_INFO("\033[0;33m Pump Power: \033[0;40m \033[0;43m" NRF_LOG_FLOAT_MARKER "\033[0;40m \r\n", NRF_LOG_FLOAT((float)ssrPump/10.0f));
          if(sPIDtimer.status == NOT_ACTIVE)
          {
              if(rtdTemperature>0.0f)
              {
                  fcn_startTemperatureController();
              }else{}
          }else{}
        }else{}

        if(sControllerInputs.Brew.smEvent == smS_Change)
        {
            if(sControllerInputs.Brew.Status == true )
            {
              NRF_LOG_INFO("BREW-> \033[0;42m ON \033[0;40m \r\n ");
            }else{
              NRF_LOG_INFO("BREW-> \033[0;43m OFF \033[0;40m \r\n ");
            }
            sControllerInputs.Brew.smEvent = smS_NoChange; 
        }else{}
        if(sControllerInputs.Steam.smEvent == smS_Change)
        {
            if(sControllerInputs.Steam.Status == true )
            {
              NRF_LOG_INFO("STEAM-> \033[0;42m ON \033[0;40m \r\n");
            }else{
              NRF_LOG_INFO("STEAM-> \033[0;43m OFF \033[0;40m \r\n");
            }
            sControllerInputs.Steam.smEvent = smS_NoChange; 
        }else{}
        //idle_state_handle();
        

        if(PumpCtrl_flag == true)
        {
            PumpCtrl_flag = false;
            
            switch(smPumpCtrl.smPump.sRunning)
            {
              case 1:  //st_RampupToPreInf
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42mStarting Pre-Infussion\033[0;40m \r\n");
              break;
              case 2: //st_PreInfussion
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42mPre-Infussion\033[0;40m \r\n");
              break;
              case 3: //st_RampupToBrew
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42mStarting Brew\033[0;40m \r\n");
              break;
              case 4: //st_BrewPressure
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42mBrewing\033[0;40m \r\n");
              break;
              default:
              break;
            }
            fcn_PumpStateDriver();
        }else{}
        
        if(LightSeq_flag == true)
        {
            LightSeq_flag = false;
            sm_DC12Voutput_drv(&s12Vout);
        }else{}

        if(flg_BrewCfg == 1 || flg_PidCfg == 1)
        {
          flg_BrewCfg = 0;
          flg_PidCfg = 0;
          fcn_WriteParameterNVM((BLEspressoVariable_struct *)&BLEspressoVar);
          NRF_LOG_INFO("\033[0;44m <FDS> \033[0;40m" "\033[0;34m Write new data into FLASH block \033[0;40m \r\n");
          
        }else{}

        if(flg_ReadCfg == 1 )
        {
          flg_ReadCfg = 0;
          fcn_Read_ParameterNVM((BLEspressoVariable_struct *)&int_NvmData);
          NRF_LOG_INFO("\033[0;44m <FDS> \033[0;40m" "\033[0;34m Read data from FLASH block data \033[0;40m \r\n");
        }else{}

        NRF_LOG_FLUSH();
       
    }
}


/*****************************************************************************
 * Function: 	fcn_initBLEspressoHwInterface_drv
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_initBLEspressoHwInterface_drv(void)
{
  //Init output for SELENOID VALVE
  //------------------------------------------------------------------------
  nrf_gpio_cfg_output(enSelenoidRelay_PIN);
  //Init ouput for 12VDC output
  //------------------------------------------------------------------------
  //nrf_gpio_cfg_output(enDC12Voutput_PIN);
  //fcn_en12VDCoutHwInterface_drv(outSTATE_OFF);
}

/*****************************************************************************
 * Function: 	fcn_enSelenoidHwInterface_drv
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
inline void fcn_enSelenoidHwInterface_drv(uint32_t state)
{
  nrf_gpio_pin_write(enSelenoidRelay_PIN, state);
}

/*****************************************************************************
 * Function: 	fcn_en12VDCoutHwInterface_drv
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
inline void fcn_en12VDCoutHwInterface_drv(uint32_t state)
{
  nrf_gpio_pin_write(enDC12Voutput_PIN, state ^ 0x0001);
}

/*
if(sPumpSSRdrv.status == ssrMIDPWR)
      {
        fcn_SSR_ctrlUpdate((struct_SSRinstance *)&sPumpSSRdrv);
        sPumpSSRdrv.smTrigStatus = smS_Engage;
        nrf_drv_gpiote_out_clear(sPumpSSRdrv.out_SSRelay);
      }else{
        if(sPumpSSRdrv.status == ssrFULLPWR)
        {
          nrf_drv_gpiote_out_clear(sPumpSSRdrv.out_SSRelay);
        }else{
          nrf_drv_gpiote_out_set(sPumpSSRdrv.out_SSRelay);
        }
      }
      */


  /**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

    

    