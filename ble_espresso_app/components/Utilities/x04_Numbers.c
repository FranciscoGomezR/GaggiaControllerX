/*
 * x04_Numbers.c
 *
 *  Created on: 09/03/2017
 *      Author: Juan Fco. Gómez
 */

//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "x04_Numbers.h"

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
/*****************************************************************************
 * Function: 	fcn_Constrain_WithinIQValues
 * Description:
 * Caveats:
 * Parameters:
 * Return:
 *****************************************************************************/


/*****************************************************************************
 * Function: 	fcn_Constrain_WithinIQValues
 * Description:
 * Caveats:
 * Parameters:
 * Return:      0   -> Number within constrains
                -1  -> Constrainted to negative
                +1  -> Constrained to positive
 *****************************************************************************/
	int8_t inline fcn_Constrain_WithinFloats( float* Number, float UpperLimit, float LowerLimit)
	{
		if( *Number > UpperLimit )
		{
			*Number = UpperLimit;
                        return POSITIVE_SATURATION;
		}else{
			if( *Number < LowerLimit )
			{
				*Number = LowerLimit;
                                return POSITIVE_SATURATION;
			}else{}
		}
                return NO_SATURATION;
	}



/*****************************************************************************
 * Function: 	fcn_AddHysteresis_WithinFloat
 * Description:
 * Caveats:
 * Parameters:
 * Return:
 *****************************************************************************/
	void inline fcn_AddHysteresis_WithinFloat( float* Number, float NumberWihtoutHyst, float OffsetLimit)
	{
		if( *Number <= OffsetLimit && *Number >= (-OffsetLimit))
		{
			*Number = NumberWihtoutHyst;
		}else{}
	}

/*****************************************************************************
 * Function: 	fcn_AddHysteresisMinusOffset_WithinIQValues
 * Description:
 * Caveats:
 * Parameters:
 * Return:
 *****************************************************************************/
	void inline fcn_AddHysteresisMinusOffset( float* Number, float NumberWihtoutHyst, float OffsetUpperLimit, float OffsetLowerLimit)
	{
	    uint32_t flag=0;
	    if(*Number >= 0.0f)
        {
	        flag=1;
        }else{
            flag=0;
            *Number = fabsf(*Number);
        }
	    if( *Number <= OffsetLowerLimit  )
        {
            *Number = 0.0f;
        }else{
            if( *Number <= OffsetUpperLimit && *Number >= OffsetLowerLimit)
            {
                *Number = *Number- OffsetLowerLimit;
            }else{}
        }
	    if(flag == 0)
	    {
	        *Number = *Number * -1.0f;
	    }else{}
	}



/*****************************************************************************
 * Function: 	fcn_Constrain_WithinIntValues
 * Description:
 * Caveats:
 * Parameters:
 * Return:
 *****************************************************************************/
	inline void inline fcn_Constrain_WithinIntValues( long* Number,  long UpperLimit,  long LowerLimit)
	{
		if( *Number > UpperLimit )
		{
			*Number = UpperLimit;
		}else{
			if( *Number < LowerLimit )
			{
				*Number = LowerLimit;
			}else{}
		}
	}

/*****************************************************************************
 * Function: 	fcn_ChrArrayToFloat
 * Description:
 * Caveats:
 * Parameters:
 * Return:
 *****************************************************************************/
    float fcn_ChrArrayToFloat( char * ptrArray, char noDigits, char noDecimals)
    {
      static float fNumber;
      fNumber = 0.0f;
      switch(noDigits)
      {
         case 4:
            fNumber = (float)((*ptrArray - 48) * 1000);
            ptrArray++;
            fNumber += (float)((*ptrArray - 48) * 100);
            ptrArray++;
            fNumber += (float)((*ptrArray - 48) * 10);
            ptrArray++;
            fNumber += (float)((*ptrArray - 48) * 1);
         break;
         case 3:
            fNumber = (float)((*ptrArray - 48) * 100);
            ptrArray++;
            fNumber += (float)((*ptrArray - 48) * 10);
            ptrArray++;
            fNumber += (float)((*ptrArray - 48) * 1);
         break;
         case 2:
            fNumber = (float)((*ptrArray - 48) * 10);
            ptrArray++;
            fNumber += (float)((*ptrArray - 48) * 1);
         break;
         case 1:
            fNumber = (float)((*ptrArray - 48) * 1);
         break;
         default:
            fNumber = 123.0f;
         break;
      }
      ptrArray++;
      switch(noDecimals)
      {
        case 2:
          fNumber += (float)((float)(*ptrArray - 48) * 0.1);
          ptrArray++;
          fNumber += (float)((float)(*ptrArray - 48) * 0.01);
        break;
        case 1:
          fNumber += (float)((float)(*ptrArray - 48) * 0.1);
        break;
        default:
          fNumber = 0.123f;
         break;
      }
      return fNumber;
    }


/*****************************************************************************
* Function: 	fcn_FloatToChrArray
* Description:
* Caveats:
* Parameters:
* Return:
*****************************************************************************/
    void  fcn_FloatToChrArray( float fNum, uint8_t * ptrArray, char noDigits, char noDecimals)
    {
      volatile int32_t fNumber;
      fNumber = (int32_t)(fNum);
      switch(noDigits)
      {
         case 3:
            *ptrArray = fNumber / 100;
            *(ptrArray+1) = (fNumber - (*(ptrArray)*100))/10;
            *(ptrArray+2) = (fNumber - (*(ptrArray)*100)-(*(ptrArray+1)*10) );
            *ptrArray     += 0x30;
            *(ptrArray+1) += 0x30;
            *(ptrArray+2) += 0x30;
            ptrArray += 3;          
         break;
         case 2:
            *ptrArray =     fNumber / 10;
            *(ptrArray+1) = (fNumber - (*(ptrArray)*10));
            *ptrArray     += 0x30;
            *(ptrArray+1) += 0x30;
            ptrArray += 2; 
         break;
         case 1:
            *ptrArray =     (uint8_t)fNumber;  
            *ptrArray     += 0x30;
            ptrArray += 1;    
         break;
         default:
            fNumber = 333.3f;
         break;
      }

      switch(noDecimals)
      {
        case 1:
          fNumber = (int32_t)(fNum);
          fNumber = (int32_t)((fNum -(float)fNumber) * 10.0f);
          *ptrArray = (uint8_t)fNumber + 0x30;
        break;
        default:
          fNumber = 333.3f;
         break;
      }
      
    }

    //TODO Switch case para manejar digitos y decimales



