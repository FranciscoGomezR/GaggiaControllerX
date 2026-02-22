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
#include "BLEspressoServices.h"

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
  TEMPCTRL_INIT_OK = 0,
  TEMPCTRL_INIT_ERROR,
  TEMPCTRL_LOAD_OK,
  TEMPCTRL_ERROR,
  TEMPCTRL_I_LOAD_OK
} tempCtrl_status_t;

typedef enum{
  SETPOINT_BREW=0,
  SETPOINT_STEAM,
  TEMPCTRL_SP_LOAD_OK
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
tempCtrl_status_t fcn_initCntrl_Temp(void);

/* call this function before activating the pump for pulling out a shot of spresso*/
tempCtrl_status_t fcn_loadIboost_ParamToCtrl_Temp(bleSpressoUserdata_struct *prt_profData);
/* call this function after deactivation the pump for pulling out a shot of spresso*/
tempCtrl_status_t fcn_multiplyI_ParamToCtrl_Temp(bleSpressoUserdata_struct *prt_profData, float factor);
/* call this function to load a new Set Point into the Temperature controller of the Boiler*/
tempCtrl_LoadSP_t fcn_loaddSetPoint_ParamToCtrl_Temp(bleSpressoUserdata_struct *prt_profData, tempCtrl_LoadSP_t Setpoint);

tempCtrl_status_t fcn_loadPID_ParamToCtrl_Temp(bleSpressoUserdata_struct *prt_profData);


float fcn_updateTemperatureController(bleSpressoUserdata_struct *prt_profData);


void fcn_startTempCtrlSamplingTmr(void);
void fcn_stopTempCtrlSamplingTmr(void);

void isr_HwTmr3_Period_EventHandler(nrf_timer_event_t event_type, void* p_context);