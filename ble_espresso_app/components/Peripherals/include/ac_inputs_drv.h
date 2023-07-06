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
extern struct_ControllerInputs sControllerInputs;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
void fcn_initACinput_drv(struct_ControllerInputs *ptr_instance);
void fcn_ACinput_drv(struct_ControllerInputs *ptr_instance);
extern void acinSteam_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
extern void acinBrew_eventHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);


#ifdef __cplusplus
}
#endif
#endif // SPI_SENSORS_H__