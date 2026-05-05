
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "pumpController.h"
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
typedef enum {
  S0_IDLE = 0,
  S00_CLOSE_SOLENOID,
  S01_RAMPUP_TO_1ST,
  S12_KEEP_1ST,
  S23_RAMPUP_TO_2ND,
  S34_KEEP_2ND,
  S45_RAMPDOWN_TO_3RD,
  S56_KEEP_3RD,
  S67_RAMPDOWN_TO_STOP,
  ST_TIMER,
  ST_STOP
} pump_state_t;

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
}pump_profile_data_t;

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
}pump_param_t;

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
/* This variable contains timer's Counts, power slopes and power values for the pump  */
/*  Note: this values are obtain from the sPumpProfData var. a copy from espresso_user_config_t */
pump_param_t g_Pump_param_s;

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
static void load_pump_param(pump_profile_data_t *ptr_profile_param, pump_param_t *ptr_pump_param);

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
  g_Pump_param_s.PumpPower        = 0;
  g_Pump_param_s.smPump.sRunning  = S0_IDLE;

  pump_profile_data_t sPumpProfData;
  sPumpProfData.Pwr_2ndP         = (uint16_t)PUMP_BREWPWR_DEFAULT;
  sPumpProfData.Pwr_1stP         = (uint16_t)PUMP_PREINFUSSIONPWR_DEFAULT;
  sPumpProfData.Pwr_3rdP         = (uint16_t)PUMP_DECLINING_PWR_DEFAULT;
  sPumpProfData.Time_1stP_ms     = (uint16_t)PUMP_PREINFUSSION_T_MS_DEFAULT;
  sPumpProfData.Time_2ndP_ms     = (uint16_t)PUMP_PEAKPRESSURE_T_MS_DEFAULT;
  sPumpProfData.Time_3rdP_ms     = (uint16_t)PUMP_DECLINING_T_MS_DEFAULT;
  sPumpProfData.Time_Rampup_ms   = (uint16_t)PUMP_RAMPUP_T_MS_DEFAULT;
  sPumpProfData.Time_Rampdown_ms = (uint16_t)PUMP_RAMPDOWN_T_MS_DEFAULT;
  /* Load Pump paramenter based on Default Pump Controller values */
  load_pump_param(&sPumpProfData,&g_Pump_param_s); 
 

  return  PUMPCTRL_INIT_OK;
}

/*****************************************************************************
* Function: 	load_new_pump_parameters
* Description: loads new parameter into the controller
*              This fcn has to be called once BLE receives new data from the user
* Parameters:	
* Return:      pumpCtrl_status_t
*****************************************************************************/
pumpCtrl_status_t load_new_pump_parameters(espresso_user_config_t *ptr_prof_data)
{
  /* Load Pump paramenter based on New Pump Controller values */
  pump_profile_data_t sPumpProfData;
  sPumpProfData.Pwr_1stP         = (uint16_t)ptr_prof_data->profPreInfusePwr;
  sPumpProfData.Pwr_2ndP         = (uint16_t)ptr_prof_data->profInfusePwr;
  sPumpProfData.Pwr_3rdP         = (uint16_t)ptr_prof_data->profTaperingPwr;
  sPumpProfData.Time_1stP_ms     = ((uint16_t)ptr_prof_data->profPreInfuseTmr)*1000;
  sPumpProfData.Time_2ndP_ms     = ((uint16_t)ptr_prof_data->profInfuseTmr)*1000;
  sPumpProfData.Time_3rdP_ms     = ((uint16_t)ptr_prof_data->profTaperingTmr)*1000;
  sPumpProfData.Time_Rampup_ms   = (uint16_t)PUMP_RAMPUP_T_MS_DEFAULT;
  sPumpProfData.Time_Rampdown_ms = (uint16_t)PUMP_RAMPDOWN_T_MS_DEFAULT;

  /* Load NEW Pump paramenter into: g_Pump_param_s  */
  load_pump_param(&sPumpProfData,&g_Pump_param_s); 
  return PUMPCTRL_LOAD_OK;
}

/*****************************************************************************
* Function: 	pump_state_driver
* Description: 
* Caveats:     State timing: 100ms
* Parameters:	
* Return:      pumpCtrl_status_t
*****************************************************************************/
pumpCtrl_status_t pump_state_driver(void)
{
  int pump_status;
  int solenoid_status;
  switch(g_Pump_param_s.smPump.sRunning)
  {
      case S0_IDLE:
        pump_status = PUMPCTRL_IDLE;
      break;

      case S00_CLOSE_SOLENOID:
        //Close Pump's valve by activating solenoid
        fcn_SolenoidSSR_On();
        // Now, we need to wait until is engage to move to next state 
        solenoid_status = get_SolenoidSSR_State();
        if( solenoid_status == SSR_STATE_ENGAGE)
        {
          g_Pump_param_s.smPump.sRunning  = S01_RAMPUP_TO_1ST;
        }else{}
      break;

      /* Ramp Up Pwr from: Step 0 to 1  */
      case S01_RAMPUP_TO_1ST:
        //Ramp-up pressure to pre-infussion power level
        g_Pump_param_s.smPump.sRunning  = ST_TIMER;
        g_Pump_param_s.smPump.sNext = S12_KEEP_1ST;
        g_Pump_param_s.Counter = g_Pump_param_s.RampUp_tCounts+1;
        g_Pump_param_s.Slope   = g_Pump_param_s.SlopeTo_1stP;
        //Send driver status
        pump_status = PUMPCTRL_STEP_1ST;
      break;
      /* Keep Pwr from: Step 1 to 2  */
      case S12_KEEP_1ST:
        g_Pump_param_s.smPump.sRunning  = ST_TIMER;
        g_Pump_param_s.smPump.sNext = S23_RAMPUP_TO_2ND;
        g_Pump_param_s.Counter = g_Pump_param_s.tCounts_1stP+1;
        //Pressure in pump remains intact -> Slope = 0
        g_Pump_param_s.Slope   = 0;
        //Send driver status
        pump_status = PUMPCTRL_STEP_1ST;
      break;
      /* Ramp Up Pwr from: Step 2 to 3  */
      case S23_RAMPUP_TO_2ND:
        g_Pump_param_s.smPump.sRunning  = ST_TIMER;
        g_Pump_param_s.smPump.sNext = S34_KEEP_2ND;
        g_Pump_param_s.Counter = g_Pump_param_s.RampUp_tCounts+1;
        g_Pump_param_s.Slope   = g_Pump_param_s.SlopeTo_2ndP;
        //Send driver status
        pump_status = PUMPCTRL_STEP_2ND;
      break;
      /* Keep Pwr from: Step 3 to 4  */
      case S34_KEEP_2ND:
        //Load timer to mantain th maximum pressure power for a certain time
        g_Pump_param_s.smPump.sRunning  = ST_TIMER;
        g_Pump_param_s.smPump.sNext = S45_RAMPDOWN_TO_3RD;
        g_Pump_param_s.Counter = g_Pump_param_s.tCounts_2ndP;
        //Pressure in pump remains intact -> Slope = 0
        g_Pump_param_s.Slope   = 0;     
        //Send driver status 
        pump_status = PUMPCTRL_STEP_2ND;
      break;
      /* Ramp Down Pwr from: Steo 4 to 5  */
      case S45_RAMPDOWN_TO_3RD:
        g_Pump_param_s.smPump.sRunning  = ST_TIMER;
        g_Pump_param_s.smPump.sNext = S56_KEEP_3RD;
        g_Pump_param_s.Counter = g_Pump_param_s.RampDown_tCounts+1;
        g_Pump_param_s.Slope   = g_Pump_param_s.SlopeTo_3rdP;
        //Send driver status
        pump_status = PUMPCTRL_STEP_3RD;
      break;
      /* Keep Pwr from: Step 5 to 6  */
      case S56_KEEP_3RD:
        g_Pump_param_s.smPump.sRunning  = ST_TIMER;
        g_Pump_param_s.smPump.sNext = S67_RAMPDOWN_TO_STOP;
        g_Pump_param_s.Counter = g_Pump_param_s.tCounts_3rdP+1;
        //Pressure in pump remains intact -> Slope = 0
        g_Pump_param_s.Slope   = 0;
        //Send driver status
        pump_status = PUMPCTRL_STEP_3RD;
      break;
      /* Ramp Down Pwr from: Steo 6 to 7  */
      case S67_RAMPDOWN_TO_STOP:
        g_Pump_param_s.smPump.sRunning  = ST_TIMER;
        g_Pump_param_s.smPump.sNext = ST_STOP;
        g_Pump_param_s.Counter = g_Pump_param_s.RampDown_tCounts+1;
        g_Pump_param_s.Slope   = g_Pump_param_s.SlopeTo_Stop;
        //Send driver status
        pump_status = PUMPCTRL_STEP_STOP;
      break;

      case ST_TIMER:
        if(-- g_Pump_param_s.Counter != 0 )
        {

          if( g_Pump_param_s.Slope != 0)
          {
            volatile int16_t pump_pwr = (int16_t)g_Pump_param_s.PumpPower;
            pump_pwr += (int16_t)g_Pump_param_s.Slope;
            g_Pump_param_s.PumpPower = pump_pwr;
            //fcn_SSR_pwrUpdate((struct_SSRinstance *)&sPumpSSRdrv, g_Pump_param_s.PumpPower);
            fcn_pumpSSR_pwrUpdate(g_Pump_param_s.PumpPower);
          }else{}
          
        }else{
          g_Pump_param_s.smPump.sRunning = g_Pump_param_s.smPump.sNext;
        }
        //Send driver status
        pump_status = PUMPCTRL_STEP_TIME;
      break;

      case ST_STOP:
        //Open Pump's valve by deactivating selenoid
        fcn_SolenoidSSR_Off();
        g_Pump_param_s.PumpPower = 0;
        fcn_pumpSSR_pwrUpdate(g_Pump_param_s.PumpPower);
        g_Pump_param_s.smPump.sRunning  = S0_IDLE;
        //Send driver status
        pump_status = PUMPCTRL_STEP_STOP;
      break;

      default:
        //smPumpCtrl.smPump.sRunning  = S0_IDLE;
        //Send driver status
        pump_status = PUMPCTRL_ERROR;
      break;
  }
  return pump_status;
}

/*****************************************************************************
 * Function: 	InitClocks
 * Description: 
 *****************************************************************************/
void start_brew(void)
{
    g_Pump_param_s.smPump.sRunning = S00_CLOSE_SOLENOID;
}

/*****************************************************************************
* Function: 	InitClocks
* Description: 
*****************************************************************************/
void cancel_brew(void)
{
   g_Pump_param_s.smPump.sRunning = S67_RAMPDOWN_TO_STOP;
}

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	load_pump_param
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
static void load_pump_param(pump_profile_data_t *ptr_profile_param, pump_param_t *ptr_pump_param)
{
  /* STEPS TIME DURATION - TIMER COUNT's CALCULATION */
  /* Counts (time) from step 0 to 1 */
  /* Counts (time) from step 2 to 3 */
  ptr_pump_param->RampUp_tCounts   = (uint16_t)(ptr_profile_param->Time_Rampup_ms / PUMP_BASETIME_T_MS);
  /* Counts (time) from step 4 to 5 */
  /* Counts (time) from step 6 to 7 */
  ptr_pump_param->RampDown_tCounts = (uint16_t)(ptr_profile_param->Time_Rampdown_ms / PUMP_BASETIME_T_MS);
  /* Counts (time) from step 1 to 2 */
  ptr_pump_param->tCounts_1stP     = (uint16_t)(ptr_profile_param->Time_1stP_ms / PUMP_BASETIME_T_MS);
  /* Counts (time) from step 3 to 4 */
  ptr_pump_param->tCounts_2ndP     = (uint16_t)(ptr_profile_param->Time_2ndP_ms / PUMP_BASETIME_T_MS);
  /* Counts (time) from step 5 to 6 */
  ptr_pump_param->tCounts_3rdP     = (uint16_t)(ptr_profile_param->Time_3rdP_ms / PUMP_BASETIME_T_MS);

  /* H1 fix: guard all slope divisors against zero.
   * A BLE write of 0 for any timing parameter maps to a tCounts of 0.
   * Integer-division by zero is undefined behaviour and causes a hard fault.
   * Clamp to 1 so the slope is calculated as a single-step ramp rather
   * than crashing; the resulting pump profile is non-ideal but safe. */
  if (ptr_pump_param->RampUp_tCounts   == 0) ptr_pump_param->RampUp_tCounts   = 1;
  if (ptr_pump_param->RampDown_tCounts == 0) ptr_pump_param->RampDown_tCounts = 1;
  if (ptr_pump_param->tCounts_3rdP     == 0) ptr_pump_param->tCounts_3rdP     = 1;

  /*SLOPES BETWEEN STEPS CALCULATION */
  /* Slope calc from Step 0 to 1  */
  ptr_pump_param->SlopeTo_1stP     = (int16_t)     ptr_profile_param->Pwr_1stP / ptr_pump_param->RampUp_tCounts;
  /* Slope calc from Step 2 to 3  */
  ptr_pump_param->SlopeTo_2ndP     = (int16_t)  (( ptr_profile_param->Pwr_2ndP - ptr_profile_param->Pwr_1stP) / ptr_pump_param->RampUp_tCounts)+1;
  /* Slope calc from Step 4 to 5  */
  ptr_pump_param->SlopeTo_3rdP     = (int16_t) -(((ptr_profile_param->Pwr_2ndP - ptr_profile_param->Pwr_3rdP) / ptr_pump_param->tCounts_3rdP)+1);
  /* Slope calc from Step 6 to 7  */
  ptr_pump_param->SlopeTo_Stop     = (int16_t) -(( ptr_profile_param->Pwr_3rdP / ptr_pump_param->RampDown_tCounts)+2);
}

