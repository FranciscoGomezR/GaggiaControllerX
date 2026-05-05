/************************************************************************************
*************************************************************************************
* 	Revision History:
*
*************************************************************************************/
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_drv_timer.h"
#include "app_error.h"
#include "x02_FlagValues.h"
#include "x205_PID_Block.h"
#include "espressoMachineServices.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************.
#define TEMP_CTRL_KP          9.52156f      //gain 550*Kc
#define TEMP_CTRL_KI          0.3f          //TauI = 7.17
#define TEMP_CTRL_KI_BOOST    (0.3f * 6.5f)    
#define TEMP_CTRL_HIST_LIMIT  100.0f
#define TEMP_CTRL_KD          0.0f          //TauD= 1.46 13.924f 
#define TEMP_CTRL_MAX         1000.0f
   
//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
 typedef enum {
  TEMP_CTRL_INIT_OK = 0,
  TEMP_CTRL_INIT_ERROR,
  TEMP_CTRL_LOAD_OK,
  TEMP_CTRL_ERROR,
  TEMP_CTRL_I_LOAD_OK
} tempCtrl_status_t;

typedef enum{
  SET_POINT_BREW=0,
  SET_POINT_STEAM,
  SET_POINT_LOAD_OK
} tempCtrl_LoadSP_t;

//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
//extern volatile PID_Block_fStruct sBoilerTempCtrl;
//extern volatile struct_PIDtimer sPIDtimer;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
tempCtrl_status_t temp_ctrl_init(void);

/* call this function before activating the pump for pulling out a shot of spresso*/
tempCtrl_status_t temp_ctrl_set_operational_integral_gain(espresso_user_config_t *ptr_prof_data);
/* call this function after deactivation the pump for pulling out a shot of spresso*/
tempCtrl_status_t temp_ctrl_scale_integral_gain(espresso_user_config_t *ptr_prof_data, float factor);
/* call this function to load a new Set Point into the Temperature controller of the Boiler*/
tempCtrl_LoadSP_t temp_ctrl_set_boiler_setpoint(espresso_user_config_t *ptr_prof_data, tempCtrl_LoadSP_t Setpoint);

tempCtrl_status_t temp_ctrl_set_pid_config(espresso_user_config_t *ptr_prof_data);


float temp_ctrl_update(espresso_user_config_t *ptr_prof_data);

void temp_ctrl_start_sampling_timer(void);
void temp_ctrl_stop_sampling_timer(void);

void temp_ctrl_sampling_timer_event_handler(nrf_timer_event_t event_type, void* p_context);