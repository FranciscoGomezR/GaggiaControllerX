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
#include "spi_Devices.h"

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
      fcn_ACinput_drv();
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

/*****************************************************************************
* Function: 	acinSteam_eventHandler
* Description: Count number of AC cycle when swicth is active 
* Definition: ac_inputs_drv.h
*****************************************************************************/

/*****************************************************************************
* Function: 	acinBrew_eventHandler
* Description: Count number of AC cycle when swicth is active  
* Definition: ac_inputs_drv.h
*****************************************************************************/

/*****************************************************************************
 * Function: 	isr_BoilderSSR_EventHandler
 * Description: Controls the SSR timing to trigger SSR 
 * Definition: solidStateRelay_Controller.h
 *****************************************************************************/

/*****************************************************************************
 * Function: 	isr_PumpSSR_EventHandler
 * Description: Controls the SSR timing to trigger SSR
 * Definition: solidStateRelay_Controller.h 
 *****************************************************************************/

/*****************************************************************************
 * Function: 	isr_ZeroCross_EventHandler
 * Description: 
 * Definition: solidStateRelay_Controller.h 
 *****************************************************************************/

/*****************************************************************************
 * Function: 	isr_HwTmr3_Period_EventHandler
 * Description: 
 * Definition: TempController.h 
 *****************************************************************************/

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
  ret_code_t err_code;
  volatile uint32_t initResultFlag=0;

  #if NRF_LOG_ENABLED == 1
    log_init();
  #endif

  ReadSensors_flag=false;
  PrintTask_flag=false;
  

  //  GPIO DRIVER init
  //---------------------------------------------------------------------------
  if (!nrf_drv_gpiote_is_init())
  {
      err_code = nrf_drv_gpiote_init();
      if (err_code != NRF_SUCCESS)
      {
          return NRF_ERROR_INTERNAL;
      }
  }
  APP_ERROR_CHECK(err_code);

  timers_init();
  spim_init();

  //initResultFlag = spim_initNVmemory();
  initResultFlag = stgCtrl_Init();
  #if NRF_LOG_ENABLED == 1
    if( initResultFlag == NVM_INIT_OK)
    { 
      NRF_LOG_DEBUG("NVM INIT SPI-NV memory ::SUCCESFUL::");
    }else{
      NRF_LOG_DEBUG("NVM INIT SPI-NV memory ::FAILED::");
    }
    NRF_LOG_FLUSH();
  #endif

  initResultFlag = stgCtrl_ChkForUserData();
  #if NRF_LOG_ENABLED == 1
    if( initResultFlag == STORAGE_USERDATA_LOADED)
    { 
      NRF_LOG_DEBUG("NVM INIT ::HAS DATA::");
    }else{
      NRF_LOG_DEBUG("NVM INIT ::IS EMPTY::");
    }
    NRF_LOG_FLUSH();
  #endif
  initResultFlag = stgCtrl_ReadUserData((bleSpressoUserdata_struct*)&BLEspressoVar);
  #if NRF_LOG_ENABLED == 1
    if( initResultFlag == STORAGE_USERDATA_LOADED)
    { 
      NRF_LOG_DEBUG("NVM INIT ::Data Loaded::");
    }else{
      NRF_LOG_DEBUG("NVM INIT ::No     Data::");
    }
    NRF_LOG_FLUSH();
  #endif

  initResultFlag = spim_initRTDconverter();
  #if NRF_LOG_ENABLED == 1
  if( initResultFlag == TMP_INIT_OK)
  { 
    NRF_LOG_DEBUG("NVM INIT SPI-Temperature Sensor ::SUCCESFUL::");
  }else{
    NRF_LOG_DEBUG("NVM INIT SPI-Temperature Sensor ::FAILED::");
  }
  NRF_LOG_FLUSH();
  #endif

  initResultFlag = fcn_initDC12Voutput_drv();
  #if NRF_LOG_ENABLED == 1
  if( initResultFlag == DRV_12VO_INIT_AS_LAMP)
  { 
    NRF_LOG_DEBUG("DRV INIT 12VDC output ::SUCCESFUL::");
  }else{
    NRF_LOG_DEBUG("NVM INIT 12VDC output ::FAILED::");
  }
  NRF_LOG_FLUSH();
  #endif

  initResultFlag = fcn_initACinput_drv();
  #if NRF_LOG_ENABLED == 1
  if( initResultFlag == DRV_AC_INPUT_INIT_OK)
  { 
    NRF_LOG_DEBUG("DRV INIT AC Inputs ::READY::");
  }else{
    NRF_LOG_DEBUG("DRV INIT AC Inputs ::FAILED::");
  }
  NRF_LOG_FLUSH();
  #endif
  
  initResultFlag = fcn_initSSRController_BLEspresso();
  #if NRF_LOG_ENABLED == 1
    if( initResultFlag == SSR_DRV_INIT_OK)
    { 
      NRF_LOG_DEBUG("DRV INIT Solid State Relays ::READY::");
    }else{
      NRF_LOG_DEBUG("DRV INIT Solid State Relays ::FAILED::");
    }
    NRF_LOG_FLUSH();
  #endif

  fcn_initTemperatureController();
 // fcn_startTemperatureController();
  #if NRF_LOG_ENABLED == 1
    NRF_LOG_DEBUG("DRV INIT: PID Boiler Controller");
    NRF_LOG_FLUSH();
  #endif

  fcn_initPumpController();
  #if NRF_LOG_ENABLED == 1
    NRF_LOG_DEBUG("DRV INIT: Pump Controller");
    NRF_LOG_FLUSH();
  #endif

  /* I don't know what this fcn is for  */
  fcn_initBLEspressoHwInterface_drv();

  /* swection of code to control LED on the board*/
  /***********************************************/
  ret_code_t err_code_gpio;
  nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
  err_code_gpio = nrf_drv_gpiote_out_init(29, &out_config);
  APP_ERROR_CHECK(err_code_gpio);

  BLEspressoVar.TargetBoilerTemp = 20.00f;
  BLEspressoVar.BrewPreInfussionPwr = 55.55f;
  BLEspressoVar.Pid_Gain_term = 1.111f;
  
  // Start Bluetooth execution.
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
        ble_update_boilerWaterTemp(f_getBoilerTemperature());
        BLEspressoVar.ActualBoilerTemp=(float)f_getBoilerTemperature();
        #if NRF_LOG_ENABLED == 1
          NRF_LOG_INFO("\033[0;36m Temp: 10," NRF_LOG_FLOAT_MARKER "\r\n \033[0;40m", NRF_LOG_FLOAT(f_getBoilerTemperature()));
        #endif
      }else{}
      
      if(OneSecond_flag == true)
      {
        OneSecond_flag = false;
        ssrPower = ssrPower +50;
        if(ssrPower>1009)
        {ssrPower=0;}
        //fcn_SSR_pwrUpdate((struct_SSRinstance *)&sBoilderSSRdrv, ssrPower);
        fcn_boilerSSR_pwrUpdate(ssrPower);
        //fcn_pumpSSR_pwrUpdate(ssrPower+50);
        NRF_LOG_INFO("\033[0;33m Heat Power: \033[0;40m \033[0;43m" NRF_LOG_FLOAT_MARKER "\033[0;40m \r\n", NRF_LOG_FLOAT((float)ssrPower/10.0f));
        //NRF_LOG_INFO("\033[0;33m Pump Power: \033[0;40m \033[0;43m" NRF_LOG_FLOAT_MARKER "\033[0;40m \r\n", NRF_LOG_FLOAT((float)ssrPump/10.0f));
        /* this section is to run trials on the PID loop
        if(sPIDtimer.status == NOT_ACTIVE)
        {
            if(f_getBoilerTemperature()>0.0f)
            {
                fcn_startTemperatureController();
            }else{}
        }else{}
        */
        //fcn_updateTemperatureController();
      }else{}

      if(fcn_StatusChange_Brew() == AC_INPUT_CHANGE)
      {
          if(fcn_GetInputStatus_Brew() == AC_INPUT_ASSERTED )
          {
            #if NRF_LOG_ENABLED == 1
              NRF_LOG_INFO("BREW-> \033[0;42m ON \033[0;40m \r\n ");
            #endif
          }else{
            #if NRF_LOG_ENABLED == 1
              NRF_LOG_INFO("BREW-> \033[0;43m OFF \033[0;40m \r\n ");
            #endif
          }
          fcn_StatusReset_Brew();
      }else{}

      if(fcn_StatusChange_Steam() == AC_INPUT_CHANGE)
      {
          if(fcn_GetInputStatus_Steam() == AC_INPUT_ASSERTED )
          {
            #if NRF_LOG_ENABLED == 1
              NRF_LOG_INFO("STEAM-> \033[0;42m ON \033[0;40m \r\n");
            #endif
          }else{
            #if NRF_LOG_ENABLED == 1
              NRF_LOG_INFO("STEAM-> \033[0;43m OFF \033[0;40m \r\n");
            #endif
          }
          fcn_StatusReset_Steam();
      }else{}
      //idle_state_handle();
      

      if(PumpCtrl_flag == true)
      {
          PumpCtrl_flag = false;
          
          switch(smPumpCtrl.smPump.sRunning)
          {
            case 1:  //st_RampupToPreInf
              #if NRF_LOG_ENABLED == 1
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42mStarting Pre-Infussion\033[0;40m \r\n");
              #endif
            break;
            case 2: //st_PreInfussion
              #if NRF_LOG_ENABLED == 1
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42mPre-Infussion\033[0;40m \r\n");
              #endif
            break;
            case 3: //st_RampupToBrew
              #if NRF_LOG_ENABLED == 1
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42mStarting Brew\033[0;40m \r\n");
              #endif
            break;
            case 4: //st_BrewPressure
              #if NRF_LOG_ENABLED == 1
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42mBrewing\033[0;40m \r\n");
              #endif
            break;
            default:
            break;
          }
          fcn_PumpStateDriver();
      }else{}
      
      if(LightSeq_flag == true)
      {
          LightSeq_flag = false;
          //sm_DC12Voutput_drv(&s12Vout);
      }else{}

      if(flg_BrewCfg == 1 || flg_PidCfg == 1)
      {
        flg_BrewCfg = 0;
        flg_PidCfg = 0;
        //fcn_WriteParameterNVM((bleSpressoUserdata_struct *)&BLEspressoVar);
        #if NRF_LOG_ENABLED == 1
          NRF_LOG_INFO("\033[0;44m <FDS> \033[0;40m" "\033[0;34m Write new data into FLASH block \033[0;40m \r\n");
        #endif
      }else{}

      if(flg_ReadCfg == 1 )
      {
        flg_ReadCfg = 0;
        //fcn_Read_ParameterNVM((bleSpressoUserdata_struct *)&int_NvmData);
        #if NRF_LOG_ENABLED == 1
          NRF_LOG_INFO("\033[0;44m <FDS> \033[0;40m" "\033[0;34m Read data from FLASH block data \033[0;40m \r\n");
        #endif
      }else{}

      #if NRF_LOG_ENABLED == 1
      NRF_LOG_FLUSH();
      #endif
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
  nrf_gpio_cfg_output(enSolenoidRelay_PIN);
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
  nrf_gpio_pin_write(enSolenoidRelay_PIN, state);
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

    

    