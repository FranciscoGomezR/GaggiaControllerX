/*
 * x205_PID_Block.h
 *
 *  Created on: Nov 7, 2017
 *      Author: Juan Fco. G�mez
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
    float ProcessVariable;
    float SetPoint;
    uint32_t TimeMilis;
}input_PID_Block_fStruct;
 
typedef struct
{
    input_PID_Block_fStruct feedPIDblock;
    float PrevError;
    float prevT_Milis;
    
    float OutputLimit;
    int8_t OutputSaturationOut;
    float Output;

    bool P_TERM_CTRL;
    float Kp;

    bool I_TERM_CTRL;
    float Ki;
    float HistoryError;
    float IntegralLimit;
    bool I_ANTIWINDUP_CTRL;
    bool WindupClampStatus;


    bool D_TERM_CTRL;
    float Kd;
    bool D_TERM_LP_FILTER_CTRL;
    lpf_rc_param_t sLPF_Param;
    float LPF_FCUTOFF_HZ;
    
}PID_Block_fStruct;

typedef struct
{
    input_PID_Block_fStruct feedPIDblock;
    float   prevT_Milis;
    float   errorK_1;
    float   errorK_2;
    
    float   OutputLimit;
    int8_t  OutputSaturationOut;
    float   Output;

    bool    P_TERM_CTRL;
    float   Kp;

    bool    I_TERM_CTRL;
    bool    I_ANTIWINDUP_CTRL;
    float   Ki;
    float   HistoryError;
    float   IntegralError;
    float   IntegralLimit;
    bool    WindupClampStatus;

    bool    D_TERM_CTRL;
    bool    D_TERM_FILTER_CTRL;
    float   prevPV;
    float   Kd;  
}PID_IMC_Block_fStruct;

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
extern float fcn_update_PID_Block(float fInput, 
                                  float fSetpoint, 
                                  uint32_t timeMilis,  
                                  PID_Block_fStruct * ptr_sPIDparam );

extern float fcn_update_PIDimc_typeA(PID_IMC_Block_fStruct * ptr_sPIDparam );
extern float fcn_update_PIDimc_typeB(PID_IMC_Block_fStruct * ptr_sPIDparam );

extern void fcn_PID_Block_ResetI( PID_Block_fStruct * ptr_sPIDparam, float Attenuator);

extern void fcn_PID_Block_Dterm_LPF_Init( PID_Block_fStruct* ptr_sPIDparam);
extern void fcn_PID_Block_Init_Dterm_LPfilter( PID_Block_fStruct* ptr_sPIDparam,
                                                      float* ptr_FilterCoeffAddress,
                                                      uint8_t NoTaps, uint8_t NoChannels);

#endif /* 02_MAL_ECU_DRIVERS_X205_PID_BLOCK_H_ */
