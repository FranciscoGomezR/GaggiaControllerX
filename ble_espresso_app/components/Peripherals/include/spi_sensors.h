#ifndef SPI_SENSORS_H__
#define SPI_SENSORS_H__

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
#include "nrf_drv_spi.h"
#include "x01_StateMachineControls.h"
//#include "nrf_log.h"
//#include "nrf_log_ctrl.h"
//#include "nrf_log_default_backends.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
#define SPI_INSTANCE  1 /**< SPI instance index. */

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
extern float eThermocoupleTemp;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
void spim_init (void);
void spim_Read_TempSensor(void);
bool spim_operation_done(void);





#ifdef __cplusplus
}
#endif
#endif // SPI_SENSORS_H__