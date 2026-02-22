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

volatile uint16_t ssrPower=0;
volatile uint16_t ssrPump=0;


#define EXCLUDE_NVM_SECTION         true
#define EXCLUDE_BLE_ADV_SECTION     false
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/*  Code Section:
    Ticks calculation for all sync functions
*/
#define TICK_SVCS_STEPFCN       (100 / SWTMR_TICK_MS)
#define TICK_SVCS_ESPRESSO      (100 / SWTMR_TICK_MS)
#define TICK_TASK_READBTN       (60 / SWTMR_TICK_MS)
#define TICK_TASK_BOILERTEMP    (101 / SWTMR_TICK_MS)
/*  Code Section:
    Main & fastest Tick time in the system: ** 50 ms **
    Use to control rest of sync function
*/
volatile uint32_t swTmr_tick_x0ms = 0;
typedef struct
{
  bool tf_ReadButton;
  bool tf_GetBoilerTemp;
  bool tf_svc_StepFunction;
  bool tf_svc_EspressoApp;
}struct_TaskFlg;

volatile struct_TaskFlg sSchedulerFlags;

//  Tick time in " mili-seconds " of the main software timer
#define SWTMR_TICK_MS       20
//  Convert milliseconds to timer ticks.
#define SWTMR_X0MS_TICKS    APP_TIMER_TICKS(SWTMR_TICK_MS)
//  Create a timer identifier and statically allocate memory for the timer
APP_TIMER_DEF(SWTMR_OS_TMR_ID);               
// Software t_50ms_swTmrHandleron -> set TIME-FLAG
static void t_x0ms_swTmrHandler(void * p_context)
{
  swTmr_tick_x0ms++;
  if( !(swTmr_tick_x0ms % TICK_TASK_BOILERTEMP))
  {
    sSchedulerFlags.tf_GetBoilerTemp = true;
  }else{}

  if( !(swTmr_tick_x0ms % TICK_TASK_READBTN) )
  {
    sSchedulerFlags.tf_ReadButton = true;
  }else{}

  if( !(swTmr_tick_x0ms % TICK_SVCS_ESPRESSO))
  {
    sSchedulerFlags.tf_svc_EspressoApp = true;
  }else{}

  if( !(swTmr_tick_x0ms % TICK_SVCS_STEPFCN))
  {
    sSchedulerFlags.tf_svc_StepFunction = true;
  }else{}



}

volatile bool PrintTask_flag    =false;
volatile bool ReadSensors_flag  =false;
volatile bool OneSecond_flag    =false;
volatile bool LightSeq_flag     =false;
volatile bool PumpCtrl_flag     =false;

/******************************************************************************************************************************/
/******************************************************************************************************************************/

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
   err_code = app_timer_create(&SWTMR_OS_TMR_ID, APP_TIMER_MODE_REPEATED, t_x0ms_swTmrHandler);
   APP_ERROR_CHECK(err_code);

}

/**@brief Function for starting timers.
 */
static void application_timers_start(void)
{
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer.
       ret_code_t err_code;
       err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
       APP_ERROR_CHECK(err_code);*/
    ret_code_t err_code;
    err_code = app_timer_start(SWTMR_OS_TMR_ID, SWTMR_X0MS_TICKS, NULL);
    APP_ERROR_CHECK(err_code);
}

/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
int main(void)
{
  /*Scheduler init. */
  sSchedulerFlags.tf_ReadButton = false;
  sSchedulerFlags.tf_svc_StepFunction = false;
  sSchedulerFlags.tf_GetBoilerTemp = false;
  sSchedulerFlags.tf_svc_EspressoApp = false;

  bool erase_bonds;
  ret_code_t err_code;
  uint32_t initResultFlag=0;
  static uint32_t userDataLoadedFlag=0;

  #if(NRF_LOG_ENABLED == 1)
    log_init();
  #endif

  //  GPIO DRIVER init
  //----------------------------------  -----------------------------------------
  if (!nrf_drv_gpiote_is_init())
  {
      err_code = nrf_drv_gpiote_init();
      if (err_code != NRF_SUCCESS)
      {
          return NRF_ERROR_INTERNAL;
      }
  }
  APP_ERROR_CHECK(err_code);
  /*  INITIALIZATION: TIMER DRIVER FOR THE SOFTWARE TIMER */
  timers_init();
  /*  INITIALIZATION: SPI DVR & EXTERNAL STORAGE DEVICE */
  spim_init();
  #if(NRF_LOG_ENABLED == 1)
    NRF_LOG_DEBUG("DRV INIT SPI interface ::READY::");
    NRF_LOG_FLUSH();
  #endif
  #if SET_TEST_USERDATA_EN == 1 
    blEspressoProfile.sp_BrewTemp       = 95.0f;
    blEspressoProfile.sp_StemTemp       = 135.0f;
    blEspressoProfile.prof_preInfusePwr = 60.0f;
    blEspressoProfile.prof_preInfuseTmr = 8.0f;
    blEspressoProfile.prof_InfusePwr    = 100.0f;
    blEspressoProfile.prof_InfuseTmr    = 12.0f;
    blEspressoProfile.Prof_DeclinePwr   = 90.0f;
    blEspressoProfile.Prof_DeclineTmr   = 20.0f;

    blEspressoProfile.Pid_P_term        = 9.52156f;
    blEspressoProfile.Pid_I_term        = 0.3f;
    blEspressoProfile.Pid_Imax_term     = 100.0f;
    blEspressoProfile.Pid_Iwindup_term  = false;
    blEspressoProfile.Pid_D_term        = 0.0f;
    blEspressoProfile.Pid_Dlpf_term     = 0.0f;
    blEspressoProfile.Pid_Gain_term     = 0.0f;
    NRF_LOG_DEBUG("SET DATA ::TEST DATA::");
    NRF_LOG_FLUSH();
  #else

  #if EXCLUDE_NVM_SECTION == false
    /*  INITIALIZATION: SPI - EXTERNAL STORAGE DEVICE DRIVER */
    initResultFlag = stgCtrl_Init();
    #if(NRF_LOG_ENABLED == 1)
      if( initResultFlag == NVM_INIT_OK)
      { 
        NRF_LOG_DEBUG("CNTRL INIT SPI External MEM ::READY::");
      }else{
        NRF_LOG_DEBUG("CNTRL INIT SPI External MEM  ::FAILED::");
      }
      NRF_LOG_FLUSH();
    #endif

    /*  CHECK FOR KEY CONTAINED INSIDE EXTERNAL STORAGE DEVICE */
    initResultFlag = stgCtrl_ChkForUserData();
    #if(NRF_LOG_ENABLED == 1)
      if( initResultFlag == STORAGE_USERDATA_LOADED)
      { 
        NRF_LOG_DEBUG("EXT MEM ::HAS DATA::");
      }else{
        NRF_LOG_DEBUG("EXT MEM ::IS EMPTY::");
      }
      NRF_LOG_FLUSH();
    #endif

    /*  IF MEMORY CONTAINS KEY  */
    if( initResultFlag == STORAGE_USERDATA_LOADED)
    {
      /* STORAGE_USERDATA_LOADED = memory read success and stored in: blEspressoProfile */
      userDataLoadedFlag = stgCtrl_ReadUserData((bleSpressoUserdata_struct*)&blEspressoProfile);
    }else{}
    #if(NRF_LOG_ENABLED == 1)
      if( userDataLoadedFlag == STORAGE_USERDATA_LOADED)
      { 
        NRF_LOG_DEBUG("USER DATA ::Loaded::");
      }else{
        NRF_LOG_DEBUG("USER DATA ::Empty::");
      }
      NRF_LOG_FLUSH();
    #endif
   #endif

    #if(NRF_LOG_ENABLED == 1)
      if( userDataLoadedFlag == STORAGE_USERDATA_LOADED)
      { 
        stgCtrl_PrintUserData((bleSpressoUserdata_struct*)&blEspressoProfile);
      }else{
        NRF_LOG_DEBUG("USER DATA ::Couldn't be Printed::");
      }
      NRF_LOG_FLUSH();
    #endif
  #endif

  /*  INITIALIZATION: SPI - TEMPERATURE SENSOR DRIVER */
  initResultFlag = spim_initRTDconverter();
  #if(NRF_LOG_ENABLED == 1)
  if( initResultFlag == TMP_INIT_OK)
  { 
    NRF_LOG_DEBUG("DRV INIT SPI-Temperature Sensor ::READY::");
  }else{
    NRF_LOG_DEBUG("DRV INIT SPI-Temperature Sensor ::FAILED::");
  }
  NRF_LOG_FLUSH();
  #endif
  /*  INITIALIZATION: HIGH-SIDE SWITCH DRIVER TO PROVIDE 12VOUT */
  initResultFlag = fcn_initDC12Voutput_drv();
  #if(NRF_LOG_ENABLED == 1)
  if( initResultFlag == DRV_12VO_INIT_AS_LAMP)
  { 
    NRF_LOG_DEBUG("DRV INIT 12VDC output ::SUCCESFUL::");
  }else{
    NRF_LOG_DEBUG("NVM INIT 12VDC output ::FAILED::");
  }
  NRF_LOG_FLUSH();
  #endif
  
  /*  INITIALIZATION: AC INPUTS DRIVER   */
  initResultFlag = fcn_initACinput_drv();
  #if(NRF_LOG_ENABLED == 1)
  if( initResultFlag == DRV_AC_INPUT_INIT_OK)
  { 
    NRF_LOG_DEBUG("DRV INIT AC Inputs ::READY::");
  }else{
    NRF_LOG_DEBUG("DRV INIT AC Inputs ::FAILED::");
  }
  NRF_LOG_FLUSH();
  #endif
  /*  INITIALIZATION: SOLID STATE RELAY CONTROLLER/DRIVER */
  initResultFlag = fcn_initSSRController_BLEspresso();
  #if(NRF_LOG_ENABLED == 1)
    if( initResultFlag == SSR_DRV_INIT_OK)
    { 
      NRF_LOG_DEBUG("DRV INIT Solid State Relays ::READY::");
    }else{
      NRF_LOG_DEBUG("DRV INIT Solid State Relays ::FAILED::");
    }
    NRF_LOG_FLUSH();
  #endif

  /*  INITIALIZATION: PUMP CONTROLLER/DRIVER */
  initResultFlag = fcn_initPumpController();
  #if(NRF_LOG_ENABLED == 1)
    if( initResultFlag == PUMPCTRL_INIT_OK)
    { 
      NRF_LOG_DEBUG("CNTRL INIT Pump Controller ::READY::");
    }else{
      NRF_LOG_DEBUG("CNTRL INIT Pump Controller ::FAILED::");
    }
    NRF_LOG_FLUSH();
  #endif
  /*  AFTER PUMP DRV INIT - LOAD OF USER PARAM FROM blEspressoProfile INTO PUMP DRV */
  #if(LOAD_USERDATA_FROM_NVM_EN == 1)
    if( userDataLoadedFlag == STORAGE_USERDATA_LOADED)
    {
      initResultFlag = fcn_LoadNewPumpParameters((bleSpressoUserdata_struct*)&blEspressoProfile);
    }else{}
    NRF_LOG_DEBUG("Pump Controller ::DATA READY::");
    NRF_LOG_FLUSH();
  #endif
  #if(LOAD_USERDATA_FROM_NVM_EN ==  0 && SET_TEST_USERDATA_EN==1)
    if( userDataLoadedFlag == STORAGE_USERDATA_LOADED)
    {
      initResultFlag = fcn_LoadNewPumpParameters((bleSpressoUserdata_struct*)&blEspressoProfile);
    }else{}
    NRF_LOG_DEBUG("Pump Controller ::TEST READY::");
    NRF_LOG_FLUSH();
  #endif

  /*  INITIALIZATION: BOILER TEMPERATURE CONTROLLER/DRIVER  */
  initResultFlag = fcn_initCntrl_Temp();
  #if(NRF_LOG_ENABLED == 1)
    if( initResultFlag == PUMPCTRL_INIT_OK)
    { 
      NRF_LOG_DEBUG("CNTRL INIT Boiler Temperature Controller ::READY::");
    }else{
      NRF_LOG_DEBUG("CNTRL INIT Boiler Temperature Controller ::FAILED::");
    }
    NRF_LOG_FLUSH();
  #endif
  #if(LOAD_USERDATA_FROM_NVM_EN == 1)
    /*  LOADING [USER] BOILER TEMP PID CONTROLLER's PARAMETERS */
    fcn_loadPID_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile);
    NRF_LOG_DEBUG("Boiler Controller ::DATA READY::");
    NRF_LOG_FLUSH();
  #endif
  #if(LOAD_USERDATA_FROM_NVM_EN ==  0 && SET_TEST_USERDATA_EN==1)
    /*  LOADING [TEST] BOILER TEMP PID CONTROLLER's PARAMETERS */
    fcn_loadPID_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile);
    NRF_LOG_DEBUG("Boiler Temperature Controller ::TEST DATA READY::");
    NRF_LOG_FLUSH();
  #endif
  initResultFlag = fcn_loaddSetPoint_ParamToCtrl_Temp(
                                                      (bleSpressoUserdata_struct*)&blEspressoProfile,
                                                      SETPOINT_BREW);
  #if(NRF_LOG_ENABLED == 1)
    if( initResultFlag == TEMPCTRL_SP_LOAD_OK)
    { 
      NRF_LOG_DEBUG("Boiler Temperature Controller ::LOAD SET POINT::");
    }else{
      NRF_LOG_DEBUG("Boiler Temperature Controller ::FAILED SET POINT::");
    }
    NRF_LOG_FLUSH();
  #endif
  

  /* swection of code to control LED on the board*/
  /***********************************************/
  ret_code_t err_code_gpio;
  nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
  err_code_gpio = nrf_drv_gpiote_out_init(29, &out_config);
  APP_ERROR_CHECK(err_code_gpio);

  // Get status of main switches to determine operation mode
  fcn_SenseACinputs_Sixty_ms();
  if( fcn_GetInputStatus_Brew() == AC_SWITCH_ASSERTED && fcn_GetInputStatus_Steam() == AC_SWITCH_ASSERTED )
  {
    #if(NRF_LOG_ENABLED == 1)
      NRF_LOG_RAW_INFO("\r\n \r\nMACHINE ::Step Function Mode::");
      NRF_LOG_FLUSH(); 
    #endif
    appModeToRun=machine_Tune;
  }else{
    #if(NRF_LOG_ENABLED == 1)
      NRF_LOG_RAW_INFO("\r\n \r\nMACHINE ::Application Mode::\r\n");
      NRF_LOG_FLUSH(); 
    #endif
    appModeToRun=machine_App;
  }
 
  // Start Bluetooth Driver
  // --------------------------------------------------------------------------
  application_timers_start();
  BLE_bluetooth_init();
  // Start Bluetooth execution 
  #if EXCLUDE_BLE_ADV_SECTION != true
    advertising_start(erase_bonds);
  #endif

  // Start Boiler Temperature controller
  // --------------------------------------------------------------------------
  fcn_startTempCtrlSamplingTmr();
  for (;;)
  {
      
    if( sSchedulerFlags.tf_ReadButton == true)
    {
      sSchedulerFlags.tf_ReadButton = false;
      fcn_SenseACinputs_Sixty_ms();
    }else{}

    if(sSchedulerFlags.tf_GetBoilerTemp == true)
    {
      sSchedulerFlags.tf_GetBoilerTemp=false;
      spim_ReadRTDconverter();
      blEspressoProfile.temp_Boiler=(float)f_getBoilerTemperature();
      //nrf_drv_gpiote_out_toggle(29);
    }else{}

    if(appModeToRun == machine_App)
    {
      if( sSchedulerFlags.tf_svc_EspressoApp == true)
      {
        sSchedulerFlags.tf_svc_EspressoApp = false;
        fcn_service_ClassicMode(fcn_GetInputStatus_Brew(),fcn_GetInputStatus_Steam());
        //fcn_service_ProfileMode(fcn_GetInputStatus_Brew(),fcn_GetInputStatus_Steam());
        nrf_drv_gpiote_out_toggle(29);
      }else{}
    }else{}

    if(appModeToRun == machine_Tune)
    {
      if( sSchedulerFlags.tf_svc_StepFunction == true )
      {
        sSchedulerFlags.tf_svc_StepFunction = false;
        fcn_service_StepFunction(fcn_GetInputStatus_Brew(),fcn_GetInputStatus_Steam());
        //nrf_drv_gpiote_out_toggle(29);
      }else{} 
    }else{}   
  }
}


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

   
void fcn_DO_NOTHING(void)
{
uint32_t initResultFlag=0;
      if(ReadSensors_flag == true)
      {
        ReadSensors_flag=false;
        spim_ReadRTDconverter();
        blEspressoProfile.temp_Boiler=(float)f_getBoilerTemperature();
        /* This function does not goes here. it has to be called after BLE service update */
        //fcn_LoadNewBoilerTemperature((bleSpressoUserdata_struct*)&blEspressoProfile);
      }else{}
      if(PrintTask_flag == true)
      {
        PrintTask_flag=false;
        ble_update_boilerWaterTemp(f_getBoilerTemperature());
        #if(NRF_LOG_ENABLED == 1)
          NRF_LOG_INFO("\033[0;36m Temp: 10," NRF_LOG_FLOAT_MARKER "\r\n \033[0;40m", NRF_LOG_FLOAT(f_getBoilerTemperature()));
        #endif
      }else{}
      
      if(OneSecond_flag == true)
      {
        OneSecond_flag = false;

      }else{}


          if(fcn_GetInputStatus_Brew() == AC_SWITCH_ASSERTED )
          {
            #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("BREW-> \033[0;42m ON \033[0;40m \r\n ");
            #endif
          }else{
            #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("BREW-> \033[0;43m OFF \033[0;40m \r\n ");
            #endif
          }


          if(fcn_GetInputStatus_Steam() == AC_SWITCH_ASSERTED )
          {
            #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("STEAM-> \033[0;42m ON \033[0;40m \r\n");
            #endif
          }else{
            #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("STEAM-> \033[0;43m OFF \033[0;40m \r\n");
            #endif
          }

      if(PumpCtrl_flag == true)
      {
          PumpCtrl_flag = false;
          initResultFlag = fcn_PumpStateDriver();
          switch(initResultFlag)
          {
            case PUMPCTRL_STEP_1ST: 
              #if(NRF_LOG_ENABLED == 1)
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42m Pre-Infussion\033[0;40m \r\n");
              #endif
            break;
            case PUMPCTRL_STEP_2ND: 
              #if(NRF_LOG_ENABLED == 1)
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42m Brewing at Max Pressure\033[0;40m \r\n");
              #endif
            break;
            case PUMPCTRL_STEP_3RD: 
              #if(NRF_LOG_ENABLED == 1)
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42m Brewing at Mid Pressure\033[0;40m \r\n");
              #endif
            break;
            case PUMPCTRL_STEP_STOP:
              #if(NRF_LOG_ENABLED == 1)
                NRF_LOG_INFO("\033[0;32m Pump: \033[0;40m\033[0;42m Brewing Stop\033[0;40m \r\n");
              #endif
            break;
            default:
            break;
          }   
      }else{}
      
      if(LightSeq_flag == true)
      {
          LightSeq_flag = false;
          //sm_DC12Voutput_drv(&s12Vout);
      }else{}

      if(flg_BrewCfg == 1)
      {
        flg_BrewCfg = 0;
        /* blEspressoProfile.temp_Target will be updated from BLE CUService when flg_BrewCfg = 1 */
        #if(ALLOW_USERDATA_WR_NVM__EN == 1)
          initResultFlag = stgCtrl_StoreShotProfileData((bleSpressoUserdata_struct*) &blEspressoProfile);
        #endif
        #if(NRF_LOG_ENABLED == 1)
          if( initResultFlag == STORAGE_PROFILEDATA_STORED)
          { 
            NRF_LOG_INFO("\033[0;44m <NVM> \033[0;40m" "\033[0;34m Brew Profile saved \033[0;40m \r\n");
          }else{
            NRF_LOG_INFO("\033[0;44m <NVM> \033[0;40m" "\033[0;34m Brew Profile could not saved \033[0;40m \r\n");
          }
          NRF_LOG_FLUSH();  
        #endif
      }else{}

      if(flg_PidCfg == 1)
      {
        flg_PidCfg = 0;
        #if(ALLOW_USERDATA_WR_NVM__EN==1)
          initResultFlag = stgCtrl_StoreControllerData((bleSpressoUserdata_struct*) &blEspressoProfile);
        #endif
        #if(NRF_LOG_ENABLED == 1)
          if( initResultFlag == STORAGE_PROFILEDATA_STORED)
          { 
            NRF_LOG_INFO("\033[0;44m <NVM> \033[0;40m" "\033[0;34m PID parameters saved \033[0;40m \r\n");
          }else{
            NRF_LOG_INFO("\033[0;44m <NVM> \033[0;40m" "\033[0;34m PID parameters could not saved \033[0;40m \r\n");
          }
          NRF_LOG_FLUSH();  
        #endif
      }else{}

      if(flg_ReadCfg == 1 )
      {
        flg_ReadCfg = 0;
        //fcn_Read_ParameterNVM((bleSpressoUserdata_struct *)&blEspressoProfile);
        #if(NRF_LOG_ENABLED == 1)
          NRF_LOG_INFO("\033[0;44m <NVM> \033[0;40m" "\033[0;34m Read data from FLASH block data \033[0;40m \r\n");
        #endif
      }else{}

      #if(NRF_LOG_ENABLED == 1)
      NRF_LOG_FLUSH();
      #endif
}