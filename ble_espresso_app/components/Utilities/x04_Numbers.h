/*
 * x04_Numbers.h
 *
 *  Created on: 09/03/2017
 *      Author: Juan Fco. Gómez
 */
#ifndef X04_NUMBERS_H_
#define X04_NUMBERS_H_
/************************************************************************************
*	Copyright Paxs Electronics 2012													*
*   All Rights Reserved																*
*   The Copyright symbol does not also indicate public availability or publication.	*
* 																					*
* 								PAXS ELECTRONICS									*
* 																					*
* - Driver:   			Power Motors Distribution									*
* 																					*
* - Compiler:           Code Composer Studio (Licensed)								*
* - Supported devices:  All Stellaris Family Cortex-M4.								*
* - AppNote:																		*
*																					*
* 	Created by: 		Juan Francisco Gomez Ruiz									*
*   Date Created: 		1 - MAR - 2012												*
*   Contac				juan.fco.gomez.ruiz@gmail.com								*
* 																					*
* 	Description: 																	*
*   Device supported and tested: - TM4C123GH6PMI-									*
* 																				2012*
*************************************************************************************

*************************************************************************************
* 	Revision History:
*
*   Date          	CP#           Author
*   DD-MM-YYYY      XXXXX:1		Initials	Description of change
*   -----------   ------------  ---------   ------------------------------------
*  	09-03-2017			1.0			JFGR	First version
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
        #include <math.h>
	#include "x03_MathConstants.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
    #define POSITIVE_SATURATION   1
    #define NEGATIVE_SATURATION  -1
    #define NO_SATURATION         0
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

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
    extern int8_t fcn_Constrain_WithinFloats( float* Number, float LowerLimit, float UpperLimit);
    extern void fcn_Constrain_WithinIntValues( long* Number,  long UpperLimit,  long LowerLimit);
    extern void fcn_AddHysteresis_WithinFloat( float* Number, float NumberWihtoutHyst, float OffsetLimit);
    extern void fcn_AddHysteresisMinusOffset( float* Number, float NumberWihtoutHyst, float OffsetUpperLimit, float OffsetLowerLimit);

    extern float fcn_ChrArrayToFloat( char * ptrArray, char noDigits, char noDecimals);
    extern void  fcn_FloatToChrArray( float fNum, uint8_t * ptrArray, char noDigits, char noDecimals);
#endif /* 04_UTILITIES_X04_NUMBERS_H_ */
