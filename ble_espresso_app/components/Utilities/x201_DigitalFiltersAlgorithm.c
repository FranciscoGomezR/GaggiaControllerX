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

void pfcn_InitRCFilterAlgorithm(lpf_rc_param_t* FilterParam, float Filt_FcutoffHz, float Filt_SamplingTimeS )
{
    /* compute equivalent 'RC' constant from cut-off freq */
    float RC_constant = 1.0f / (6.28318530718f * Filt_FcutoffHz);
    
    /* Precompute filter coefficient from first-order Low-pass filter */
    FilterParam->FilterRCCoefficients[0] =  Filt_SamplingTimeS / (Filt_SamplingTimeS + RC_constant);
    FilterParam->FilterRCCoefficients[1] =  RC_constant / (RC_constant + Filt_SamplingTimeS);

    FilterParam->DataOut_n_1 = 0.0f;
    FilterParam->DataOut_n = 0.0f;
}

void pfcn_RCFilterAlgorithm(lpf_rc_param_t* FilterParam, float DataIn )
{
    /* Shift output samples, DataOut_n_1 will store previous output */
    FilterParam->DataOut_n_1 = FilterParam->DataOut_n;

    /* compute new output samples */
    FilterParam->DataOut_n = (FilterParam->FilterRCCoefficients[0] * DataIn) + (FilterParam->FilterRCCoefficients[1] * FilterParam->DataOut_n_1 );
}

/*****************************************************************************
 * Function: 	lpf_rc_calculate_const
 * Description: compute equivalent 'RC' constant from cut-off freq 
 *****************************************************************************/
void lpf_rc_calculate_const(lpf_rc_param_t* filterParam, float feq_cutoff_hz )
{
    filterParam->rc_constant = 1.0f / (6.28318530718f * feq_cutoff_hz);
    filterParam->DataOut_n = 0.0f;
    filterParam->DataOut_n_1 = 0.0f;
}

/*****************************************************************************
 * Function: 	lpf_rc_update
 * Description: Compute filter coefficient from first-order Low-pass filter based on new Sampling Time or dt
                once we have the new coeff, compute new output
 * Return:      Filter output
 *****************************************************************************/
float lpf_rc_update(lpf_rc_param_t* filterParam, float data_In, float t_seconds )
{
    /* Shift output samples, DataOut_n_1 will store previous output */
    filterParam->DataOut_n_1 = filterParam->DataOut_n;

    /* Compute filter coefficient from first-order Low-pass filter */
    filterParam->FilterRCCoefficients[0] =  t_seconds / (t_seconds + filterParam->rc_constant);
    filterParam->FilterRCCoefficients[1] =  filterParam->rc_constant / (filterParam->rc_constant + t_seconds); 

    /* compute new output samples */
    filterParam->DataOut_n =    (filterParam->FilterRCCoefficients[0] * data_In) 
                              + (filterParam->FilterRCCoefficients[1] * filterParam->DataOut_n_1 );

    /* return filtered sample */
    return filterParam->DataOut_n;

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



