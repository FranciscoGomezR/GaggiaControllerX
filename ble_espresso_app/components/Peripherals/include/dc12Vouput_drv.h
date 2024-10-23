/************************************************************************************
*	Copyright  			"Your Name / Company"										*
*   All Rights Reserved																*
*   The Copyright symbol does not also indicate public availability or publication.	*																			2012*
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
#include <string.h>
#include <stdio.h>
#include "boards.h"
#include "nrf_drv_pwm.h"
#include "x01_StateMachineControls.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
typedef struct
  {
      StateMachineCtrl_Struct sDrv;
      uint8_t outState;
  }StateMachine12Vout_Struct;

enum{
  st_Idle = 0,
  st_LoadDimOn,
  st_turningON,
  st_LoadDimOff,
  st_turningOFF,
  st_LoadDimDown,
  st_DimDown,
  st_LoadDimUp,
  st_DimUp
};

enum{
  outst_ON = 0,
  outst_OFF,
  outst_2_3
};

extern volatile uint32_t flag;
extern StateMachine12Vout_Struct s12Vout;

//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************


//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
void fcn_initDC12Voutput_drv(void);
void sm_DC12Voutput_drv(StateMachine12Vout_Struct *ptr_drvState);