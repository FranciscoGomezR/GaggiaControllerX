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
    float       processVariable;
    float       setPoint;
    uint32_t    timeMsecs;
}pid_input_t;

typedef struct
{
    pid_input_t feedPidBlock;
    uint32_t    prevTimeMsecs;
    float       errorK1;
    float       errorK2;
    float       output;
    float       outputLimit;

    float       kp;
    float       ki;
    float       historyError;
    float       integralError;
    float       integralLimit;
    float       kd;

    /* 3-state saturation flag (NO/POSITIVE/NEGATIVE_SATURATION) */
    int8_t      outputSaturation;

    /* control/status flags packed into one storage unit */
    bool        isPTermEnabled       : 1;
    bool        isITermEnabled       : 1;
    bool        isIAntiwindupEnabled : 1;
    bool        isDTermEnabled       : 1;
    bool        flagWindupClamped    : 1;
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
