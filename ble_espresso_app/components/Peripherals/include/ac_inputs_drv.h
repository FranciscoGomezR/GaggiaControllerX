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
#define BREW_CFG_THRESHOLD_N  8
#define BREW_CFG_EVT_HANDLER  acinBrew_eventHandler

#define STEAM_CFG_PIN_ID       inSTEAM_PIN
#define STEAM_CFG_STATUS       false
#define STEAM_CFG_THRESHOLD_N  8
#define STEAM_CFG_EVT_HANDLER  acinSteam_eventHandler

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
typedef struct
{
  uint8_t     pinID;
  uint16_t    Counter;
  bool        Status;
  bool        prevStatus;
  uint8_t     smEvent;
  uint16_t    nCycles;
  nrfx_gpiote_evt_handler_t ext_isr_handler;
}struct_AcInputPin;

typedef struct
{
  struct_AcInputPin  Brew;
  struct_AcInputPin  Steam;
}struct_ControllerInputs;

enum{
  smS_NoChange=0,
  smS_Change
};

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
void fcn_initACinput_drv(void);
void fcn_ACinput_drv(void);

uint8_t fcn_StatusChange_Brew(void);
uint8_t fcn_StatusChange_Steam(void);
void fcn_StatusReset_Brew(void);
void fcn_StatusReset_Steam(void);

bool fcn_GetInputStatus_Brew(void);
bool fcn_GetInputStatus_Steam(void);

extern void acinSteam_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
extern void acinBrew_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

#ifdef __cplusplus
}
#endif
#endif // SPI_SENSORS_H__