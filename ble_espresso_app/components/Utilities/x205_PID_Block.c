/*
 * x205_PID_Block.c
 *
 *  Created on: Nov 7, 2017
 *      Author: Juan Fco. Gómez
 */

//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
	#include "x205_PID_Block.h"
	#include "x04_Numbers.h"
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
	typedef struct
	{
		float Error;
		float P_Term;
		float I_Term;
		float D_Term;
		float PID;
                float PIDraw;
	}PID_BlockVariable_fStruct;

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
 * Function:
 * Description:
 * Caveats:		inline fcn
 * Parameters:
 * Return:
 *****************************************************************************/
	inline void fcn_PID_Block_Iteration( PID_Block_fStruct * ptr_sPIDparam)
	{
		static PID_BlockVariable_fStruct Block = {0.0f,0.0f,0.0f,0.0f,0.0f};
		//
		//	System Error Stage --> PID loop
		//	-----------------------------------------------
		//		Error = SetPoint - Process Value / Actual
		Block.Error = ptr_sPIDparam->SetPoint - ptr_sPIDparam->Input;
		//-----------------------------------------------------------------------------------------
		if(ptr_sPIDparam->P_TERM_CTRL)
		{
			//	(PROPORTIONAL)	P-STAGE --> PID loop
			//	-----------------------------------------------
			//   	P --> Kp * Error
		    //
			Block.P_Term =  Block.Error * ptr_sPIDparam->Kp;
		}else{}
		//-----------------------------------------------------------------------------------------
		if(ptr_sPIDparam->I_TERM_CTRL)
		{

                      //AINT-WINDUp
                      //Reference Link: https://www.mathworks.com/videos/understanding-pid-control-part-2-expanding-beyond-a-simple-integral-1528310418260.html
                      // 	(INTEGRAL)		I-STAGE --> PID loop
                        ptr_sPIDparam->HistoryError += Block.Error * ptr_sPIDparam->dt;
                        if(ptr_sPIDparam->WindupStatus && ptr_sPIDparam->I_ANTIWINDUP_CTRL)
                        {
                            //ptr_sPIDparam->HistoryError -= ptr_sPIDparam->WindupError;
                            ptr_sPIDparam->HistoryError = 0.0f;
                        }else{}

                        fcn_Constrain_WithinFloats(&ptr_sPIDparam->HistoryError, ptr_sPIDparam->HistoryErrorLimit, -ptr_sPIDparam->HistoryErrorLimit);
			Block.I_Term = ptr_sPIDparam->HistoryError * ptr_sPIDparam->Ki;
		}else{}
		//-----------------------------------------------------------------------------------------
		if(ptr_sPIDparam->D_TERM_CTRL)
		{
			//
			//	(DIFFERENTIAL)	D-STAGE --> PID loop
			//	-----------------------------------------------
			//		Delta Error = Error - Previous Error
			//		D --> Kd * (Delta Error / dt)
			//
			Block.D_Term = Block.Error -	ptr_sPIDparam->PreviousError;
			Block.D_Term = Block.D_Term / ptr_sPIDparam->dt;

			if(ptr_sPIDparam->D_TERM_LP_FILTER_CTRL)
			{
			    pfcn_RCFilterAlgorithm(&ptr_sPIDparam->sLPF_Param,Block.D_Term);
			    Block.D_Term = ptr_sPIDparam->sLPF_Param.DataOut_n;
				//pfcn_FiniteImpulseResponseFilterAlgorithm((float*)&Block.D_Term, (float*)&Block.D_Term, &ptr_sPIDparam->sLowPassFilterParam);
			}else{}
			Block.D_Term = Block.D_Term * ptr_sPIDparam->Kd;
		}else{}
		//-----------------------------------------------------------------------------------------
		//
		//	(PID Controller)	PID --> P + I + D
		//	-----------------------------------------------
		//		PIDraw = P_Term + I_Term + D_Term;
		//		PIDraw = Kpid * PID
		//
		Block.PIDraw = Block.P_Term + Block.I_Term + Block.D_Term;
           
		//-----------------------------------------------------------------------------------------
		if(ptr_sPIDparam->PID_OUTPUT_GAIN_CTRL)
		{
                    //  (PID GAIN)  PID loop general Gain.
                    //  -----------------------------------------------
                    //      [PID Block] x Gain = Output
                    //
                    Block.PID = Block.PIDraw * ptr_sPIDparam->Kpid;
		}else{
                    Block.PID = Block.PIDraw;
                }
		//-----------------------------------------------------------------------------------------
		int8_t SaturationSign = fcn_Constrain_WithinFloats((float*)&Block.PID, ptr_sPIDparam->OutputLimit, -(ptr_sPIDparam->OutputLimit));
                if(SaturationSign != NO_SATURATION)
                {
                    ptr_sPIDparam->WindupStatus = ACTIVE;
                    ptr_sPIDparam->WindupError = Block.PIDraw - Block.PID; 
                    ptr_sPIDparam->WindupError *= ptr_sPIDparam->Kwindup;
                }else{
                  ptr_sPIDparam->WindupStatus = NOT_ACTIVE;
                }
		ptr_sPIDparam->PreviousError=Block.Error;
		ptr_sPIDparam->Output=Block.PID;
	}

/*****************************************************************************
 * Function:
 * Description:
 * Caveats:     inline fcn
 * Parameters:
 * Return:
 *****************************************************************************/
	inline void fcn_PID_Block_ResetI( PID_Block_fStruct * ptr_sPIDparam, float Attenuator)
	{
		ptr_sPIDparam->HistoryError=ptr_sPIDparam->HistoryError * Attenuator;
	}

/*****************************************************************************
 * Function:
 * Description:
 * Caveats:     inline fcn
 * Parameters:
 * Return:
 *****************************************************************************/
    void fcn_PID_Block_Dterm_LPF_Init( PID_Block_fStruct* ptr_sPIDparam)
    {
        if( ptr_sPIDparam->D_TERM_LP_FILTER_CTRL == ACTIVE)
        {
            pfcn_InitRCFilterAlgorithm(&ptr_sPIDparam->sLPF_Param,ptr_sPIDparam->LPF_FCUTOFF_HZ,ptr_sPIDparam->LPF_SAMPLING_S);
        }else{}
    }

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function:
 * Description:
 * Caveats:		inline fcn
 * Parameters:
 * Return:
 *****************************************************************************/
