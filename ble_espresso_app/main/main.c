/* This solution is applciable for the actual Controller  */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "boards.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "bluetooth_drv.h"
#include "log_drv.h"
#include "board_comp_drv.h"
#include "nrf_drv_timer.h"
#include "app_timer.h"
#include "i2c_sensors.h"
#include "spi_sensors.h"

#include "solidStateRelay_Controller.h"
#include "ac_inputs_drv.h"
#include "nrf_delay.h"
#include "app_error.h"

volatile bool PrintTask_flag;
volatile bool ReadSensors_flag;
volatile bool OneSecond_flag;
volatile uint16_t ssrPower=0;

//  Create a timer identifier and statically allocate memory for the timer
APP_TIMER_DEF(READTEMPSENSORS_TMR_ID);
APP_TIMER_DEF(PRINTTEMPVALUES_TMR_ID);
APP_TIMER_DEF(ONE_AC_CYCLE____TMR_ID);
APP_TIMER_DEF(ONE_SECOND______TMR_ID);

//  Convert milliseconds to timer ticks.
#define READTEMPSENSORS_TICKS   APP_TIMER_TICKS(200)
#define PRINTTEMPVALUES_TICKS   APP_TIMER_TICKS(1000)
#define ONE_AC_CYCLE____TICKS   APP_TIMER_TICKS(100)
#define ONE_SECOND______TICKS   APP_TIMER_TICKS(600)

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

void acinSteam_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    sControllerInputs.Steam.Counter++;
}
void acinBrew_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    sControllerInputs.Brew.Counter++;
}

void isr_ZeroCross_EventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if( sSSRdrvConfig.smTrigStatus == smS_Release)
    {
      //nrf_drv_gpiote_out_set(31);
      fcn_SSR_ctrlUpdate((struct_ssrController *)&sSSRdrvConfig);
      //nrf_drv_timer_enable((nrfx_timer_t const *)&sSSRdrvConfig.hwTmr);
      sSSRdrvConfig.smTrigStatus = smS_Engage;
      //nrf_drv_gpiote_out_clear(31);
      nrf_drv_gpiote_out_clear(sSSRdrvConfig.out_SSRelay);
    }else{
      sSSRdrvConfig.smTrigStatus = smS_Release;
    }
}

void isr_SSRcontroller_EventHandler(nrf_timer_event_t event_type, void* p_context)
{
    //nrf_drv_timer_pause((nrfx_timer_t const *)&sSSRdrvConfig.hwTmr);
    //nrf_drv_timer_clear((nrfx_timer_t const *)&sSSRdrvConfig.hwTmr);
      switch (event_type)
      {
          case NRF_TIMER_EVENT_COMPARE0:
              nrf_drv_gpiote_out_set(sSSRdrvConfig.out_SSRelay);
              break;
          case NRF_TIMER_EVENT_COMPARE1:
              nrf_drv_gpiote_out_clear(sSSRdrvConfig.out_SSRelay);
              break;
          default:
              //Do nothing.
              break;
      }
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
                 one.
       ret_code_t err_code;
       err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
       APP_ERROR_CHECK(err_code); */
       err_code = app_timer_create(&READTEMPSENSORS_TMR_ID, APP_TIMER_MODE_REPEATED, ReadTempSns_swTmrHandler);
       APP_ERROR_CHECK(err_code);
       err_code = app_timer_create(&PRINTTEMPVALUES_TMR_ID, APP_TIMER_MODE_REPEATED, PrintTempVal_swTmrHandler);
       APP_ERROR_CHECK(err_code);
       err_code = app_timer_create(&ONE_AC_CYCLE____TMR_ID, APP_TIMER_MODE_REPEATED, OneACcycle_swTmrHandler);
       APP_ERROR_CHECK(err_code);
       err_code = app_timer_create(&ONE_SECOND______TMR_ID, APP_TIMER_MODE_REPEATED, OneSecond_swTmrHandler);
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



/**@brief Function for application main entry.
 */
int main(void)
{
    bool erase_bonds;
    // Initialize.
    log_init();
    ReadSensors_flag=false;
    PrintTask_flag=false;
    timers_init();
    //buttons_leds_init(&erase_bonds);
    //bsp_board_leds_init();
    bsp_board_init(BSP_INIT_LEDS);
    /*twi_init();
    NRF_LOG_INFO("I2C0 example started.");
    LM75B_set_mode();
*/
    spim_init();
    spim_initRTDconverter();
    NRF_LOG_INFO("SPI1 example started.");

    BLE_bluetooth_init();
    // Start execution.
    NRF_LOG_INFO("First APP draft.");
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

    sSSRdrvConfig.hwTmr               = (nrf_drv_timer_t)NRF_DRV_TIMER_INSTANCE(1);
    sSSRdrvConfig.hwTmr_isr_handler   = isr_SSRcontroller_EventHandler;
    sSSRdrvConfig.in_zCross           = inZEROCROSS_PIN;
    sSSRdrvConfig.out_SSRelay         = outSSRrelay_PIN;
    sSSRdrvConfig.zcross_isr_handler  = isr_ZeroCross_EventHandler;
    fcn_initSsrController((struct_ssrController *)&sSSRdrvConfig);

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
          NRF_LOG_INFO("\033[0;36m Temp: " NRF_LOG_FLOAT_MARKER "\r\n \033[0;40m", NRF_LOG_FLOAT(rtdTemperature));
        }else{}
        
        if(OneSecond_flag == true)
        {
          OneSecond_flag = false;
          //ssrPower = ssrPower +50;
          if(ssrPower>1009)
          {ssrPower=0;}
          fcn_SSR_pwrUpdate((struct_ssrController *)&sSSRdrvConfig, ssrPower);
          NRF_LOG_INFO("\033[0;33m Heat Power: \033[0;40m \033[0;43m" NRF_LOG_FLOAT_MARKER "\033[0;40m \r\n", NRF_LOG_FLOAT((float)ssrPower/10.0f));
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
        NRF_LOG_FLUSH();
    }
}
