/*
 * x201_DigitalFiltersAlgorithm.c
 *
 *  Created on: 14/03/2012
 *      Author: uidw3542
 */

/************************************************************************************
*	Copyright Paxs Electronics 2012													*
*   All Rights Reserved																*
*   The Copyright symbol does not also indicate public availability or publication.	*
* 																					*
* 								PAXS ELECTRONICS									*
* 																					*
* - Driver:   			Digital Filter Driver										*
* 																					*
* - Compiler:           Code Composer Studio (Licensed)								*
* - Supported devices:  All Stellaris Family Cortex-M3.								*
* - AppNote:																		*
*																					*
* 	Created by: 		Juan Francisco Gomez Ruiz									*
*   Date Created: 		14 - MAR - 2012												*
*   Contac				pacoduz@gmail.com											*
* 																					*
* 	Description: 		Digital Filters Driver, FIR or IIR filter type selection	*
* 						Number of channels, Order selection, N-Tap selection. 		*
* 						*All this options aply to all channels.						*
*   Device supported and tested: - LM3S5749 - LM3S3748 -							*
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

void pfcn_InitRCFilterAlgorithm(struct_DigitalRCFilterParam* FilterParam, float Filt_FcutoffHz, float Filt_SamplingTimeS )
{
	// Reference Link: https://www.youtube.com/watch?v=MrbffdimDts&t=767s&ab_channel=Phil%E2%80%99sLab
	/* compute equivalent 'RC' constant from cut-off freq */
    float RC_constant = 1.0f / (6.28318530718f * Filt_FcutoffHz);
	/* Precompute filter coefficient from first-order Low-pass filter */
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


/*****************************************************************************
 * Function: 	pfcn_FiniteImpulseResponseFilterAlgorithm
 * Description:
 * Caveats:
 * Parameters:
 * Return:
 *****************************************************************************/
inline void pfcn_FiniteImpulseResponseFilterAlgorithm(float *ptr_DataInput, float *ptr_DataOuput, struct_DigitalFilterParam* FilterParam)
{
	volatile uint32_t Nvalue,Nchannel;
	float Zero = 0.0f;
	Nvalue = 0;
	Nchannel = 0;
	//Shift old sample out of filter memory, in all channels
	//-------------------------------------------------------
	while(Nchannel != (FilterParam->NumberChannel))
	{
		Nvalue=FilterParam->NumberTaps-1;
		while(Nvalue != (0))
		{
			*( FilterParam->ptr_HistorySamples + ((Nvalue)+(Nchannel*(FilterParam->NumberTaps))) ) = *( FilterParam->ptr_HistorySamples + ((Nvalue+(Nchannel*(FilterParam->NumberTaps)))-1) );
			Nvalue--;
		}
		Nchannel++;
	}
	// Insert new sample for all channels into filter memnory.
	//----------------------------------------------------------
	Nchannel=0;
	while(Nchannel != FilterParam->NumberChannel)
	{
		//Pass the Inpunt values to the Filter buffer "For each channel"
		*((FilterParam->ptr_HistorySamples)+ (Nchannel * FilterParam->NumberTaps)) = *((ptr_DataInput)+Nchannel);
		Nchannel ++;
	}

	Nvalue = 0;
	Nchannel = 0;
	while(Nchannel != (FilterParam->NumberChannel))
	{
		*(ptr_DataOuput + Nchannel)=Zero;
		Nchannel++;
	}
	Nchannel=0;
	while(Nchannel != (FilterParam->NumberChannel))
	{
		Nvalue=0;
		while(Nvalue != (FilterParam->NumberTaps))
		{
			*((ptr_DataOuput) + Nchannel) += (*(FilterParam->ptr_HistorySamples+(Nvalue)+(Nchannel*(FilterParam->NumberTaps))) * *(FilterParam->ptr_FilterCoefficients + Nvalue));
			Nvalue++;
		}
		Nchannel++;
	}
}

/*****************************************************************************
 * Function: 	pfcn_InitFilterAlgorithm
 * Description:
 * Caveats:
 * Parameters:
 * Return:
 *****************************************************************************/
inline void pfcn_InitFilterAlgorithm(struct_DigitalFilterParam* FilterParam, float* ptr_FilterCoeffAddress, uint8_t NoTaps, uint8_t NoChannels )
{
	FilterParam->ptr_HistorySamples =&FilterParam->HistorySamples[0];
	FilterParam->ptr_FilterCoefficients = ptr_FilterCoeffAddress;
	FilterParam->NumberTaps = NoTaps;
	FilterParam->NumberChannel = NoChannels;
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



