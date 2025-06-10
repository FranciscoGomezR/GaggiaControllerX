
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
  st_Idle = 0,
  st_RampupToPreInf,
  st_PreInfussion,
  st_RampupToBrew,
  st_LoadMaxPkTime,
  st_RampdownToLowPressure,
  st_LowPressureBrew,
  st_RampdownToStop,
  st_Timer,
  st_Stop
};

  typedef struct
  {
      uint16_t RampUpCounts;
      uint16_t RampDownCounts;
      uint16_t PeakTimeCounts;
      uint16_t DecliningTimeCounts;
      uint16_t PreInfussionCounts;
      int16_t SlopeToPreInfussion;
      int16_t SlopeToBrew;
      int16_t SlopeToLowPressure;
      int16_t SlopeToStop;
      uint16_t Counter;
      int16_t Slope;
  }Struct_PumpTiming;

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
StateMachinePump_Struct smPumpCtrl;
Struct_PumpTiming sPumpTiming;

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
 * Function: 	fcn_initPumpController
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
pumpCtrl_status_t fcn_initPumpController(void)
{
  smPumpCtrl.PumpPower          = 0;
  smPumpCtrl.smPump.sRunning    = st_Idle;
  smPumpCtrl.PumpBrePwr         = PUMP_BREWPWR_DEFAULT;
  smPumpCtrl.preInfussionPwr    = PUMP_PREINFUSSIONPWR_DEFAULT;
  smPumpCtrl.decliningPressurePwr = PUMP_DECLINING_PWR_DEFAULT;

  smPumpCtrl.preInfussionT_ms   = PUMP_PREINFUSSION_T_MS_DEFAULT;
  smPumpCtrl.peakPressureT_ms   = PUMP_PEAKPRESSURE_T_MS_DEFAULT;
  smPumpCtrl.decliningPressureT_ms = PUMP_DECLINING_T_MS_DEFAULT;
  smPumpCtrl.PumpRampUpT_ms     = PUMP_RAMPUP_T_MS_DEFAULT;
  smPumpCtrl.PumpRampDownT_ms   = PUMP_RAMPDOWN_T_MS_DEFAULT;
  
  sPumpTiming.RampUpCounts      = (uint16_t)(PUMP_RAMPUP_T_MS_DEFAULT / PUMP_BASETIME_T_MS);
  sPumpTiming.RampDownCounts    = (uint16_t)(PUMP_RAMPDOWN_T_MS_DEFAULT / PUMP_BASETIME_T_MS);
  sPumpTiming.PreInfussionCounts= (uint16_t)(PUMP_PREINFUSSION_T_MS_DEFAULT / PUMP_BASETIME_T_MS);
  sPumpTiming.PeakTimeCounts    = (uint16_t)(PUMP_PEAKPRESSURE_T_MS_DEFAULT / PUMP_BASETIME_T_MS);
  sPumpTiming.DecliningTimeCounts = (uint16_t)(PUMP_DECLINING_T_MS_DEFAULT / PUMP_BASETIME_T_MS);
  sPumpTiming.SlopeToPreInfussion  = PUMP_PREINFUSSIONPWR_DEFAULT / sPumpTiming.RampUpCounts;
  sPumpTiming.SlopeToBrew       = ((PUMP_BREWPWR_DEFAULT - PUMP_PREINFUSSIONPWR_DEFAULT) / sPumpTiming.RampUpCounts)+1; 
  sPumpTiming.SlopeToLowPressure= -((( PUMP_BREWPWR_DEFAULT - PUMP_DECLINING_PWR_DEFAULT) / sPumpTiming.DecliningTimeCounts)+1);
  sPumpTiming.SlopeToStop       = -((PUMP_DECLINING_PWR_DEFAULT / sPumpTiming.RampDownCounts)+2); 


  //GPIOS SECTION
  //------------------------------------------------------------------------------
  ret_code_t err_code_gpio;
  //nrf_drv_gpiote_init -> Already init. in ac_inputs_drv
  //err_code_gpio = nrf_drv_gpiote_init();
  //APP_ERROR_CHECK(err_code_gpio);
  //Output pin to control SSR
  //------------------------------------------------------------------------
  nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
  err_code_gpio = nrf_drv_gpiote_out_init(enSolenoidRelay_PIN, &out_config);
  APP_ERROR_CHECK(err_code_gpio);

  return  PUMPCTRL_INIT_OK;
}

 /*****************************************************************************
 * Function: 	fcn_PumpStateDriver
 * Description: 
 * Caveats:     State timing: 100ms
 * Parameters:	
 * Return:
 *****************************************************************************/
  void fcn_PumpStateDriver(void)
  {
    switch(smPumpCtrl.smPump.sRunning)
    {
        case st_Idle:
 
        break;

        case st_RampupToPreInf:
          //Close Pump's valve by activating solenoid
          nrf_drv_gpiote_out_set(enSolenoidRelay_PIN);
          //Ramp-up pressure to pre-infussion power level
          smPumpCtrl.smPump.sRunning  = st_Timer;
          smPumpCtrl.smPump.sNext = st_PreInfussion;
          sPumpTiming.Counter = sPumpTiming.RampUpCounts+1;
          sPumpTiming.Slope   = sPumpTiming.SlopeToPreInfussion;

          //Close Valve
        break;

        case st_PreInfussion:
          smPumpCtrl.smPump.sRunning  = st_Timer;
          smPumpCtrl.smPump.sNext = st_RampupToBrew;
          sPumpTiming.Counter = sPumpTiming.PreInfussionCounts+1;
          sPumpTiming.Slope   = 0;
        break;

        case st_RampupToBrew:
          smPumpCtrl.smPump.sRunning  = st_Timer;
          smPumpCtrl.smPump.sNext = st_LoadMaxPkTime;
          sPumpTiming.Counter = sPumpTiming.RampUpCounts+1;
          sPumpTiming.Slope   = sPumpTiming.SlopeToBrew;
        break;

        case st_LoadMaxPkTime:
          //Load timer to mantain th maximum pressure power for a certain time
          smPumpCtrl.smPump.sRunning  = st_Timer;
          smPumpCtrl.smPump.sNext = st_RampdownToLowPressure;
          sPumpTiming.Counter = sPumpTiming.PeakTimeCounts;
          sPumpTiming.Slope   = 0; //Pressure in pump remains intact
        break;

        case st_RampdownToLowPressure:
          smPumpCtrl.smPump.sRunning  = st_Timer;
          smPumpCtrl.smPump.sNext = st_LowPressureBrew;
          sPumpTiming.Counter = sPumpTiming.DecliningTimeCounts+1;
          sPumpTiming.Slope   = sPumpTiming.SlopeToLowPressure;
        break;

        case st_LowPressureBrew:
          smPumpCtrl.smPump.sRunning  = st_Timer;
          smPumpCtrl.smPump.sNext = st_RampdownToStop;
          sPumpTiming.Counter = sPumpTiming.DecliningTimeCounts+1;
          sPumpTiming.Slope   = sPumpTiming.SlopeToLowPressure;
        break;

        case st_RampdownToStop:
          smPumpCtrl.smPump.sRunning  = st_Timer;
          smPumpCtrl.smPump.sNext = st_Stop;
          sPumpTiming.Counter = sPumpTiming.RampDownCounts+1;
          sPumpTiming.Slope   = sPumpTiming.SlopeToStop;
        break;

        case st_Timer:
          if(-- sPumpTiming.Counter != 0 )
          {

            if( sPumpTiming.Slope != 0)
            {
              volatile int16_t PumpPwr = (int16_t)smPumpCtrl.PumpPower;
              PumpPwr += (int16_t)sPumpTiming.Slope;
              smPumpCtrl.PumpPower = PumpPwr;
              //fcn_SSR_pwrUpdate((struct_SSRinstance *)&sPumpSSRdrv, smPumpCtrl.PumpPower);
              fcn_pumpSSR_pwrUpdate(smPumpCtrl.PumpPower);
            }else{}
            
          }else{
            smPumpCtrl.smPump.sRunning = smPumpCtrl.smPump.sNext;
          }
        break;

        case st_Stop:
          //Open Pump's valve by deactivating selenoid
          //nrf_drv_gpiote_out_clear(enSolenoidRelay_PIN);
          smPumpCtrl.PumpPower = 0;
          fcn_pumpSSR_pwrUpdate(smPumpCtrl.PumpPower);
          smPumpCtrl.smPump.sRunning  = st_Idle;
          //fcn_SSR_pwrUpdate((struct_SSRinstance *)&sPumpSSRdrv, 0);
          //Open valve
        break;

        default:
          smPumpCtrl.smPump.sRunning  = st_Stop;
        break;
    }

  }

/*****************************************************************************
 * Function: 	InitClocks
 * Description: 
 *****************************************************************************/
    void fcn_StartBrew(void)
    {
        smPumpCtrl.smPump.sRunning = st_RampupToPreInf;
    }

/*****************************************************************************
* Function: 	InitClocks
* Description: 
*****************************************************************************/
    void fcn_CancelBrew(void)
    {
       smPumpCtrl.smPump.sRunning = st_RampdownToStop;
    }

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	InitClocks
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
 // Public function 2 