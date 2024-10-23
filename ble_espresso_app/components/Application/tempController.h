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
* 																					*
* - AppNote:			"Name of file that help to comprehend the code"				*
*																					*
* 	Created by: 		"Your Name"													*
*   Date Created: 		"date of creation"											*
*   Contact:			"Email"														*
* 																					*
* 	Description: 		"description".												*
*   Device supported 																*
*   and tested: 		- MSP430F5529 - 											*
*   					-  	- 														*
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
*
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

    #define TEMP_CTRL_GAIN_WINDUP   .005f

    //t=~333.3ms -> 0.341130f
    #define TEMP_CTRL_ITERATION_T   0.3333f     //Iteration Time in Seconds
//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
typedef struct
{
    nrf_drv_timer_t             hwTmr;              ///HW-Timer that will control Relay trigger
    nrfx_timer_event_handler_t  hwTmr_isr_handler;
    uint32_t                    tmrPeriod_us;
    uint32_t                    tmrPeriod_ticks;
    bool                        status;
} struct_PIDtimer;

//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
extern volatile PID_Block_fStruct sBoilerTempCtrl;
extern volatile struct_PIDtimer sPIDtimer;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
void fcn_initTemperatureController(PID_Block_fStruct *ptr_pidTempCtrl);
void fcn_startTemperatureController(void);
void fcn_stopTemperatureController(void);

void isr_TempController_EventHandler(nrf_timer_event_t event_type, void* p_context);