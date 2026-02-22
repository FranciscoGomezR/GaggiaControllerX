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

  typedef struct
  {
    float dt;
    float Error;
    float P_Term;
    float I_Term;
    float D_Term;
    float PIDout;
  }PID_InternalModeControl_fStruct;

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
inline float fcn_update_PID_Block(  float fInput, 
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
 * Function:      fcn_update_PIDimc_Block
 * Description:   PID control loop with Internal Model Control (IMC) rules
 *                Type A: Textbook PID
 * Caveats:       https://apmonitor.com/pdc/index.php/Main/ProportionalIntegralDerivative
 * Parameters:
 * Return:
 *****************************************************************************/
inline float fcn_update_PIDimc_typeA( PID_IMC_Block_fStruct * ptr_sPIDparam )
{
    static PID_InternalModeControl_fStruct Block = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
    //	Calculate Delta time for this Iteration in SECONDS
    //	-----------------------------------------------
    Block.dt =  ((float)(ptr_sPIDparam->feedPIDblock.TimeMilis - ptr_sPIDparam->prevT_Milis)) / 1000.0f;

    //	Error = SetPoint - Process Variable
    //	-----------------------------------------------
    Block.Error = ptr_sPIDparam->feedPIDblock.SetPoint - ptr_sPIDparam->feedPIDblock.ProcessVariable;
    //  (PROPORTIONAL) -> Kc * e(t)
    //-------------------------------------------------
    if(ptr_sPIDparam->P_TERM_CTRL)
    {
      //   P --> Kp * Error
      Block.P_Term =  Block.Error * ptr_sPIDparam->Kp;
    }else{}
    //  (INTEGRAL) -> Kc * tI *?e(t)dt
    //-------------------------------------------------
    if(ptr_sPIDparam->I_TERM_CTRL)
    {
        //  Ki * [Previois Error + (Error  * dt) ]
        //-----------------------------------------------------------------------------------------
        ptr_sPIDparam->HistoryError = ptr_sPIDparam->HistoryError + (Block.Error * Block.dt);
        //Integral gain block
        Block.I_Term = ptr_sPIDparam->HistoryError * ptr_sPIDparam->Ki;
        //Integral saturation checkt
        fcn_Constrain_WithinFloats(&ptr_sPIDparam->HistoryError, 
                                ptr_sPIDparam->IntegralLimit,
                               -ptr_sPIDparam->IntegralLimit);
    }else{}  
    //  (DIFFERENTIAL) -> Kc * tD * (e_k - 2*e_k-1 + e_k-2)/dt
    //-------------------------------------------------
    if(ptr_sPIDparam->D_TERM_CTRL)
    {
        //  Delta PV = PV - Previous PV
        //-----------------------------------------------------------------------------------------
        Block.D_Term = (Block.Error - (2.0f*ptr_sPIDparam->errorK_1) + ptr_sPIDparam->errorK_2);
        /* Protect against division by zero  */
        if ( fabs((double)Block.D_Term) > FLOAT_EPSILON )
        {
            Block.D_Term = Block.D_Term / Block.dt;
        }else{
            Block.D_Term = 0.0f;
        }
        //  Kd * Delta PV 
        //-----------------------------------------------------------------------------------------
        Block.D_Term = Block.D_Term * ptr_sPIDparam->Kd;
    }else{}

    //	PID OUTPUT --> P + I + D
    //-----------------------------------------------------------------------------------------
    Block.PIDout = Block.P_Term + Block.I_Term + Block.D_Term;

    //PID ouput saturation & sign check
    //  Flag values::           NO_SATURATION
    //                          POSITIVE_SATURATION
    //                          NEGATIVE_SATURATION
    //  Saturation limited by:: ptr_sPIDparam->OutputLimit
    //-----------------------------------------------------------------------------------------
    ptr_sPIDparam->OutputSaturationOut = fcn_Constrain_WithinFloats((float*)&Block.PIDout, ptr_sPIDparam->OutputLimit, -(ptr_sPIDparam->OutputLimit));

    if(ptr_sPIDparam->I_ANTIWINDUP_CTRL && ptr_sPIDparam->OutputSaturationOut)
    {
      //ANTI-WINDUP Scheme: anti-reset
      //Check if integrator is becoming POSITIVE
      if( ptr_sPIDparam->OutputSaturationOut == POSITIVE_SATURATION || ptr_sPIDparam->OutputSaturationOut == NEGATIVE_SATURATION)
      {
          ptr_sPIDparam->WindupClampStatus = ACTIVE;
          ptr_sPIDparam->IntegralError = ptr_sPIDparam->IntegralError - ptr_sPIDparam->IntegralError * Block.dt;
      }else{
          //Decision is to not to clamp
          ptr_sPIDparam->WindupClampStatus = NOT_ACTIVE;
      } 
    }else{
      //Windup controller is not activated. Therefore, Clamping Status is NOT_ACTIVE by DEFAULT
      ptr_sPIDparam->WindupClampStatus = NOT_ACTIVE;
    }

    //SAVED ERROR DATA, T DATA and PID OUTPUT
    //-----------------------------------------------------------------------------------------
    ptr_sPIDparam->prevT_Milis  = ptr_sPIDparam->feedPIDblock.TimeMilis;
    ptr_sPIDparam->Output       = Block.PIDout;

    return Block.PIDout;
}

/*****************************************************************************
 * Function:      fcn_update_PIDimc_Block
 * Description:   PID control loop with Internal Model Control (IMC) rules
 *                Type B: Remove SP from Derivative
 * Caveats:       https://apmonitor.com/pdc/index.php/Main/ProportionalIntegralDerivative
 * Parameters:
 * Return:
 *****************************************************************************/
inline float fcn_update_PIDimc_typeB( PID_IMC_Block_fStruct * ptr_sPIDparam )
{
    static PID_InternalModeControl_fStruct Block = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
    //	Calculate Delta time for this Iteration in SECONDS
    //	-----------------------------------------------
    Block.dt =  ((float)(ptr_sPIDparam->feedPIDblock.TimeMilis - ptr_sPIDparam->prevT_Milis)) / 1000.0f;

    //	Error = SetPoint - Process Variable
    //	-----------------------------------------------
    Block.Error = ptr_sPIDparam->feedPIDblock.SetPoint - ptr_sPIDparam->feedPIDblock.ProcessVariable;
    //  (PROPORTIONAL) -> Kc * e(t)
    //-------------------------------------------------
    if(ptr_sPIDparam->P_TERM_CTRL)
    {
      //   P --> Kp * Error
      Block.P_Term =  Block.Error * ptr_sPIDparam->Kp;
    }else{}
    //  (INTEGRAL) -> Kc * tI *?e(t)dt
    //-------------------------------------------------
    if(ptr_sPIDparam->I_TERM_CTRL)
    {
        //  Ki * [Previois Error + (Error  * dt) ]
        //-----------------------------------------------------------------------------------------
        ptr_sPIDparam->HistoryError = ptr_sPIDparam->HistoryError + (Block.Error * Block.dt);
        //Integral gain block
        Block.I_Term = ptr_sPIDparam->HistoryError * ptr_sPIDparam->Ki;
        //Integral saturation checkt
        fcn_Constrain_WithinFloats(&ptr_sPIDparam->HistoryError, 
                                ptr_sPIDparam->IntegralLimit,
                               -ptr_sPIDparam->IntegralLimit);
    }else{}  
    //  (DIFFERENTIAL) -> -Kc * tD * d(PV)/dt
    //-------------------------------------------------
    if(ptr_sPIDparam->D_TERM_CTRL)
    {
        //  Delta PV = PV - Previous PV
        //-----------------------------------------------------------------------------------------
        Block.D_Term = ptr_sPIDparam->feedPIDblock.ProcessVariable - ptr_sPIDparam->prevPV;
        /* Protect against division by zero  */
        if ( fabs((double)Block.D_Term) > FLOAT_EPSILON )
        {
            Block.D_Term = Block.D_Term / Block.dt;
        }else{
            Block.D_Term = 0.0f;
        }
        //  Kd * Delta PV 
        //-----------------------------------------------------------------------------------------
        Block.D_Term = Block.D_Term * -ptr_sPIDparam->Kd;
    }else{}

    //	PID OUTPUT --> P + I + D
    //-----------------------------------------------------------------------------------------
    Block.PIDout = Block.P_Term + Block.I_Term + Block.D_Term;
    //Block.PIDout = ptr_sPIDparam->pidOut_t0 + Block.P_Term + Block.I_Term + Block.D_Term;

    //PID ouput saturation & sign check
    //  Flag values::           NO_SATURATION
    //                          POSITIVE_SATURATION
    //                          NEGATIVE_SATURATION
    //  Saturation limited by:: ptr_sPIDparam->OutputLimit
    //-----------------------------------------------------------------------------------------
    ptr_sPIDparam->OutputSaturationOut = fcn_Constrain_WithinFloats((float*)&Block.PIDout, ptr_sPIDparam->OutputLimit, -(ptr_sPIDparam->OutputLimit));

    if(ptr_sPIDparam->I_ANTIWINDUP_CTRL && ptr_sPIDparam->OutputSaturationOut)
    {
      //ANTI-WINDUP Scheme: anti-reset
      //Check if integrator is becoming POSITIVE
      if( ptr_sPIDparam->OutputSaturationOut == POSITIVE_SATURATION || ptr_sPIDparam->OutputSaturationOut == NEGATIVE_SATURATION)
      {
          ptr_sPIDparam->WindupClampStatus = ACTIVE;
          ptr_sPIDparam->IntegralError = ptr_sPIDparam->IntegralError - ptr_sPIDparam->IntegralError * Block.dt;
      }else{
          //Decision is to not to clamp
          ptr_sPIDparam->WindupClampStatus = NOT_ACTIVE;
      } 
    }else{
      //Windup controller is not activated. Therefore, Clamping Status is NOT_ACTIVE by DEFAULT
      ptr_sPIDparam->WindupClampStatus = NOT_ACTIVE;
    }

    //SAVED ERROR DATA, T DATA and PID OUTPUT
    //-----------------------------------------------------------------------------------------
    ptr_sPIDparam->prevT_Milis  = ptr_sPIDparam->feedPIDblock.TimeMilis;
    ptr_sPIDparam->prevPV       = ptr_sPIDparam->feedPIDblock.ProcessVariable;
    ptr_sPIDparam->Output       = Block.PIDout;
    return Block.PIDout;
}


/*****************************************************************************
 * Function:      fcn_update_PIDimc_Block
 * Description:   PID control loop with Internal Model Control (IMC) rules
 * Caveats:       https://apmonitor.com/pdc/index.php/Main/ProportionalIntegralDerivative
 * Parameters:
 * Return:
 *****************************************************************************/
static inline void fcn_PID_Block_Iteration( PID_Block_fStruct * ptr_sPIDparam)
{
        static PID_BlockVariable_fStruct Block = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
        //  Calculate Delta time for this Iteration in SECONDS
        //  -----------------------------------------------
        Block.dt =  ((float)(ptr_sPIDparam->feedPIDblock.TimeMilis - ptr_sPIDparam->prevT_Milis)) / 1000.0f;

        //  Error = SetPoint - Process Variable
        //  -----------------------------------------------
        Block.Error = ptr_sPIDparam->feedPIDblock.SetPoint - ptr_sPIDparam->feedPIDblock.ProcessVariable;
        
        //  (PROPORTIONAL) -> Kc * e(t)
        //-------------------------------------------------     
        if(ptr_sPIDparam->P_TERM_CTRL)
        {
            //   P --> Kp * Error
            Block.P_Term =  Block.Error * ptr_sPIDparam->Kp;
        }else{}
       
        //  (INTEGRAL) -> Kc * tI *?e(t)dt
        //-------------------------------------------------
        if(ptr_sPIDparam->I_TERM_CTRL)
        {
            //  Ki * [Previois Error + (Error  * dt) ]
            //-----------------------------------------------------------------------------------------
            ptr_sPIDparam->HistoryError += Block.Error * Block.dt;
            //Integral gain block
            Block.I_Term = ptr_sPIDparam->HistoryError * ptr_sPIDparam->Ki;
            //Integral saturation checkt
            fcn_Constrain_WithinFloats(&Block.I_Term, 
                                    ptr_sPIDparam->IntegralLimit,
                                   -ptr_sPIDparam->IntegralLimit);
        }else{}
        
        //  (DIFFERENTIAL) -> Kc * tD * d(PV)/dt
        //-------------------------------------------------
        if(ptr_sPIDparam->D_TERM_CTRL)
        {
                //  D --> Kd * (Delta Error / dt)
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



