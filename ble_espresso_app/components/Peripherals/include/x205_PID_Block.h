/*
 * x205_PID_Block.h
 *
 *  Created on: Nov 7, 2017
 *      Author: Juan Fco. Gómez
 */

#ifndef X205_PID_BLOCK_H_
#define X205_PID_BLOCK_H_
/************************************************************************************
*	Copyright Paxs Electronics 2012													*
*   All Rights Reserved																*
*   The Copyright symbol does not also indicate public availability or publication.	*
* 																					*
* 								PAXS ELECTRONICS									*
* 																					*
* - Driver:   			PID control Algorithm										*
* 																					*
* - Compiler:           Code Composer Studio (Licensed)								*
* - Supported devices:  All Stellaris Family Cortex-M4.								*
* - AppNote:			AVR221 - Discrete PID Controller							*
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
*   Date          CP#           Author
*   MM-DD-YY      XXXXX:1       Initials   Description of change
*   -----------   -----------   --------   ------------------------------------
*    03-01-12      	1.0             JFGR	Initial version.
*    01-07-17		2.0				JFGR	Second version.
*    03-10-17		2.1				JFGR	Second version, now supporting LowPass Filter before D term

*************************************************************************************
*
* File/
*	PID Algorithm to controll the QuaXcopter. 4 functions user available:
*
*	https://github.com/bitcraze/crazyflie-firmware/blob/master/src/modules/src/pid.c
* 	Outputs:
* 	None.
* 	**********************************************************************
*
* 	IQ Range:
* 	---------------------------------------------------------
* 	Data Type	Min			Max					Resolution
* 	---------------------------------------------------------
* 	_iq23		  -256	  255.999 999 981		0.000 000 119
* 	_iq15		-65536	65535.999 969 482		0.000 000 119
* 	---------------------------------------------------------
*/
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
	#include <stdbool.h>
	#include <stdint.h>
	#include "x02_FlagValues.h"
	#include "x201_DigitalFiltersAlgorithm.h"

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
      float Input;
      float SetPoint;
      float Output;
      float dt;
      bool PID_OUTPUT_GAIN_CTRL;
      float Kpid;
      float OutputLimit;
    
      float PreviousError;
      bool P_TERM_CTRL;
      float Kp;
      bool I_TERM_CTRL;
      float Ki;
      float HistoryError;
      float HistoryErrorLimit;
      bool I_ANTIWINDUP_CTRL;
      bool WindupStatus;
      float Kwindup;
      float WindupError;
      bool D_TERM_CTRL;
      float Kd;
      bool D_TERM_LP_FILTER_CTRL;
      struct_DigitalRCFilterParam sLPF_Param;
      const float LPF_FCUTOFF_HZ;
      const float LPF_SAMPLING_S;	
  }PID_Block_fStruct;


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
	extern void fcn_PID_Block_Iteration( PID_Block_fStruct * ptr_sPIDparam);

	extern void fcn_PID_Block_ResetI( PID_Block_fStruct * ptr_sPIDparam, float Attenuator);

	extern void fcn_PID_Block_Dterm_LPF_Init( PID_Block_fStruct* ptr_sPIDparam);
	extern void fcn_PID_Block_Init_Dterm_LPfilter( PID_Block_fStruct* ptr_sPIDparam,
	                                                      float* ptr_FilterCoeffAddress,
	                                                      uint8_t NoTaps, uint8_t NoChannels);

#endif /* 02_MAL_ECU_DRIVERS_X205_PID_BLOCK_H_ */
