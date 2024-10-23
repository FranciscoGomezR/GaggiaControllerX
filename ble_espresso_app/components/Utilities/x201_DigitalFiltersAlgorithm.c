/*
 * x201_DigitalFiltersAlgorithm.c
 *
 *  Created on: 14/03/2012
 *      Author: uidw3542
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
*  File/
*
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
#include "x201_DigitalFiltersAlgorithm.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************

//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
// Reference Link: https://www.youtube.com/watch?v=MrbffdimDts&t=767s&ab_channel=Phil%E2%80%99sLab

void pfcn_InitRCFilterAlgorithm(struct_DigitalRCFilterParam* FilterParam, float Filt_FcutoffHz, float Filt_SamplingTimeS )
{
    float RC_constant = 1.0f / (6.28318530718f * Filt_FcutoffHz);

    FilterParam->FilterRCCoefficients[0] =  Filt_SamplingTimeS / (Filt_SamplingTimeS + RC_constant);
    FilterParam->FilterRCCoefficients[1] =  RC_constant / (RC_constant + Filt_SamplingTimeS);

    FilterParam->DataOut_n_1 = 0.0f;
    FilterParam->DataOut_n = 0.0f;
}

void pfcn_RCFilterAlgorithm(struct_DigitalRCFilterParam* FilterParam, float DataIn )
{
    FilterParam->DataOut_n_1 = FilterParam->DataOut_n;
    FilterParam->DataOut_n = (FilterParam->FilterRCCoefficients[0] * DataIn) + (FilterParam->FilterRCCoefficients[1] * FilterParam->DataOut_n_1 );
}


//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	InitClocks
 * Description:
 * Caveats:
 * Parameters:
 * Return:
 *****************************************************************************/
 // Public function 2



