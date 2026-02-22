
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "PumpController.h"
#include "solidStateRelay_Controller.h"
#include "app_config.h"


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
enum{
  s0_Idle = 0,
  s00_CloseSolenoid,
  s01_RampupTo1stP,
  s12_Keep1stP,
  s23_RampupTo2ndP,
  s34_Keep2ndP,
  s45_RampdownTo3rdP,
  s56_Keep3rdP,
  s67_RampdownToStop,
  st_Timer,
  st_Stop
};

typedef struct
{
  uint16_t Pwr_1stP;
  uint16_t Pwr_2ndP;
  uint16_t Pwr_3rdP;
  uint16_t Time_1stP_ms;
  uint16_t Time_2ndP_ms;
  uint16_t Time_3rdP_ms;
  uint16_t Time_Rampup_ms;
  uint16_t Time_Rampdown_ms;
}Struct_PumpProfileData;

typedef struct
{
  StateMachineCtrl_Struct smPump;
  uint16_t PumpPower;
  uint16_t RampUp_tCounts;
  uint16_t RampDown_tCounts;
  uint16_t tCounts_1stP;
  uint16_t tCounts_2ndP;
  uint16_t tCounts_3rdP;
  int16_t  SlopeTo_1stP;
  int16_t  SlopeTo_2ndP;
  int16_t  SlopeTo_3rdP;
  int16_t  SlopeTo_Stop;
  uint16_t Counter;
  int16_t  Slope;
}Struct_PumpParam;

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
/* This variable contains timer's Counts, power slopes and power values for the pump  */
/*  Note: this values are obtain from the sPumpProfData var. a copy from bleSpressoUserdata_struct */
Struct_PumpParam sPumpParam;

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
void fcn_LoadPumpParam(Struct_PumpProfileData *prt_ProfileParam, Struct_PumpParam *prt_PumpParam);

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	fcn_initPumpController
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
pumpCtrl_status_t fcn_initPumpController(void)
{
  /* Load default Pump PARAMETER  */ 
  /* in case the NVM does not contain user data */
  sPumpParam.PumpPower        = 0;
  sPumpParam.smPump.sRunning  = s0_Idle;

  Struct_PumpProfileData sPumpProfData;
  sPumpProfData.Pwr_2ndP         = (uint16_t)PUMP_BREWPWR_DEFAULT;
  sPumpProfData.Pwr_1stP         = (uint16_t)PUMP_PREINFUSSIONPWR_DEFAULT;
  sPumpProfData.Pwr_3rdP         = (uint16_t)PUMP_DECLINING_PWR_DEFAULT;
  sPumpProfData.Time_1stP_ms     = (uint16_t)PUMP_PREINFUSSION_T_MS_DEFAULT;
  sPumpProfData.Time_2ndP_ms     = (uint16_t)PUMP_PEAKPRESSURE_T_MS_DEFAULT;
  sPumpProfData.Time_3rdP_ms     = (uint16_t)PUMP_DECLINING_T_MS_DEFAULT;
  sPumpProfData.Time_Rampup_ms   = (uint16_t)PUMP_RAMPUP_T_MS_DEFAULT;
  sPumpProfData.Time_Rampdown_ms = (uint16_t)PUMP_RAMPDOWN_T_MS_DEFAULT;
  /* Load Pump paramenter based on Default Pump Controller values */
  fcn_LoadPumpParam(&sPumpProfData,&sPumpParam); 
 

  return  PUMPCTRL_INIT_OK;
}

/*****************************************************************************
* Function: 	fcn_LoadNewPumpParameters
* Description: loads new parameter into the controller
*              This fcn has to be called once BLE receives new data from the user
* Parameters:	
* Return:      pumpCtrl_status_t
*****************************************************************************/
pumpCtrl_status_t fcn_LoadNewPumpParameters(bleSpressoUserdata_struct *prt_profData)
{
  /* Load Pump paramenter based on New Pump Controller values */
  Struct_PumpProfileData sPumpProfData;
  sPumpProfData.Pwr_1stP         = (uint16_t)prt_profData->prof_preInfusePwr;
  sPumpProfData.Pwr_2ndP         = (uint16_t)prt_profData->prof_InfusePwr;
  sPumpProfData.Pwr_3rdP         = (uint16_t)prt_profData->Prof_DeclinePwr;
  sPumpProfData.Time_1stP_ms     = ((uint16_t)prt_profData->prof_preInfuseTmr)*1000;
  sPumpProfData.Time_2ndP_ms     = ((uint16_t)prt_profData->prof_InfuseTmr)*1000;
  sPumpProfData.Time_3rdP_ms     = ((uint16_t)prt_profData->Prof_DeclineTmr)*1000;
  sPumpProfData.Time_Rampup_ms   = (uint16_t)PUMP_RAMPUP_T_MS_DEFAULT;
  sPumpProfData.Time_Rampdown_ms = (uint16_t)PUMP_RAMPDOWN_T_MS_DEFAULT;

  /* Load NEW Pump paramenter into: sPumpParam  */
  fcn_LoadPumpParam(&sPumpProfData,&sPumpParam); 
  return PUMPCTRL_LOAD_OK;
}

/*****************************************************************************
* Function: 	fcn_PumpStateDriver
* Description: 
* Caveats:     State timing: 100ms
* Parameters:	
* Return:      pumpCtrl_status_t
*****************************************************************************/
pumpCtrl_status_t fcn_PumpStateDriver(void)
{
  int pumpStatus;
  int solenoidStatus;
  switch(sPumpParam.smPump.sRunning)
  {
      case s0_Idle:
        pumpStatus = PUMPCTRL_IDLE;
      break;

      case s00_CloseSolenoid:
        //Close Pump's valve by activating solenoid
        fcn_SolenoidSSR_On();
        // Now, we need to wait until is engage to move to next state 
        solenoidStatus = get_SolenoidSSR_State();
        if( solenoidStatus == SSR_STATE_ENGAGE)
        {
          sPumpParam.smPump.sRunning  = s01_RampupTo1stP;
        }else{}
      break;

      /* Ramp Up Pwr from: Step 0 to 1  */
      case s01_RampupTo1stP:
        //Ramp-up pressure to pre-infussion power level
        sPumpParam.smPump.sRunning  = st_Timer;
        sPumpParam.smPump.sNext = s12_Keep1stP;
        sPumpParam.Counter = sPumpParam.RampUp_tCounts+1;
        sPumpParam.Slope   = sPumpParam.SlopeTo_1stP;
        //Send driver status
        pumpStatus = PUMPCTRL_STEP_1ST;
      break;
      /* Keep Pwr from: Step 1 to 2  */
      case s12_Keep1stP:
        sPumpParam.smPump.sRunning  = st_Timer;
        sPumpParam.smPump.sNext = s23_RampupTo2ndP;
        sPumpParam.Counter = sPumpParam.tCounts_1stP+1;
        //Pressure in pump remains intact -> Slope = 0
        sPumpParam.Slope   = 0;
        //Send driver status
        pumpStatus = PUMPCTRL_STEP_1ST;
      break;
      /* Ramp Up Pwr from: Step 2 to 3  */
      case s23_RampupTo2ndP:
        sPumpParam.smPump.sRunning  = st_Timer;
        sPumpParam.smPump.sNext = s34_Keep2ndP;
        sPumpParam.Counter = sPumpParam.RampUp_tCounts+1;
        sPumpParam.Slope   = sPumpParam.SlopeTo_2ndP;
        //Send driver status
        pumpStatus = PUMPCTRL_STEP_2ND;
      break;
      /* Keep Pwr from: Step 3 to 4  */
      case s34_Keep2ndP:
        //Load timer to mantain th maximum pressure power for a certain time
        sPumpParam.smPump.sRunning  = st_Timer;
        sPumpParam.smPump.sNext = s45_RampdownTo3rdP;
        sPumpParam.Counter = sPumpParam.tCounts_2ndP;
        //Pressure in pump remains intact -> Slope = 0
        sPumpParam.Slope   = 0;     
        //Send driver status 
        pumpStatus = PUMPCTRL_STEP_2ND;
      break;
      /* Ramp Down Pwr from: Steo 4 to 5  */
      case s45_RampdownTo3rdP:
        sPumpParam.smPump.sRunning  = st_Timer;
        sPumpParam.smPump.sNext = s56_Keep3rdP;
        sPumpParam.Counter = sPumpParam.RampDown_tCounts+1;
        sPumpParam.Slope   = sPumpParam.SlopeTo_3rdP;
        //Send driver status
        pumpStatus = PUMPCTRL_STEP_3RD;
      break;
      /* Keep Pwr from: Step 5 to 6  */
      case s56_Keep3rdP:
        sPumpParam.smPump.sRunning  = st_Timer;
        sPumpParam.smPump.sNext = s67_RampdownToStop;
        sPumpParam.Counter = sPumpParam.tCounts_3rdP+1;
        //Pressure in pump remains intact -> Slope = 0
        sPumpParam.Slope   = 0;
        //Send driver status
        pumpStatus = PUMPCTRL_STEP_3RD;
      break;
      /* Ramp Down Pwr from: Steo 6 to 7  */
      case s67_RampdownToStop:
        sPumpParam.smPump.sRunning  = st_Timer;
        sPumpParam.smPump.sNext = st_Stop;
        sPumpParam.Counter = sPumpParam.RampDown_tCounts+1;
        sPumpParam.Slope   = sPumpParam.SlopeTo_Stop;
        //Send driver status
        pumpStatus = PUMPCTRL_STEP_STOP;
      break;

      case st_Timer:
        if(-- sPumpParam.Counter != 0 )
        {

          if( sPumpParam.Slope != 0)
          {
            volatile int16_t PumpPwr = (int16_t)sPumpParam.PumpPower;
            PumpPwr += (int16_t)sPumpParam.Slope;
            sPumpParam.PumpPower = PumpPwr;
            //fcn_SSR_pwrUpdate((struct_SSRinstance *)&sPumpSSRdrv, sPumpParam.PumpPower);
            fcn_pumpSSR_pwrUpdate(sPumpParam.PumpPower);
          }else{}
          
        }else{
          sPumpParam.smPump.sRunning = sPumpParam.smPump.sNext;
        }
        //Send driver status
        pumpStatus = PUMPCTRL_STEP_TIME;
      break;

      case st_Stop:
        //Open Pump's valve by deactivating selenoid
        fcn_SolenoidSSR_Off();
        sPumpParam.PumpPower = 0;
        fcn_pumpSSR_pwrUpdate(sPumpParam.PumpPower);
        sPumpParam.smPump.sRunning  = s0_Idle;
        //Send driver status
        pumpStatus = PUMPCTRL_STEP_STOP;
      break;

      default:
        //smPumpCtrl.smPump.sRunning  = s0_Idle;
        //Send driver status
        pumpStatus = PUMPCTRL_ERROR;
      break;
  }
  return pumpStatus;
}

/*****************************************************************************
 * Function: 	InitClocks
 * Description: 
 *****************************************************************************/
void fcn_StartBrew(void)
{
    sPumpParam.smPump.sRunning = s00_CloseSolenoid;
}

/*****************************************************************************
* Function: 	InitClocks
* Description: 
*****************************************************************************/
void fcn_CancelBrew(void)
{
   sPumpParam.smPump.sRunning = s67_RampdownToStop;
}

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	fcn_LoadPumpParam
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_LoadPumpParam(Struct_PumpProfileData *prt_ProfileParam, Struct_PumpParam *prt_PumpParam)
{
  /* STEPS TIME DURATION - TIMER COUNT's CALCULATION */
  /* Counts (time) from step 0 to 1 */
  /* Counts (time) from step 2 to 3 */
  prt_PumpParam->RampUp_tCounts   = (uint16_t)(prt_ProfileParam->Time_Rampup_ms / PUMP_BASETIME_T_MS);
  /* Counts (time) from step 4 to 5 */
  /* Counts (time) from step 6 to 7 */
  prt_PumpParam->RampDown_tCounts = (uint16_t)(prt_ProfileParam->Time_Rampdown_ms / PUMP_BASETIME_T_MS);
  /* Counts (time) from step 1 to 2 */
  prt_PumpParam->tCounts_1stP     = (uint16_t)(prt_ProfileParam->Time_1stP_ms / PUMP_BASETIME_T_MS);
  /* Counts (time) from step 3 to 4 */
  prt_PumpParam->tCounts_2ndP     = (uint16_t)(prt_ProfileParam->Time_2ndP_ms / PUMP_BASETIME_T_MS);
  /* Counts (time) from step 5 to 6 */
  prt_PumpParam->tCounts_3rdP     = (uint16_t)(prt_ProfileParam->Time_3rdP_ms / PUMP_BASETIME_T_MS);
  
  /*SLOPES BETWEEN STEPS CALCULATION */
  /* Slope calc from Step 0 to 1  */
  prt_PumpParam->SlopeTo_1stP     = (int16_t)     prt_ProfileParam->Pwr_1stP / prt_PumpParam->RampUp_tCounts;
  /* Slope calc from Step 2 to 3  */
  prt_PumpParam->SlopeTo_2ndP     = (int16_t)  (( prt_ProfileParam->Pwr_2ndP - prt_ProfileParam->Pwr_1stP) / prt_PumpParam->RampUp_tCounts)+1; 
  /* Slope calc from Step 4 to 5  */
  prt_PumpParam->SlopeTo_3rdP     = (int16_t) -(((prt_ProfileParam->Pwr_2ndP - prt_ProfileParam->Pwr_3rdP) / prt_PumpParam->tCounts_3rdP)+1);
  /* Slope calc from Step 6 to 7  */
  prt_PumpParam->SlopeTo_Stop     = (int16_t) -(( prt_ProfileParam->Pwr_3rdP / prt_PumpParam->RampDown_tCounts)+2); 
}

