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
#include "espressoMachineServices.h"
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

volatile uint16_t g_ssrPower=0;
volatile uint16_t g_ssrPump=0;


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
#define TICK_TASK_BLEUPDATE     (1000 / SWTMR_TICK_MS)

/*  Code Section:
    Main & fastest Tick time in the system: ** 50 ms **
    Use to control rest of sync function
*/
volatile uint32_t g_swTmr_tick_x0ms = 0;
typedef struct
{
  bool tf_ReadButton;
  bool tf_GetBoilerTemp;
  bool tf_ble_update;
  bool tf_svc_StepFunction;
  bool tf_svc_EspressoApp;
}struct_TaskFlg;

volatile struct_TaskFlg g_Scheduler_flags_s;

//  Tick time in " mili-seconds " of the main software timer
#define SWTMR_TICK_MS       20
//  Convert milliseconds to timer ticks.
#define SWTMR_X0MS_TICKS    APP_TIMER_TICKS(SWTMR_TICK_MS)
//  Create a timer identifier and statically allocate memory for the timer
APP_TIMER_DEF(SWTMR_OS_TMR_ID);               
// Software t_50ms_swTmrHandleron -> set TIME-FLAG
static void t_x0ms_swTmrHandler(void * p_context)
{
  g_swTmr_tick_x0ms++;
  if( !(g_swTmr_tick_x0ms % TICK_TASK_BOILERTEMP))
  {
    g_Scheduler_flags_s.tf_GetBoilerTemp = true;
  }else{}

  if( !(g_swTmr_tick_x0ms % TICK_TASK_READBTN) )
  {
    g_Scheduler_flags_s.tf_ReadButton = true;
  }else{}

  if( !(g_swTmr_tick_x0ms % TICK_TASK_BLEUPDATE) )
  {
    g_Scheduler_flags_s.tf_ble_update = true;
  }else{}

  if( !(g_swTmr_tick_x0ms % TICK_SVCS_ESPRESSO))
  {
    g_Scheduler_flags_s.tf_svc_EspressoApp = true;
  }else{}

  if( !(g_swTmr_tick_x0ms % TICK_SVCS_STEPFCN))
  {
    g_Scheduler_flags_s.tf_svc_StepFunction = true;
  }else{}



}

volatile bool g_PrintTask_flag    =false;
volatile bool g_ReadSensors_flag  =false;
volatile bool g_OneSecond_flag    =false;
volatile bool g_LightSeq_flag     =false;
volatile bool g_PumpCtrl_flag     =false;

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
 * Function: 	temp_ctrl_sampling_timer_event_handler
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

/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
int main(void)
{
  /*Scheduler init. */

  g_Scheduler_flags_s.tf_ReadButton = false;
  g_Scheduler_flags_s.tf_svc_StepFunction = false;
  g_Scheduler_flags_s.tf_GetBoilerTemp = false;
  g_Scheduler_flags_s.tf_ble_update = false;
  g_Scheduler_flags_s.tf_svc_EspressoApp = false;

  bool erase_bonds;
  ret_code_t err_code;
  uint32_t init_result_flag=0;
  static uint32_t user_data_loaded_flag=0;

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
    g_Espresso_user_config_s.brewTempDegC     = 95.0f;
    g_Espresso_user_config_s.steamTempDegC    = 135.0f;

    g_Espresso_user_config_s.profPreInfusePwr = 80.0f;
    g_Espresso_user_config_s.profPreInfuseTmr = 8.0f;

    g_Espresso_user_config_s.profInfusePwr    = 100.0f;
    g_Espresso_user_config_s.profInfuseTmr    = 10.0f;

    g_Espresso_user_config_s.profTaperingPwr  = 90.0f;
    g_Espresso_user_config_s.profTaperingTmr  = 15.0f;

    g_Espresso_user_config_s.pidPTerm        = 9.52156f;
    g_Espresso_user_config_s.pidITerm        = 0.3f;
    g_Espresso_user_config_s.pidImaxTerm     = 100.0f;
    g_Espresso_user_config_s.pidIwindupTerm  = false;
    g_Espresso_user_config_s.pidDTerm        = 0.0f;
    g_Espresso_user_config_s.pidDlpfTerm     = 0.0f;
    g_Espresso_user_config_s.pidGainTerm     = 0.0f;
    NRF_LOG_DEBUG("SET DATA ::TEST DATA::");
    NRF_LOG_FLUSH();
  #else

  #if EXCLUDE_NVM_SECTION == false
    /*  INITIALIZATION: SPI - EXTERNAL STORAGE DEVICE DRIVER */
    init_result_flag = storage_init();
    #if(NRF_LOG_ENABLED == 1)
      if( init_result_flag == NVM_INIT_OK)
      { 
        NRF_LOG_DEBUG("CNTRL INIT SPI External MEM ::READY::");
      }else{
        NRF_LOG_DEBUG("CNTRL INIT SPI External MEM  ::FAILED::");
      }
      NRF_LOG_FLUSH();
    #endif

    /*  CHECK FOR KEY CONTAINED INSIDE EXTERNAL STORAGE DEVICE */
    init_result_flag = storage_has_user_config();
    #if(NRF_LOG_ENABLED == 1)
      if( init_result_flag == STORAGE_USERDATA_LOADED)
      { 
        NRF_LOG_DEBUG("EXT MEM ::HAS DATA::");
      }else{
        NRF_LOG_DEBUG("EXT MEM ::IS EMPTY::");
      }
      NRF_LOG_FLUSH();
    #endif

    /*  IF MEMORY CONTAINS KEY  */
    if( init_result_flag == STORAGE_USERDATA_LOADED)
    {
      /* STORAGE_USERDATA_LOADED = memory read success and stored in: g_Espresso_user_config_s */
      user_data_loaded_flag = storage_load_user_config((espresso_user_config_t*)&g_Espresso_user_config_s);
    }else{}
    #if(NRF_LOG_ENABLED == 1)
      if( user_data_loaded_flag == STORAGE_USERDATA_LOADED)
      { 
        NRF_LOG_DEBUG("USER DATA ::Loaded::");
      }else{
        NRF_LOG_DEBUG("USER DATA ::Empty::");
      }
      NRF_LOG_FLUSH();
    #endif
   #endif

    #if(NRF_LOG_ENABLED == 1)
      if( user_data_loaded_flag == STORAGE_USERDATA_LOADED)
      { 
        storage_print_user_config((espresso_user_config_t*)&g_Espresso_user_config_s);
      }else{
        NRF_LOG_DEBUG("USER DATA ::Couldn't be Printed::");
      }
      NRF_LOG_FLUSH();
    #endif
  #endif

  /*  INITIALIZATION: SPI - TEMPERATURE SENSOR DRIVER */
  init_result_flag = spim_initRTDconverter();
  #if(NRF_LOG_ENABLED == 1)
  if( init_result_flag == TMP_INIT_OK)
  { 
    NRF_LOG_DEBUG("DRV INIT SPI-Temperature Sensor ::READY::");
  }else{
    NRF_LOG_DEBUG("DRV INIT SPI-Temperature Sensor ::FAILED::");
  }
  NRF_LOG_FLUSH();
  #endif
  /*  INITIALIZATION: HIGH-SIDE SWITCH DRIVER TO PROVIDE 12VOUT */
  init_result_flag = fcn_initDC12Voutput_drv();
  #if(NRF_LOG_ENABLED == 1)
  if( init_result_flag == DRV_12VO_INIT_AS_LAMP)
  { 
    NRF_LOG_DEBUG("DRV INIT 12VDC output ::SUCCESFUL::");
  }else{
    NRF_LOG_DEBUG("NVM INIT 12VDC output ::FAILED::");
  }
  NRF_LOG_FLUSH();
  #endif
  
  /*  INITIALIZATION: AC INPUTS DRIVER   */
  init_result_flag = fcn_initACinput_drv();
  #if(NRF_LOG_ENABLED == 1)
  if( init_result_flag == DRV_AC_INPUT_INIT_OK)
  { 
    NRF_LOG_DEBUG("DRV INIT AC Inputs ::READY::");
  }else{
    NRF_LOG_DEBUG("DRV INIT AC Inputs ::FAILED::");
  }
  NRF_LOG_FLUSH();
  #endif
  /*  INITIALIZATION: SOLID STATE RELAY CONTROLLER/DRIVER */
  init_result_flag = fcn_initSSRController_BLEspresso();
  #if(NRF_LOG_ENABLED == 1)
    if( init_result_flag == SSR_DRV_INIT_OK)
    { 
      NRF_LOG_DEBUG("DRV INIT Solid State Relays ::READY::");
    }else{
      NRF_LOG_DEBUG("DRV INIT Solid State Relays ::FAILED::");
    }
    NRF_LOG_FLUSH();
  #endif

  /*  INITIALIZATION: PUMP CONTROLLER/DRIVER */
  init_result_flag = fcn_initPumpController();
  #if(NRF_LOG_ENABLED == 1)
    if( init_result_flag == PUMPCTRL_INIT_OK)
    { 
      NRF_LOG_DEBUG("CNTRL INIT Pump Controller ::READY::");
    }else{
      NRF_LOG_DEBUG("CNTRL INIT Pump Controller ::FAILED::");
    }
    NRF_LOG_FLUSH();
  #endif
  /*  AFTER PUMP DRV INIT - LOAD OF USER PARAM FROM g_Espresso_user_config_s INTO PUMP DRV */
  #if(LOAD_USERDATA_FROM_NVM_EN == 1)
    if( user_data_loaded_flag == STORAGE_USERDATA_LOADED)
    {
      init_result_flag = load_new_pump_parameters((espresso_user_config_t*)&g_Espresso_user_config_s);
    }else{}
    NRF_LOG_DEBUG("Pump Controller ::DATA READY::");
    NRF_LOG_FLUSH();
  #endif
  #if(LOAD_USERDATA_FROM_NVM_EN ==  0 && SET_TEST_USERDATA_EN==1)
    if( user_data_loaded_flag == STORAGE_USERDATA_LOADED)
    {
      init_result_flag = load_new_pump_parameters((espresso_user_config_t*)&g_Espresso_user_config_s);
    }else{}
    NRF_LOG_DEBUG("Pump Controller ::TEST READY::");
    NRF_LOG_FLUSH();
  #endif

  /*  INITIALIZATION: BOILER TEMPERATURE CONTROLLER/DRIVER  */
  init_result_flag = temp_ctrl_init();
  #if(NRF_LOG_ENABLED == 1)
    if( init_result_flag == PUMPCTRL_INIT_OK)
    { 
      NRF_LOG_DEBUG("CNTRL INIT Boiler Temperature Controller ::READY::");
    }else{
      NRF_LOG_DEBUG("CNTRL INIT Boiler Temperature Controller ::FAILED::");
    }
    NRF_LOG_FLUSH();
  #endif
  #if(LOAD_USERDATA_FROM_NVM_EN == 1)
    /*  LOADING [USER] BOILER TEMP PID CONTROLLER's PARAMETERS */
    temp_ctrl_set_pid_config((espresso_user_config_t*)&g_Espresso_user_config_s);
    NRF_LOG_DEBUG("Boiler Controller ::DATA READY::");
    NRF_LOG_FLUSH();
  #endif
  #if(LOAD_USERDATA_FROM_NVM_EN ==  0 && SET_TEST_USERDATA_EN==1)
    /*  LOADING [TEST] BOILER TEMP PID CONTROLLER's PARAMETERS */
    temp_ctrl_set_pid_config((espresso_user_config_t*)&g_Espresso_user_config_s);
    NRF_LOG_DEBUG("Boiler Temperature Controller ::TEST DATA READY::");
    NRF_LOG_FLUSH();
  #endif
  init_result_flag = temp_ctrl_set_boiler_setpoint(
                                                      (espresso_user_config_t*)&g_Espresso_user_config_s,
                                                      SET_POINT_BREW);
  #if(NRF_LOG_ENABLED == 1)
    if( init_result_flag == SET_POINT_LOAD_OK)
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
    g_operation_mode=ESPRESSO_MODE__TUNE;
  }else{
    #if(NRF_LOG_ENABLED == 1)
      NRF_LOG_RAW_INFO("\r\n \r\nMACHINE ::Application Mode::\r\n");
      NRF_LOG_FLUSH(); 
    #endif
    g_operation_mode=ESPRESSO_MODE__MANUAL;
  }
 
  // Start Bluetooth Driver
  // --------------------------------------------------------------------------
  application_timers_start();
  bluetooth_low_energy_init((espresso_user_config_t *)&g_Espresso_user_config_s);
  // Start Bluetooth execution 
  #if EXCLUDE_BLE_ADV_SECTION != true
    advertising_start(erase_bonds);
  #endif

  // Start Boiler Temperature controller
  // --------------------------------------------------------------------------
  temp_ctrl_start_sampling_timer();
  for (;;)
  {
      
    if( g_Scheduler_flags_s.tf_ReadButton == true)
    {
      g_Scheduler_flags_s.tf_ReadButton = false;
      fcn_SenseACinputs_Sixty_ms();
    }else{}

    if(g_Scheduler_flags_s.tf_GetBoilerTemp == true)
    {
      /* Get water temperature from the boiler  */
      g_Scheduler_flags_s.tf_GetBoilerTemp=false;
      spim_ReadRTDconverter();
      g_Espresso_user_config_s.boilerTempDegC=(float)f_getBoilerTemperature();
      //nrf_drv_gpiote_out_toggle(29);
    }else{}

    if(g_operation_mode == ESPRESSO_MODE__MANUAL)
    {
      if( g_Scheduler_flags_s.tf_svc_EspressoApp == true)
      {
        g_Scheduler_flags_s.tf_svc_EspressoApp = false;
        //fcn_service_ClassicMode(fcn_GetInputStatus_Brew(),fcn_GetInputStatus_Steam());
        fcn_service_ProfileMode(fcn_GetInputStatus_Brew(),fcn_GetInputStatus_Steam());
        nrf_drv_gpiote_out_toggle(29);
      }else{}
    }else{}

    if(g_operation_mode == ESPRESSO_MODE__TUNE)
    {
      if( g_Scheduler_flags_s.tf_svc_StepFunction == true )
      {
        g_Scheduler_flags_s.tf_svc_StepFunction = false;
        fcn_service_StepFunction(fcn_GetInputStatus_Brew(),fcn_GetInputStatus_Steam());
        //nrf_drv_gpiote_out_toggle(29);
      }else{} 
    }else{} 

    if( g_Scheduler_flags_s.tf_ble_update == true)
    {
      /* NOTIFY to BLE the new read of the water temperature from the boiler  */
      g_Scheduler_flags_s.tf_ble_update = false;
      ble_notify_boiler_water_temp(g_Espresso_user_config_s.boilerTempDegC);
    }else{}  

    if (NRF_LOG_PROCESS() == false){idle_state_handle();}
  }
}

/*
NRF_LOG_INFO("\033[0;44m <NVM> \033[0;40m" "\033[0;34m Brew Profile saved \033[0;40m \r\n");
NRF_LOG_INFO("\033[0;44m <NVM> \033[0;40m" "\033[0;34m Brew Profile could not saved \033[0;40m \r\n");

NRF_LOG_INFO("STEAM-> \033[0;42m ON \033[0;40m \r\n");
NRF_LOG_INFO("STEAM-> \033[0;43m OFF \033[0;40m \r\n");


*/