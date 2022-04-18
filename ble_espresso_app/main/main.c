
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bluetooth_drv.h"
#include "log_drv.h"
#include "board_comp_drv.h"
#include "app_timer.h"
#include "i2c_sensors.h"
#include "spi_sensors.h"

#include "nrf_delay.h"

volatile bool PrintTask_flag;
volatile bool ReadSensors_flag;


//  Create a timer identifier and statically allocate memory for the timer
APP_TIMER_DEF(READTEMPSENSORS_TMR_ID);
APP_TIMER_DEF(PRINTTEMPVALUES_TMR_ID);
APP_TIMER_DEF(TOGGLELED0______TMR_ID);

//  Convert milliseconds to timer ticks.
#define READTEMPSENSORS_TICKS   APP_TIMER_TICKS(200)
#define PRINTTEMPVALUES_TICKS   APP_TIMER_TICKS(533)
#define TOGGLELED0______TICKS   APP_TIMER_TICKS(500)

static void ReadTempSns_swTmrHandler(void * p_context)
{
  ReadSensors_flag = true;
}

static void PrintTempVal_swTmrHandler(void * p_context)
{
  PrintTask_flag = true;
}

static void ToggleLed0_swTmrHandler(void * p_context)
{
    bsp_board_led_invert(3);
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
       err_code = app_timer_create(&TOGGLELED0______TMR_ID, APP_TIMER_MODE_REPEATED, ToggleLed0_swTmrHandler);
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
    err_code = app_timer_start(TOGGLELED0______TMR_ID, TOGGLELED0______TICKS, NULL);
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
    buttons_leds_init(&erase_bonds);

    twi_init();
    NRF_LOG_INFO("I2C0 example started.");
    LM75B_set_mode();

    spim_init();
    NRF_LOG_INFO("SPI1 example started.");

    BLE_bluetooth_init();
    // Start execution.
    NRF_LOG_INFO("First APP draft.");

    NRF_LOG_FLUSH();

    application_timers_start();
    advertising_start(erase_bonds);
    // Enter main loop.
    for (;;)
    {
        if(ReadSensors_flag == true)
        {
          ReadSensors_flag=false;
          read_sensor_data();
          spim_Read_TempSensor();
        }else{}
        if(PrintTask_flag == true)
        {
          PrintTask_flag=false;
          NRF_LOG_INFO("Tpcb: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(eTMP006_Temp));
          NRF_LOG_INFO("Tamb: " NRF_LOG_FLOAT_MARKER "\r\n", NRF_LOG_FLOAT(eThermocoupleTemp));
          NRF_LOG_FLUSH();
        }else{}
        //idle_state_handle();
        //NRF_LOG_FLUSH();
    
    }
}


/**
 * @}
 */
