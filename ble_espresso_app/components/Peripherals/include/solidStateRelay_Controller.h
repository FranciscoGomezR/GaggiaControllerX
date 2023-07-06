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

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
#define AC_CYCLE_PERIOD   20000
#define AC_PERCENT_STEP   50
#define POWER_MAX_VALUE   1000
//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
/**
 * @brief Timer driver instance data structure.
 */
typedef struct
{
    uint16_t    tStep;
    uint16_t    tPeriod;
    uint16_t    tTrigger;
    uint16_t    tZCdelay;
    uint32_t    cPeriod;
    uint32_t    cTrigger;
    uint32_t    cZCdelay;
}struct_ssrTiming;

typedef struct
{
    nrf_drv_timer_t             hwTmr;              ///HW-Timer that will control Relay trigger
    nrfx_timer_event_handler_t  hwTmr_isr_handler;
    uint8_t                     in_zCross;    ///AC Zero-cross input pin
    uint8_t                     out_SSRelay;     ///controller output pin
    nrfx_gpiote_evt_handler_t   zcross_isr_handler;
    struct_ssrTiming            sSRR_timing_us;
    uint8_t                     smTrigStatus;
    bool                        status;
    uint16_t                    srrPower;
} struct_ssrController;

enum{
  smS_Release=0,
  smS_Engage
};

enum{
  ssrBUSY=0,
  ssrREADY_TO_UPDATE
};
//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
extern volatile struct_ssrController sSSRdrvConfig;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
void fcn_initSsrController(struct_ssrController * ptr_instance);
extern void isr_SSRcontroller_EventHandler(nrf_timer_event_t event_type, void* p_context);
extern void isr_ZeroCross_EventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

void fcn_SSR_pwrUpdate(struct_ssrController * ptr_instance, uint16_t outputPower);
void fcn_SSR_ctrlUpdate(struct_ssrController * ptr_instance);



#ifdef __cplusplus
}
#endif
#endif // SPI_SENSORS_H__