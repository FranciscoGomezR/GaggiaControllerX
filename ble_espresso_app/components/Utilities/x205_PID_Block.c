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
    #define FLOAT_EPSILON   0.000001
//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
  typedef struct
  {
      float dt;
      float Error;
      float P_Term;
      float I_Term;
      float D_Term;
      float PIDraw;
      float PIDclamped;
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
static inline void fcn_PID_Block_Iteration( PID_Block_fStruct * ptr_sPIDparam);
//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function:    fcn_update_PID_Block
 * Description: 
 *****************************************************************************/
inline float fcn_update_PID_Block( float fInput, 
                                  float fSetpoint, 
                                  uint32_t timeMilis,  
                                  PID_Block_fStruct * ptr_sPIDparam )
{
    ptr_sPIDparam->feedPIDblock.ProcessVariable = fInput;
    ptr_sPIDparam->feedPIDblock.SetPoint = fSetpoint;
    ptr_sPIDparam->feedPIDblock.TimeMilis = timeMilis;
    fcn_PID_Block_Iteration( ptr_sPIDparam );
    return ptr_sPIDparam->Output;
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
        lpf_rc_calculate_const(&ptr_sPIDparam->sLPF_Param, ptr_sPIDparam->LPF_FCUTOFF_HZ);
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
static inline void fcn_PID_Block_Iteration( PID_Block_fStruct * ptr_sPIDparam)
{
        static PID_BlockVariable_fStruct Block = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
        //
        //	Calculate Delta time for this Iteration in SECONDS
        //	-----------------------------------------------
        //	
        Block.dt =  ((float)(ptr_sPIDparam->feedPIDblock.TimeMilis - ptr_sPIDparam->prevT_Milis)) / 1000.0f;

        //
        //	System Error Stage --> PID loop
        //	-----------------------------------------------
        //	Error = SetPoint - Process Variablec
        Block.Error = ptr_sPIDparam->feedPIDblock.SetPoint - ptr_sPIDparam->feedPIDblock.ProcessVariable;
        

        
        if(ptr_sPIDparam->P_TERM_CTRL)
        {
              //-----------------------------------------------------------------------------------------
              //  (PROPORTIONAL)	P-STAGE --> PID loop
              //-----------------------------------------------
              //   P --> Kp * Error
              //
              Block.P_Term =  Block.Error * ptr_sPIDparam->Kp;
        }else{}
       
        if(ptr_sPIDparam->I_TERM_CTRL)
        {
          if(ptr_sPIDparam->I_ANTIWINDUP_CTRL && ptr_sPIDparam->OutputSaturationOut)
          {
            //ANTI-WINDUP
            //-----------------------------------------------------------------------------------------
            //This link explain: clamping method for anti-windup
            //Reference Link: https://www.mathworks.com/videos/understanding-pid-control-part-2-expanding-beyond-a-simple-integral-1528310418260.html
            //for other scheme like: Back Calculation Go To:
            //Reference link: https://www.scilab.org/pid-anti-windup-schemes
            //-----------------------------------------------------------------------------------------
            //This Scheme is: Clamping
            //Check if integrator is becoming POSITIVE and the controller is already saturated on the POSITIVE side
            if( ptr_sPIDparam->HistoryError >= 0.0f && ptr_sPIDparam->OutputSaturationOut == POSITIVE_SATURATION)
            {
                //Integrator is trying to become more postive even thouhg the PID is saturated
                //Decision is to clamp
                ptr_sPIDparam->WindupClampStatus = ACTIVE;
            }else{
            //Check if integrator is becoming NEGATIVE and the controller is already saturated on the NEGATIVE side
              if( ptr_sPIDparam->HistoryError < 0.0f && ptr_sPIDparam->OutputSaturationOut == NEGATIVE_SATURATION)
              {
                //Integrator is trying to become more negative even thouhg the PID is saturated
                //Decision is to clamp
                ptr_sPIDparam->WindupClampStatus = ACTIVE;
              }else{
                //Decision is to not to clamp
                ptr_sPIDparam->WindupClampStatus = NOT_ACTIVE;
              } 
            }
          }else{
            //Windup controller is not activated. Therefore, Clamping Status is NOT_ACTIVE by DEFAULT
            ptr_sPIDparam->WindupClampStatus = NOT_ACTIVE;
          }

          if(ptr_sPIDparam->WindupClampStatus == NOT_ACTIVE) 
          {
            //-----------------------------------------------------------------------------------------
            //	(INTEGRAL)	I-STAGE --> PID loop
            //	-----------------------------------------------
            //  I --> Ki * [HistoryError] <- +(DeltaError * dt)
            //Integral section
            ptr_sPIDparam->HistoryError += Block.Error * Block.dt;
            //Integral gain block
            Block.I_Term = ptr_sPIDparam->HistoryError * ptr_sPIDparam->Ki;
            //Integral saturation checkt
            fcn_Constrain_WithinFloats(&Block.I_Term, 
                                    ptr_sPIDparam->IntegralLimit,
                                   -ptr_sPIDparam->IntegralLimit);

          }else{}
        }else{}
        
        if(ptr_sPIDparam->D_TERM_CTRL)
        {
                //-----------------------------------------------------------------------------------------
                //
                //	(DIFFERENTIAL)	D-STAGE --> PID loop
                //	-----------------------------------------------
                //		Delta Error = Error - Previous Error
                //		D --> Kd * (Delta Error / dt)
                //
                Block.D_Term = Block.Error - ptr_sPIDparam->PrevError;

                /* Protect against division by zero  */
                if ( fabs((double)Block.D_Term) < FLOAT_EPSILON )
                {
                    Block.D_Term = Block.D_Term / Block.dt;
                }else{
                    Block.D_Term = 0.0f;
                }
                
                /*  filter D parameter using first-order RC filter  */
                if(ptr_sPIDparam->D_TERM_LP_FILTER_CTRL)
                {
                    Block.D_Term = lpf_rc_update(&ptr_sPIDparam->sLPF_Param,Block.D_Term,Block.dt);
                }else{}
                Block.D_Term = Block.D_Term * ptr_sPIDparam->Kd;
        }else{}
        //-----------------------------------------------------------------------------------------
        //
        //	(PID Controller)	PID --> P + I + D
        //	-----------------------------------------------
        //		PIDraw = P_Term + I_Term + D_Term;
        //
        Block.PIDraw = Block.P_Term + Block.I_Term + Block.D_Term;

        //PID ouput saturation & sign check
        //  Flag values::           NO_SATURATION
        //                          POSITIVE_SATURATION
        //                          NEGATIVE_SATURATION
        //  Saturation limited by:: ptr_sPIDparam->OutputLimit
        //-----------------------------------------------------------------------------------------
        ptr_sPIDparam->OutputSaturationOut = fcn_Constrain_WithinFloats((float*)&Block.PIDraw, ptr_sPIDparam->OutputLimit, -(ptr_sPIDparam->OutputLimit));
  
        //SAVED ERROR DATA, T DATA and PID OUTPUT
        //-----------------------------------------------------------------------------------------
        ptr_sPIDparam->PrevError = Block.Error;
        ptr_sPIDparam->prevT_Milis = ptr_sPIDparam->feedPIDblock.TimeMilis;
        ptr_sPIDparam->Output = Block.PIDraw;
}
