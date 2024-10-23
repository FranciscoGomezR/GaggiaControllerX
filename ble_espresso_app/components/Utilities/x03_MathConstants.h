/*
 * 0x03_MathConstants.h
 *
 *  Created on: 16/12/2016
 *      Author: Juan Fco. Gomez
 */

#ifndef X03_MATHCONSTANTS_H_
#define X03_MATHCONSTANTS_H_
/************************************************************************************
*	Copyright Paxs Electronics 2012													*
*   All Rights Reserved																*
*   The Copyright symbol does not also indicate public availability or publication.	*
* 																					*
* 								PAXS ELECTRONICS									*
* 																					*
* - Driver:   			Numerical Constants											*
* 																					*
* - Compiler:           Code Composer Studio (Licensed)								*
* - Supported devices:  All Stellaris Family Cortex-M3.								*
* 						And TIVA Family												*
* - AppNote:																		*
*																					*
* 	Created by: 		Juan Francisco Gomez Ruiz									*
*   Date Created: 		27 - Dec - 2011												*
*   Contac				juan.fco.gomez.riuz@gmail.com								*
* 																					*
* 	Description: 		Full Extended Direction Cosine MAtrix Algorithm.			*
* 						Support:													*
* 						- IMU board Orientation.									*
* 						- Short/int or _IQ input variables.							*
*   Device supported and tested: 													*
*   					- LM3S5749 - LM3S3748 -										*
*   					- TM4C123GH6PM												*
* 																				2012*
*************************************************************************************

*************************************************************************************
* 	Revision History:
*
*   Date          CP#           Author
*   DD-MM-YY      XXXXX:1       Initials   Description of change
*   -----------   -----------   --------   ------------------------------------
*    27-12-11      	1.0             JFGR	Initial version. Short/int input variables only.
*    07-03-12		1.1				JFGR	Initial Version. Support _IQ input variables
*    13-12-16		1.2				JFGR	Second Revison

*************************************************************************************
*
* File/
*	Direction Cosine Matrix.
*	"Hardware abstraction Layer - Second level".
* 	These Functions are implemented with Fixed-Point Arithmetic,
*   giving a good precision and faster response than Float-point
*   variables. This code uses a fixed-point library to do all
*   Math. The IQMath Library provided by Texas Instruments to
*   be used on Stellaris and C2000 Microcontrollers families.
*/
//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
	#define TWO_PI                  6.283185307f
    #define PI						3.141592654f
	#define HALF_PI					1.570796327f

	#define DEGREES_TO_RADIANS		0.017453292521f
	#define RADIANS_TO_DEGREES		57.29577951f

	#define GRAVITY_SCALAR			9.80665f

	#define KNOTS_TO_M_PER_SEC		0.514444444444f
	#define MINUTES_TO_HOURS		0.016666666667f

#endif /* 04_UTILITIES_0X03_MATHCONSTANTS_H_ */
