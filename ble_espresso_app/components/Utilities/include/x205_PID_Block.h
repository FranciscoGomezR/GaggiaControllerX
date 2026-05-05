/*
 * x205_PID_Block.h
 *
 *  Created on: Nov 7, 2017
 *      Author: Juan Fco. G�mez
 */

#ifndef X205_PID_BLOCK_H_
#define X205_PID_BLOCK_H_
/* File/
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
}pid_input_t;
 
typedef struct
{
    pid_input_t feedPIDblock;
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
}pid_imc_block_t;

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

float pid_imc_compute(pid_imc_block_t * ptr_pid_param_s );


#endif /* 02_MAL_ECU_DRIVERS_X205_PID_BLOCK_H_ */
