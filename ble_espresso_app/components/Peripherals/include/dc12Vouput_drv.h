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

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
typedef enum {
  DRV_12VO_INIT_AS_LAMP = 0,
  DRV_12VO_INIT_AS_OUT,
} dc12vout_status_t;
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
dc12vout_status_t fcn_initDC12Voutput_drv(void);
