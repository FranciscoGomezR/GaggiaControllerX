#ifndef I2C_SENSORS_H__
#define I2C_SENSORS_H__

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
#include <stdint.h>
#include <string.h>
#include "boards.h"

#include "nrf_drv_twi.h"
#include "nrf_delay.h"
#include "x01_StateMachineControls.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
/* TWI instance ID. */
#define TWI_INSTANCE_ID     0

/* Common addresses definition for temperature sensor. */
#define TMP006_ADDR          (0x41)

#define TMP006_REG_TEMP      0x01U
#define TMP006_REG_CONF      0x02U

/* Mode for TMP006. */
#define NORMAL_MODE 0x70U

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
extern float eTMP006_Temp;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
void LM75B_set_mode(void);
void twi_init (void);
void read_sensor_data(void);
bool twi_operation_done(void);


#ifdef __cplusplus
}
#endif
#endif // I2C_SENSORS_H__