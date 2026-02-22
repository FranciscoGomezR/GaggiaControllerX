#ifndef AC_INPUTS_DRV_H__
#define AC_INPUTS_DRV_H__

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
										2012*
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
#include "boards.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "app_error.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
#define BREW_CFG_PIN_ID       inBREW_PIN
#define BREW_CFG_STATUS       false
#define BREW_CFG_EVT_HANDLER  acinBrew_eventHandler

#define STEAM_CFG_PIN_ID       inSTEAM_PIN
#define STEAM_CFG_STATUS       false
#define STEAM_CFG_EVT_HANDLER  acinSteam_eventHandler

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
typedef enum {
  DRV_AC_INPUT_INIT_OK = 0,
  DRV_AC_INPUT_INIT_ERROR,
  AC_SWITCH_ASSERTED,
  AC_SWITCH_DEASSERTED
} acInput_status_t;

//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
//extern struct_ControllerInputs sIO_ACinput;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
acInput_status_t fcn_initACinput_drv(void);
acInput_status_t fcn_GetInputStatus_Brew(void);
acInput_status_t fcn_GetInputStatus_Steam(void);
//This function shall be called every 60ms.
void fcn_SenseACinputs_Sixty_ms(void);

//GPIO' ISRs are created in this module and used inside of it.
extern void acinSteam_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
extern void acinBrew_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

#ifdef __cplusplus
}
#endif
#endif // SPI_SENSORS_H__