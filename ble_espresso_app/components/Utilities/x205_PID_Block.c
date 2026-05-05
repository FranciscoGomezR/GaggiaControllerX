/*
 * x205_PID_Block.c
 *
 *  Created on: Nov 7, 2017
 *      Author: Juan Fco. G�mez
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
    float PIDout;
  }pid_imc_ctrl_terms_t;

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
float pid_imc_compute( pid_imc_block_t * ptr_pid_param_s )
{
    static pid_imc_ctrl_terms_t Pid_ctrl_s = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
    //	Calculate Delta time for this Iteration in SECONDS
    //	-----------------------------------------------
    Pid_ctrl_s.dt =  ((float)(ptr_pid_param_s->feedPIDblock.TimeMilis - ptr_pid_param_s->prevT_Milis)) / 1000.0f;

    //	Error = SetPoint - Process Variable
    //	-----------------------------------------------
    Pid_ctrl_s.Error = ptr_pid_param_s->feedPIDblock.SetPoint - ptr_pid_param_s->feedPIDblock.ProcessVariable;
    //  (PROPORTIONAL) -> Kc * e(t)
    //-------------------------------------------------
    if(ptr_pid_param_s->P_TERM_CTRL)
    {
      //   P --> Kp * Error
      Pid_ctrl_s.P_Term =  Pid_ctrl_s.Error * ptr_pid_param_s->Kp;
    }else{}
    //  (INTEGRAL) -> Kc * tI *?e(t)dt
    //-------------------------------------------------
    if(ptr_pid_param_s->I_TERM_CTRL)
    {
        //  Ki * [Previois Error + (Error  * dt) ]
        //-----------------------------------------------------------------------------------------
        ptr_pid_param_s->HistoryError = ptr_pid_param_s->HistoryError + (Pid_ctrl_s.Error * Pid_ctrl_s.dt);
        //Integral gain block
        Pid_ctrl_s.I_Term = ptr_pid_param_s->HistoryError * ptr_pid_param_s->Ki;
        //Integral saturation checkt
        fcn_Constrain_WithinFloats(&ptr_pid_param_s->HistoryError, 
                                ptr_pid_param_s->IntegralLimit,
                               -ptr_pid_param_s->IntegralLimit);
    }else{}  
    //  (DIFFERENTIAL) -> Kc * tD * (e_k - 2*e_k-1 + e_k-2)/dt
    //-------------------------------------------------
    if(ptr_pid_param_s->D_TERM_CTRL)
    {
        //  Delta PV = PV - Previous PV
        //-----------------------------------------------------------------------------------------
        Pid_ctrl_s.D_Term = (Pid_ctrl_s.Error - (2.0f*ptr_pid_param_s->errorK_1) + ptr_pid_param_s->errorK_2);
        /* Protect against division by zero  */
        if ( fabs((double)Pid_ctrl_s.D_Term) > FLOAT_EPSILON )
        {
            Pid_ctrl_s.D_Term = Pid_ctrl_s.D_Term / Pid_ctrl_s.dt;
        }else{
            Pid_ctrl_s.D_Term = 0.0f;
        }
        //  Kd * Delta PV 
        //-----------------------------------------------------------------------------------------
        Pid_ctrl_s.D_Term = Pid_ctrl_s.D_Term * ptr_pid_param_s->Kd;
    }else{}

    //	PID OUTPUT --> P + I + D
    //-----------------------------------------------------------------------------------------
    Pid_ctrl_s.PIDout = Pid_ctrl_s.P_Term + Pid_ctrl_s.I_Term + Pid_ctrl_s.D_Term;

    //PID ouput saturation & sign check
    //  Flag values::           NO_SATURATION
    //                          POSITIVE_SATURATION
    //                          NEGATIVE_SATURATION
    //  Saturation limited by:: ptr_pid_param_s->OutputLimit
    //-----------------------------------------------------------------------------------------
    ptr_pid_param_s->OutputSaturationOut = fcn_Constrain_WithinFloats((float*)&Pid_ctrl_s.PIDout, ptr_pid_param_s->OutputLimit, -(ptr_pid_param_s->OutputLimit));

    if(ptr_pid_param_s->I_ANTIWINDUP_CTRL && ptr_pid_param_s->OutputSaturationOut)
    {
      //ANTI-WINDUP Scheme: anti-reset
      //Check if integrator is becoming POSITIVE
      if( ptr_pid_param_s->OutputSaturationOut == POSITIVE_SATURATION || ptr_pid_param_s->OutputSaturationOut == NEGATIVE_SATURATION)
      {
          ptr_pid_param_s->WindupClampStatus = ACTIVE;
          ptr_pid_param_s->IntegralError = ptr_pid_param_s->IntegralError - ptr_pid_param_s->IntegralError * Pid_ctrl_s.dt;
      }else{
          //Decision is to not to clamp
          ptr_pid_param_s->WindupClampStatus = NOT_ACTIVE;
      } 
    }else{
      //Windup controller is not activated. Therefore, Clamping Status is NOT_ACTIVE by DEFAULT
      ptr_pid_param_s->WindupClampStatus = NOT_ACTIVE;
    }

    //SAVED ERROR DATA, T DATA and PID OUTPUT
    //-----------------------------------------------------------------------------------------
    ptr_pid_param_s->prevT_Milis  = ptr_pid_param_s->feedPIDblock.TimeMilis;
    ptr_pid_param_s->Output       = Pid_ctrl_s.PIDout;

    return Pid_ctrl_s.PIDout;
}





