/*
 * x04_Numbers.h
 *
 *  Created on: 09/03/2017
 *      Author: Juan Fco. Gómez
 */
#ifndef X04_NUMBERS_H_
#define X04_NUMBERS_H_

/* useulf link about float variables:   */
/* https://blog.demofox.org/2017/11/21/floating-point-precision/ */

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
