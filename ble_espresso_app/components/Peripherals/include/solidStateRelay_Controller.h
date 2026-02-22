#ifndef SOLIDSTATERELAY_CONTROLLER_H__
#define SOLIDSTATERELAY_CONTROLLER_H__

#ifdef __cplusplus
extern  "C" {
#endif
/************************************************************************************
*	Copyright  			"Your Name / Company"										*
*   All Rights Reserved																*
*   The Copyright symbol does not also indicate public availability or publication.	*
* 																					*
* 							"YOUR NAME / COMPANY"									*
* 																					*
* - Driver:   			"Name of file".												*
* 																					*
* - Compiler:           Code Composer Studio (Licensed)								*
* - Version:			6.1.0.xxxxx													*
* - Supported devices:  "Microcontroller used" 										*
* 																				2012*
*************************************************************************************

*************************************************************************************
* 	Revision History:
*
*   Date          	CP#           Author
*   DD-MM-YYYY      XXXXX:1		Initials	Description of change
*   -----------   ------------  ---------   ------------------------------------
*  	XX-XX-XXXX		X.X			ABCD		"CHANGE"	
*
*************************************************************************************
* File/
*  "More detail description of the code"
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
*  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
*  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*  DEALINGS IN THE SOFTWARE.
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

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
#define AC_CYCLE_PERIOD       20000   //This period time is in micro-second
#define AC_PERCENT_STEP       50      //Number of cycle on the main
#define POWER_MAX_VALUE       1000

#define ZERO_CROSS            0
#define ANGLE                 1     
#define SSR_CTRL_BOILER_HEAT  ZERO_CROSS
#define FREQ_2HZ_2PER         0
#define FREQ_1HZ_1PER         1
#define SSR_CTRL_BOILER_RATE  FREQ_2HZ_2PER
//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
typedef enum {
    SSR_DRV_INIT_OK = 0,
    SSR_DRV_INIT_ERROR,
    SSR_STATE_ENGAGE,
    SSR_STATE_ACTIVE,
    SSR_STATE_OFF
} ssr_status_t;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
/* fcn_initSSRController_BLEspresso initiate the driver to control the following SSR  */
/* 1- SSR for Boiler Heater (0000 to 1000)     */
/* 2- SSR for the Pump      (0000 to 1000)     */
/* 3- SSR for the solenoid  (ON/OFF fashion)   */ 
ssr_status_t fcn_initSSRController_BLEspresso(void);
void fcn_boilerSSR_pwrUpdate( uint16_t outputPower);
void fcn_pumpSSR_pwrUpdate( uint16_t outputPower);

void fcn_SolenoidSSR_On(void);
ssr_status_t get_SolenoidSSR_State(void);
void fcn_SolenoidSSR_Off(void);

//External interrupt ISR has to be created in the main thread 
extern void isr_ZeroCross_EventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

//Timers' ISRs are created in this module and used inside of it.
extern void isr_BoilderSSR_EventHandler(nrf_timer_event_t event_type, void* p_context);
extern void isr_PumpSSR_EventHandler(nrf_timer_event_t event_type, void* p_context);


#ifdef __cplusplus
}
#endif
#endif // SPI_SENSORS_H__