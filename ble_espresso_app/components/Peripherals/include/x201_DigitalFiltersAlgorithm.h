/*
 * x201_DigitalFiltersAlgorithm.h
 *
 *  Created on: 09/03/2017
 *      Author: Juan Fco. Gómez
 */
#ifndef X201_DIGITALFILTERSALGORITHM_H_
#define X201_DIGITALFILTERSALGORITHM_H_
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
*   Date          CP#           Author
*   MM-DD-YY      XXXXX:1       Initials   Description of change
*   -----------   -----------   --------   ------------------------------------
*    03-14-12      	1.0             JFGR	Initial version.
*    03-09-17		2.0				JFGR	Second version.
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
	#include <stdlib.h>

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
typedef struct{
    float DataOut_n;
    float DataOut_n_1;
    float FilterRCCoefficients[2];
}struct_DigitalRCFilterParam;

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
extern void pfcn_InitRCFilterAlgorithm(struct_DigitalRCFilterParam* FilterParam, float Filt_FcutoffHz, float Filt_SamplingTimeS );
extern void pfcn_RCFilterAlgorithm(struct_DigitalRCFilterParam* FilterParam, float DataIn );

#endif /* 02_MAL_ECU_DRIVERS_X201_DIGITALFILTERSALGORITHM_H_ */
