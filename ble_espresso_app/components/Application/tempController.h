/************************************************************************************
*************************************************************************************
* 	Revision History:
*
*   Date          	CP#           Author
*   DD-MM-YYYY      XXXXX:1		Initials	Description of change
*   -----------   ------------  ---------   ------------------------------------
*  	XX-XX-XXXX		X.X			ABCD		"CHANGE"	
*
*************************************************************************************
*
* File/
*  "More detail description of the code"
*
*
*/

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

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************.
    #define TEMP_CTRL_GAIN_P        100.0f
    #define TEMP_CTRL_GAIN_I        0.1f
    #define TEMP_CTRL_HIST_LIMIT    200.0f
    #define TEMP_CTRL_GAIN_D        0.0f
    #define TEMP_CTRL_MAX           900.0f

    #define TEMP_CTRL_GAIN_WINDUP   0.005f

   
    #define TEMP_CTRL_SAMPLING_T    0.01f     //Sampling Time in seconds
//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************


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
void fcn_initTemperatureController(void);
void fcn_startTemperatureController(void);
void fcn_stopTemperatureController(void);

void isr_SamplingTime_EventHandler(nrf_timer_event_t event_type, void* p_context);