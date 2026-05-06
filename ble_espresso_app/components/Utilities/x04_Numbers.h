/*
 * x04_Numbers.h
 *
 *  Created on: 09/03/2017
 *      Author: Juan Fco. Gmez
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
/* Validate a single float field.
 * If *value is NaN, Inf, or outside [min, max] it is replaced by safeDefault.
 * Returns true if the value was already valid, false if it was corrected. */
bool validate_float_in_range(float *value, float min, float max, float safeDefault);

extern int8_t constrain_within_floats( float* Number, float LowerLimit, float UpperLimit);
extern void constrain_within_int_values( long* Number,  long UpperLimit,  long LowerLimit);
extern void add_hysteresis_within_float( float* Number, float NumberWihtoutHyst, float OffsetLimit);
extern void add_hysteresis_minus_offset( float* Number, float NumberWihtoutHyst, float OffsetUpperLimit, float OffsetLowerLimit);

extern float chr_array_to_float( char * ptrArray, char noDigits, char noDecimals);
extern void  float_to_chr_array( float fNum, uint8_t * ptrArray, char noDigits, char noDecimals);

#endif /* 04_UTILITIES_X04_NUMBERS_H_ */
