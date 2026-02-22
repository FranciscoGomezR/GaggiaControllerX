//NOTES: Solenoid valve either directs water into the steam valve or to the group head.
//      Default state when power is active = water goes to the group of the machine.
//      when switchinb syteam button, valve is deactivated.
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "BLEspressoServices.h"
#include "tempController.h"
#include "spi_Devices.h"
#include "x01_StateMachineControls.h"
#include "solidStateRelay_Controller.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************
#define PUMP_PWR_ON             1000    //100%
#define PUMP_PWR_OFF            0       //0%

#define TEMP_CTRL_PHI2_THR      1.0f

#define STPFCN_HEATING_PWR      1000    //100%
#define STPFCN_TIMEBASE         0.1f                      //seconds
#define STPFCN_START_DELAY      (30.0f/STPFCN_TIMEBASE)   //Ticks
#define STPFCN_LOG_PRINT        (0.5f/STPFCN_TIMEBASE)    //Ticks

#define SERVICE_BASE_T_MS       100                       //time in ms
#define SVC_MONITOR_TICK          (500/SERVICE_BASE_T_MS)   //Ticks

#define SVC_LOG_LEN             80

static const char *TAG_SYS_MONITOR ="[System]  <Monitor:";
static const char *TAG_SYS_MSG   =  "[System]  <Message:";
static const char *TAG_PROF_MODE =  "[Profile]    <Mode:";
static const char *TAG_CLAS_MODE =  "[Classic]    <Mode:";

//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
typedef enum {
  cl_idle = 0,
  cl_Mode_1,
  cl_Mode_2,
  cl_Mode_3,
  cl_Mode_max
} s_espresso_status_t;

typedef enum {
  prof_idle = 0,
  prof_Mode,
  prof_Mode_Infuse,
  prof_Mode_Decline,
  prof_Mode_Halt,
  prof_Mode_2,
  prof_Mode_3,
  prof_Mode_max
} s_profile_status_t;

typedef enum {
  sf_idle = 0,
  sf_Mode_1,
  sf_Mode_2a,
  sf_Mode_2b,
  sf_Mode_max
} s_stepfcn_status_t;

typedef enum {
  NO_SWITCH = 0,
  BREW_SWTICH,
  STEAM_SWTICH,
  BOTH_SWTICH
} swtich_status_t;

typedef struct{
  uint16_t        heatingPwr;
  uint16_t        pumpPwr;
  uint16_t        svcStartT;
  bool            b_boostI_phase1;
  bool            b_boostI_phase2;
  bool            b_normalI;
}s_classic_data_t;

typedef struct{
  uint16_t        tick;
  uint16_t        tickTabTarget;
  uint16_t        heatingPwr;
  int16_t         delta_pumpPwr;
  uint16_t        base_pumpPwr;
  uint16_t        pumpPwr;
  uint16_t        svcStartT;
  const float     a_expGrowth[14]; 
  const float     a_expDecay[14];
  const uint16_t  noTabs; 
  const float     *ptrTab;
  uint16_t        tabCnt; 
  bool            b_activeFlg;
  bool            b_boostI_phase1;
  bool            b_boostI_phase2;
  bool            b_normalI;
}s_profile_data_t;

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
volatile bleSpressoUserdata_struct blEspressoProfile;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
static StateMachineCtrl_Struct espressoService_Status = {cl_idle,cl_idle,cl_idle};
static StateMachineCtrl_Struct profileService_Status = {prof_idle,prof_idle,prof_idle};
static StateMachineCtrl_Struct stepfcnService_Status = {sf_idle,sf_idle,sf_idle};
static s_classic_data_t classicData = {
                                      .pumpPwr=0,
                                      .b_normalI=true,
                                      .b_boostI_phase1=false,
                                      .b_boostI_phase2=false
                                      };

static s_profile_data_t profileData = {
                                      .tick=0,
                                      .pumpPwr=0,
                                      .a_expGrowth= {
                                                    0.39f,0.63f,0.75f,0.86f,0.90f,0.95f,0.97f,
                                                    0.98f,0.99f,1.00f,1.00f,1.00f,1.00f,1.00f
                                                    },
                                      .a_expDecay= {
                                                    0.60f,0.37f,0.25f,0.13f,0.09f,0.05f,0.03f,
                                                    0.02f,0.01f,0.00f,0.00f,0.00f,0.00f,0.00f
                                                    },
                                      .noTabs = 10,
                                      .b_normalI=true,
                                      .b_boostI_phase1=false,
                                      .b_boostI_phase2=false
                                      };

//Data to be printed into the Serial Terminal 
static uint32_t serviceTick=0;
static uint32_t app_timeStamp;
static uint16_t app_PumpPwr;
static uint16_t app_HeatPwr;
static float    f_BoilerTemp;
static float    f_BoilerTragetTemp;
static bool     b_phase2_flag = false;

/*This variable controls the timing inside this module: 
  1-  Delay for the step function start
  2-  Log Print period
*/
static uint32_t stpfcn_tickCounter;

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************
uint32_t intservice_GetSwitcheState(void);

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	fcn_service_EspressoApp
 * Prerequisite:fcn shall be called every 100ms
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_service_ClassicMode(acInput_status_t swBrew, acInput_status_t swSteam)
{
  float svrDurationT;
  uint8_t  aLogText[SVC_LOG_LEN]={0};
  static bool espApp_initialized = false;  // persists across calls

  if (!espApp_initialized) {
    // Code to run only once
    #if(NRF_LOG_ENABLED == 1)
      sprintf(aLogText,"%s;%08d;Espresso Machine enters into ::CLASSIC MODE::;", 
                        TAG_SYS_MSG,
                        serviceTick*100);
      NRF_LOG_RAW_INFO("%s\n",aLogText);
      NRF_LOG_FLUSH(); 
    #endif
    espApp_initialized = true;
  }

  serviceTick++;
  if( !(serviceTick % SVC_MONITOR_TICK))
  {
    /* Get Boiler Temperature
       Get Target Temperature
       run Temperature Controller */
    f_BoilerTemp=(float)f_getBoilerTemperature();
    f_BoilerTragetTemp = (float)blEspressoProfile.temp_Target;
    classicData.heatingPwr = (uint16_t)fcn_updateTemperatureController((bleSpressoUserdata_struct *)&blEspressoProfile);
    #if SERVICE_HEAT_ACTION_EN == 1
      fcn_boilerSSR_pwrUpdate(classicData.heatingPwr);
    #endif

    //Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)
    #if(NRF_LOG_ENABLED == 1)
      sprintf(aLogText,"%s;%08d;%04d;%.1f;%.2f;%04d;", 
                        TAG_SYS_MONITOR,
                        serviceTick*100,
                        classicData.heatingPwr,
                        f_BoilerTragetTemp,
                        f_BoilerTemp,
                        app_PumpPwr);
      NRF_LOG_RAW_INFO("%s\n",aLogText);
      NRF_LOG_FLUSH(); 
    #endif
  }else{} 
  //Controller is running Phase2-I gain, now let's monitor Temp to return to main I gain. 
  if(classicData.b_boostI_phase2 == true)
  {
    if( (float)(f_BoilerTemp + TEMP_CTRL_PHI2_THR) > (float)f_BoilerTragetTemp )
    {
      //When boiler temp. reach target temp controller revert to main gains. (I-gain)
      classicData.b_boostI_phase2 = false;
      classicData.b_normalI = true;
      fcn_multiplyI_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile,1.0f);
    }else{}
  }else{}

  switch(espressoService_Status.sRunning)
  {
    case cl_idle:
        //SWITCH Activation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {
          //Save: Strating time
          classicData.svcStartT = serviceTick;
          //ACTION: Increase I gain
          fcn_loadIboost_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile);
          classicData.b_normalI=false;
          classicData.b_boostI_phase1=true;
          //ACTION: Solenoid valve ON
          fcn_SolenoidSSR_On();
          //ACTION: Pump ON 
          classicData.pumpPwr = PUMP_PWR_ON;
          #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(classicData.pumpPwr);
          #endif
          //STATE JUMP: Mode1
          espressoService_Status.sRunning= cl_Mode_1;
          #if(NRF_LOG_ENABLED == 1)
            sprintf(aLogText,"%s;%08d;msg::Pulling a shot of espresso Start_time(ms)::;", 
                              TAG_CLAS_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH();
          #endif
        }else{}
        //SWITCH Activation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          //ACTION: Setting new target temperatire for Steam generation
          blEspressoProfile.temp_Target = blEspressoProfile.sp_StemTemp;
          //STATE JUMP: Mode2A
          espressoService_Status.sRunning = cl_Mode_2;
          #if(NRF_LOG_ENABLED == 1)
            sprintf(aLogText,"%s;%08d;msg::Steam generation Start_time(ms)::;", 
                              TAG_CLAS_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
    break;

    case cl_Mode_1:
        //SWITCH Deactivation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {}else{
          //STATE JUMP: Mode_Max
          espressoService_Status.sRunning = cl_idle;
          //ACTION: Pump OFF 
          classicData.pumpPwr = PUMP_PWR_OFF;
          #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(classicData.pumpPwr);
          #endif
          //ACTION: Decrease I gain to Ki*2 and set flag monitor
          fcn_multiplyI_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile,2.0f);
          classicData.b_boostI_phase1=false;
          classicData.b_boostI_phase2=true;
          b_phase2_flag = true;
          //ACTION: Solenoid OFF 
          fcn_SolenoidSSR_Off();
          #if(NRF_LOG_ENABLED == 1)
            sprintf(aLogText,"%s;%08d;msg::Service Stop_time(ms)::;", 
                              TAG_CLAS_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
            svrDurationT = (float)((serviceTick-classicData.svcStartT)*100);
            svrDurationT = svrDurationT/1000.0f;
            
            sprintf(aLogText,"%s;%08d;%.2f s;msg::Espresso shot Duration_time(s)::;", 
                              TAG_CLAS_MODE,
                              serviceTick*100,
                              svrDurationT);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
          #endif
        }
        //BUTTON Activation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          //ACTION: Solenoid OFF 
          fcn_SolenoidSSR_Off();
          //ACTION: Setting new target temperatire for Steam generation
          blEspressoProfile.temp_Target = blEspressoProfile.sp_StemTemp;
          //STATE JUMP: Mode2A
          espressoService_Status.sRunning = cl_Mode_3;
          #if(NRF_LOG_ENABLED == 1)
            sprintf(aLogText,"%s;%08d;msg::Pump On + Solenoid Shut::;", 
                              TAG_CLAS_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
          #endif
        }else{}
    break;

    case cl_Mode_2:
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        classicData.pumpPwr = PUMP_PWR_ON;
        #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(classicData.pumpPwr);
        #endif
        //STATE JUMP: Mode2B
        espressoService_Status.sRunning= cl_Mode_3;
        #if(NRF_LOG_ENABLED == 1)
          sprintf(aLogText,"%s;%08d;msg::Pump On + Solenoid Shut::;", 
                            TAG_CLAS_MODE,
                            serviceTick*100);
          NRF_LOG_RAW_INFO("%s\n",aLogText); 
        #endif
      }else{}
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: shut pump down.
        classicData.pumpPwr = PUMP_PWR_OFF;
        #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(classicData.pumpPwr);
        #endif
        //ACTION: Setting target temperature back to brew setpoint
        blEspressoProfile.temp_Target = blEspressoProfile.sp_BrewTemp;
        //STATE JUMP: idle
        espressoService_Status.sRunning= cl_idle;
        #if(NRF_LOG_ENABLED == 1)
          sprintf(aLogText,"%s;%08d;msg::Steam Generation Stop_time(ms)::;", 
                            TAG_CLAS_MODE,
                            serviceTick*100);
          NRF_LOG_RAW_INFO("%s\n",aLogText);
          NRF_LOG_FLUSH();  
        #endif
      }
    break;

    case cl_Mode_3:
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: shut pump down.
        classicData.pumpPwr = PUMP_PWR_OFF;
        #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(classicData.pumpPwr);
        #endif
        //STATE JUMP [easy]: To idle, then this stage will take care of the switch state
        espressoService_Status.sRunning= cl_idle;
        //STATE JUMP: Mode2B
        //espressoService_Status.sRunning= cl_Mode_2;
        #if(NRF_LOG_ENABLED == 1)
          sprintf(aLogText,"%s;%08d;msg::Steam purge Stop_time(ms)::;", 
                            TAG_CLAS_MODE,
                            serviceTick*100);
          NRF_LOG_RAW_INFO("%s\n",aLogText);
        #endif
      }
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: Setting target temperature back to brew setpoint
        blEspressoProfile.temp_Target = blEspressoProfile.sp_BrewTemp;
        //STATE JUMP [easy]: To idle, then this stage will take care of the switch state
        espressoService_Status.sRunning= cl_idle;
        //STATE JUMP: Mode1A
        //espressoService_Status.sRunning= cl_Mode_1;
        #if(NRF_LOG_ENABLED == 1)
          sprintf(aLogText,"%s;%08d;msg::Steam purge Stop_time(ms)::;", 
                            TAG_CLAS_MODE,
                            serviceTick*100);
          NRF_LOG_RAW_INFO("%s\n",aLogText);
        #endif
      }
    break;

    case cl_Mode_max:      
      espressoService_Status.sRunning= cl_idle;
    break;
  }
}

/*****************************************************************************
 * Function: 	fcn_service_ProfileMode
 * Prerequisite:fcn shall be called every 100ms
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_service_ProfileMode(acInput_status_t swBrew, acInput_status_t swSteam)
{ 
  float svrDurationT;
  uint8_t  aLogText[SVC_LOG_LEN]={0};
  static bool espApp_initialized = false;  // persists across calls

  if (!espApp_initialized) {
    // Code to run only once
    #if(NRF_LOG_ENABLED == 1)
    sprintf(aLogText,"%s;%08d;Espresso Machine enters into ::PROFILE MODE::;", 
                      TAG_SYS_MSG,
                      serviceTick*100);
    NRF_LOG_RAW_INFO("%s\n",aLogText);
    NRF_LOG_FLUSH(); 
    #endif
    espApp_initialized = true;
  }

  serviceTick++;
  if( !(serviceTick % SVC_MONITOR_TICK))
  {
    /* Get Boiler Temperature
       Get Target Temperature
       run Temperature Controller */
    f_BoilerTemp=(float)f_getBoilerTemperature();
    f_BoilerTragetTemp = (float)blEspressoProfile.temp_Target;
    profileData.heatingPwr = (uint16_t)fcn_updateTemperatureController((bleSpressoUserdata_struct *)&blEspressoProfile);
    #if SERVICE_HEAT_ACTION_EN == 1
      fcn_boilerSSR_pwrUpdate(profileData.heatingPwr);
    #endif
    //Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)
    #if(NRF_LOG_ENABLED == 1)
      sprintf(aLogText,"%s;%08d;%04d;%.1f;%.2f;%04d;", 
                        TAG_SYS_MONITOR,
                        serviceTick*100,
                        profileData.heatingPwr,
                        f_BoilerTragetTemp,
                        f_BoilerTemp,
                        profileData.pumpPwr);
      NRF_LOG_RAW_INFO("%s\n",aLogText);
      NRF_LOG_FLUSH(); 
    #endif
  }else{} 
  //Controller is running Phase2-I gain, now let's monitor Temp to return to main I gain. 
  if(profileData.b_boostI_phase2 == true)
  {
    if( (float)(f_BoilerTemp + TEMP_CTRL_PHI2_THR) > (float)f_BoilerTragetTemp )
    {
      //When boiler temp. reach target temp controller revert to main gains. (I-gain)
      profileData.b_boostI_phase2 = false;
      profileData.b_normalI = true;
      fcn_multiplyI_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile,1.0f);
    }else{}
  }else{}

  switch(profileService_Status.sRunning)
  {
    case prof_idle:
        //SWITCH Activation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {
          profileData.b_activeFlg=true;
          profileData.b_normalI=false;
          profileData.b_boostI_phase1=true;
          //ACTION: Increase I gain
          //LOAD: Data for Profiler state
          profileData.tickTabTarget = (uint16_t)(1000.0f*(blEspressoProfile.prof_preInfuseTmr+0.999999f));
          profileData.tickTabTarget = (profileData.tickTabTarget / SERVICE_BASE_T_MS) / profileData.noTabs;
          profileData.tick          = profileData.tickTabTarget;
          //Let's calculate delta power
          profileData.delta_pumpPwr= (int16_t)((blEspressoProfile.prof_preInfusePwr
                                            - 0.0f) * 10.0f);
          profileData.base_pumpPwr=0;
          //Delta will always be positive -> Growth table will be use
          profileData.ptrTab=&profileData.a_expGrowth[0];
          //Load # of tab to go through
          profileData.tabCnt=profileData.noTabs;
          profileData.pumpPwr   = (uint16_t)(((float)profileData.delta_pumpPwr)*(*profileData.ptrTab));
          //profileData.pumpPwr+= profileData.base_pumpPwr; 
          //ACTION: Solenoid valve ON
          fcn_SolenoidSSR_On();
          //ACTION: Pump ON 
          #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(profileData.pumpPwr);
          #endif
          //Save: Strating time
          profileData.svcStartT = serviceTick;
          //STATE JUMP: Profiler Move
          profileService_Status.sRunning= prof_Mode;
          profileService_Status.sNext = prof_Mode_Infuse;
          #if(NRF_LOG_ENABLED == 1)
            sprintf(aLogText,"%s;%08d;msg::Pulling a shot of espresso::;", 
                              TAG_PROF_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
            sprintf(aLogText,"%s;%08d;msg::Pre infusion Start_time(ms)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
        //SWITCH Activation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          //ACTION: Setting new target temperatire for Steam generation
          blEspressoProfile.temp_Target = blEspressoProfile.sp_StemTemp;
          //STATE JUMP: Mode2A
          profileService_Status.sRunning = prof_Mode_2;
          #if(NRF_LOG_ENABLED == 1)
            sprintf(aLogText,"%s;%08d;msg::Steam generation Start_time(ms)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
    break;

    case prof_Mode:
      //SWITCH deactivation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        profileData.tick--;
        //End of current profile step time?
        if(profileData.tick==0)
        {
          //Move to next tab within table
          profileData.tabCnt--;
          //Load timer.
          profileData.tick = profileData.tickTabTarget;
          //ACTION: Increase I gain
          //LOAD: next Data for Profiler
          //Let's calculate new step power for the pump
          profileData.ptrTab++;
          profileData.pumpPwr = (uint16_t)(((float)profileData.delta_pumpPwr)*(*profileData.ptrTab));
          profileData.pumpPwr+= profileData.base_pumpPwr;
          //Action: Increment Integral Gain accordingly to pump PWR. e.g. 100%PWR Igain=I-boost & 0%PWR Igain=I
          fcn_multiplyI_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile,
                                          (float)(1.0f+((float)profileData.pumpPwr/10.0f)) );
          //ACTION: Introduce new power value.
          #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(profileData.pumpPwr);
          #endif
        }else{}
        if(profileData.tabCnt==0)
        {
          profileService_Status.sRunning = profileService_Status.sNext;
        }else{}
      }else{
        profileData.b_activeFlg=false;
        profileData.b_boostI_phase1=false;
        profileData.b_boostI_phase2=true;
        fcn_multiplyI_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile,2.0f);
        //ACTION: shut pump down.
        app_PumpPwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_PumpPwr);
        //ACTION: Solenoid OFF 
        fcn_SolenoidSSR_Off();
        //STATE JUMP: IDLE
        profileService_Status.sRunning= prof_idle;
        #if(NRF_LOG_ENABLED == 1)
            svrDurationT = (float)((serviceTick-profileData.svcStartT)*100);
            svrDurationT = (float)(svrDurationT/1000.0f);

            sprintf(aLogText,"%s;%08d;msg::Service Stop_time(ms)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 

            sprintf(aLogText,"%s;%08d;%.2fs;msg::Espresso shot Duration_time(s)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100,
                              svrDurationT);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
        #endif
      }
      //SWITCH Activation: Steam -> IGNORED
    break;

    case prof_Mode_Infuse:
      //SWITCH deactivation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        //ACTION: Increase I gain
        //LOAD: Data for Profiler state
        profileData.tickTabTarget = (uint16_t)(1000.0f*(blEspressoProfile.prof_InfuseTmr+0.999999f));
        profileData.tickTabTarget = (profileData.tickTabTarget / SERVICE_BASE_T_MS) / profileData.noTabs;
        profileData.tick          = profileData.tickTabTarget;
        //Let's calculate delta power
        profileData.delta_pumpPwr= (int16_t)((blEspressoProfile.prof_InfusePwr
                                          - blEspressoProfile.prof_preInfusePwr) * 10.0f);
        //check oid delta pumpPwr is possitve or negative
        if( profileData.delta_pumpPwr>0)
        {
          //Save base pumpPwr ->previous pumpPwr step
          profileData.base_pumpPwr = (uint16_t)(blEspressoProfile.prof_preInfusePwr * 10.0f);
          //positive Delta -> Growth table will be use
          profileData.ptrTab=&profileData.a_expGrowth[0];
          //Load # of tab to go through
          profileData.tabCnt=profileData.noTabs;
        }else{
          profileData.delta_pumpPwr *= -1;
          //Save base pumpPwr ->target pumpPwr step
          profileData.base_pumpPwr = (uint16_t)(blEspressoProfile.Prof_DeclinePwr * 10.0f);
          //negative Delta -> Decay table will be use
          profileData.ptrTab=&profileData.a_expDecay[0];
          //Load # of tab to go through
          profileData.tabCnt=profileData.noTabs;
        }
        profileData.pumpPwr  = (uint16_t)(((float)profileData.delta_pumpPwr)*(*profileData.ptrTab));
        profileData.pumpPwr += profileData.base_pumpPwr; 
        //ACTION: Pump ON 
        #if SERVICE_PUMP_ACTION_EN == 1
          fcn_pumpSSR_pwrUpdate(profileData.pumpPwr);
        #endif
        //STATE JUMP: Profiler Move
        profileService_Status.sRunning= prof_Mode;
        profileService_Status.sNext = prof_Mode_Decline;
        #if(NRF_LOG_ENABLED == 1)
            sprintf(aLogText,"%s;%08d;msg::Infusion stage Start_time(ms)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
        #endif  
      }else{
        profileData.b_activeFlg=false;
        profileData.b_boostI_phase1=false;
        profileData.b_boostI_phase2=true;
        fcn_multiplyI_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile,2.0f);
        //ACTION: shut pump down.
        app_PumpPwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_PumpPwr);
        //ACTION: Solenoid OFF 
        fcn_SolenoidSSR_Off();
        //STATE JUMP: IDLE
        profileService_Status.sRunning= prof_idle;
        #if(NRF_LOG_ENABLED == 1)
            svrDurationT = (float)((serviceTick-profileData.svcStartT)*100);
            svrDurationT = svrDurationT/1000.0f;
            
            sprintf(aLogText,"%s;%08d;msg::Service Stop_time(ms)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 

            sprintf(aLogText,"%s;%08d;%.2f s;msg::Espresso shot Duration_time(s)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100,
                              svrDurationT);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
        #endif
      }
      //SWITCH Activation: Steam -> IGNORED
    break;

    case prof_Mode_Decline:
      //SWITCH deactivation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        //ACTION: Increase I gain
        //LOAD: Data for Profiler state
        profileData.tickTabTarget = (uint16_t)(1000.0f*(blEspressoProfile.Prof_DeclineTmr+0.999999f));
        profileData.tickTabTarget = (profileData.tickTabTarget / SERVICE_BASE_T_MS) / profileData.noTabs;
        profileData.tick          = profileData.tickTabTarget;
        //Let's calculate delta power
        profileData.delta_pumpPwr= (int16_t)((blEspressoProfile.Prof_DeclinePwr
                                          - blEspressoProfile.prof_InfusePwr) * 10.0f);
        //check oid delta pumpPwr is possitve or negative
        if( profileData.delta_pumpPwr>0)
        {
          //Save base pumpPwr ->previous pumpPwr step
          profileData.base_pumpPwr = (uint16_t)(blEspressoProfile.prof_InfusePwr * 10.0f);
          //positive Delta -> Growth table will be use
          profileData.ptrTab=&profileData.a_expGrowth[0];
          //Load # of tab to go through
          profileData.tabCnt=profileData.noTabs;
        }else{
          profileData.delta_pumpPwr *= -1;
          //Save base pumpPwr ->target pumpPwr step
          profileData.base_pumpPwr = (uint16_t)(blEspressoProfile.Prof_DeclinePwr * 10.0f);
          //negative Delta -> Decay table will be use
          profileData.ptrTab=&profileData.a_expDecay[0];
          //Load # of tab to go through
          profileData.tabCnt=profileData.noTabs;
        }
        profileData.pumpPwr  = (uint16_t)(((float)profileData.delta_pumpPwr)*(*profileData.ptrTab));
        profileData.pumpPwr += profileData.base_pumpPwr; 
        //ACTION: Pump ON 
        #if SERVICE_PUMP_ACTION_EN == 1
          fcn_pumpSSR_pwrUpdate(profileData.pumpPwr);
        #endif
        //STATE JUMP: Profiler Move
        profileService_Status.sRunning= prof_Mode;
        profileService_Status.sNext = prof_Mode_Halt;
        #if(NRF_LOG_ENABLED == 1)
          sprintf(aLogText,"%s;%08d;msg::Decline stage Start_time(ms)::;", 
                            TAG_PROF_MODE,
                            serviceTick*100);
          NRF_LOG_RAW_INFO("%s\n",aLogText);
          NRF_LOG_FLUSH(); 
        #endif  
      }else{
        profileData.b_activeFlg=false;
        profileData.b_boostI_phase1=false;
        profileData.b_boostI_phase2=true;
        fcn_multiplyI_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile,2.0f);
        //ACTION: shut pump down.
        app_PumpPwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_PumpPwr);
        //ACTION: Solenoid OFF 
        fcn_SolenoidSSR_Off();
        //STATE JUMP: IDLE
        profileService_Status.sRunning= prof_idle;
        #if(NRF_LOG_ENABLED == 1)
            svrDurationT = (float)((serviceTick-profileData.svcStartT)*100);
            svrDurationT = svrDurationT/1000.0f;
            sprintf(aLogText,"%s;%08d;msg::Service Stop_time(ms)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
            sprintf(aLogText,"%s;%08d;%.2f s;msg::Espresso shot Duration_time(s)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100,
                              svrDurationT);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
        #endif
      }
      //SWITCH Activation: Steam -> IGNORED
    break;

    case prof_Mode_Halt:
      if(profileData.b_activeFlg==true)
      {
        profileData.b_activeFlg=false;
        profileData.b_boostI_phase1=false;
        profileData.b_boostI_phase2=true;
        fcn_multiplyI_ParamToCtrl_Temp((bleSpressoUserdata_struct*)&blEspressoProfile,2.0f);
        //ACTION: shut pump down.
        app_PumpPwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_PumpPwr);
        //ACTION: Solenoid OFF 
        fcn_SolenoidSSR_Off();
        #if(NRF_LOG_ENABLED == 1)
            svrDurationT = (float)((serviceTick-profileData.svcStartT)*100);
            svrDurationT = svrDurationT/1000.0f;
            sprintf(aLogText,"%s;%08d;msg::Ready::;\n\t\t\t\tmsg::Espresso shot time%.2f(s)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100,
                              svrDurationT);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
            sprintf(aLogText,"%s;msg::Deactive Brew switch to reset::;", 
                              TAG_PROF_MODE);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH();
        #endif 
      }else{}
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {}else{
        //STATE JUMP: IDLE
        profileService_Status.sRunning= prof_idle;
        #if(NRF_LOG_ENABLED == 1)
          sprintf(aLogText,"%s;%08d;msg::Ready to pull a new espresso shot::;", 
                              TAG_PROF_MODE,
                              serviceTick*100);
          NRF_LOG_RAW_INFO("%s\n",aLogText);
          NRF_LOG_FLUSH();
        #endif 
      }
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED && swBrew == AC_SWITCH_ASSERTED )
      {
        //Go to Mode 3.
      }else{}
    break;

    case prof_Mode_2:
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {       
        app_PumpPwr = PUMP_PWR_ON;
        fcn_pumpSSR_pwrUpdate(app_PumpPwr);
        //STATE JUMP: Mode2B
        profileService_Status.sRunning= prof_Mode_3;
        #if(NRF_LOG_ENABLED == 1)
          sprintf(aLogText,"%s;%08d;msg::Pump On + Solenoid Shut::;", 
                            TAG_PROF_MODE,
                            serviceTick*100);
          NRF_LOG_RAW_INFO("%s\n",aLogText); 
        #endif
      }else{}
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: shut pump down.
        app_PumpPwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_PumpPwr);
        //ACTION: Setting target temperature back to brew setpoint
        blEspressoProfile.temp_Target = blEspressoProfile.sp_BrewTemp;
        //STATE JUMP: idle
        profileService_Status.sRunning= prof_idle;
        #if(NRF_LOG_ENABLED == 1)
          sprintf(aLogText,"%s;%08d;msg::Steam Generation Stop_time(ms)::;", 
                            TAG_PROF_MODE,
                            serviceTick*100);
          NRF_LOG_RAW_INFO("%s\n",aLogText);
          NRF_LOG_FLUSH();
        #endif
      }
    break;

    case prof_Mode_3:
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: shut pump down.
        app_PumpPwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_PumpPwr);
        //STATE JUMP: Mode2B
        profileService_Status.sRunning= prof_Mode_2;
        #if(NRF_LOG_ENABLED == 1)
            sprintf(aLogText,"%s;%08d;msg::Steam generation Start_time(ms)::;", 
                              TAG_PROF_MODE,
                              serviceTick*100);
            NRF_LOG_RAW_INFO("%s\n",aLogText);
            NRF_LOG_FLUSH(); 
        #endif
      }
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: Setting target temperature back to brew setpoint
        blEspressoProfile.temp_Target = blEspressoProfile.sp_BrewTemp;
        //STATE JUMP: Mode1A
        profileService_Status.sRunning= prof_idle;
        #if(NRF_LOG_ENABLED == 1)
          sprintf(aLogText,"%s;%08d;msg::Pulling a shot of espresso Start_time(ms)::;", 
                            TAG_PROF_MODE,
                            serviceTick*100);
          NRF_LOG_RAW_INFO("%s\n",aLogText);
        #endif
      }
    break;

    case prof_Mode_max:
      profileService_Status.sRunning= prof_idle;      
    break;
  }
}


/*****************************************************************************
 * Function: 	fcn_service_StepFunction
 * Prerequisite:fcn shall be called every 100ms
 * Parameters:	
 * Return:
 *****************************************************************************/
void fcn_service_StepFunction(acInput_status_t swBrew, acInput_status_t swSteam)
{
  static bool Stpfcn_initialized = false;  // persists across calls
  static bool Stpfcn_HeatingStatus = false;  // persists across calls

  if (!Stpfcn_initialized) {
    // Code to run only once
    #if(NRF_LOG_ENABLED == 1)
    NRF_LOG_INFO("BLE Espresso Machine has entered into: \r\n Step Function Mode \r\n this mode is helps to fast-tune \n the PID controller for the boiler temperatuer\r\n");
    NRF_LOG_INFO("STP_FCN ::IDLE::");
    NRF_LOG_FLUSH(); 
    #endif
    Stpfcn_initialized = true;
  }

  switch(stepfcnService_Status.sRunning)
  {
    case sf_idle:
        //SWITCH Activation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {
          //ACTION: Solenoid valve ON
          fcn_SolenoidSSR_On();
          //ACTION: Pump ON 
          app_PumpPwr = PUMP_PWR_ON;
          fcn_pumpSSR_pwrUpdate(app_PumpPwr);
          //STATE JUMP: Mode1
          stepfcnService_Status.sRunning= sf_Mode_1;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::FILLING BOILER::");
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
        //SWITCH Activation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          //STATE JUMP: Mode2A
          stepfcnService_Status.sRunning = sf_Mode_2a;
          //Set intial parameters before jumping into next mode
          app_timeStamp=0;
          app_HeatPwr=0;
          //REset stepCounter. this var counts for 5s to get initial temperature of the boiler
          //before the step function stars
          stpfcn_tickCounter=0;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::INIT STEP FUNCTION::");
            NRF_LOG_INFO("STP_FCN ::5second delay     ::");
            NRF_LOG_INFO("STP_FCN ::FORMAT            ::");
            NRF_LOG_INFO("STP_FCN;TimeStamp;HeatPWR;BoilerTemperature");
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
    break;

    case sf_Mode_1:
        //SWITCH Deactivation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {}else{
          //STATE JUMP: Mode_Max
          stepfcnService_Status.sRunning= sf_Mode_max;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::STOPPING FILLING BOILER::");
            NRF_LOG_FLUSH(); 
          #endif
        }
    break;

    case sf_Mode_2a:
        //ACTION: NOT operation to [ tick_counter % 5 = 0 ]  
        if( !(stpfcn_tickCounter % (uint32_t)STPFCN_LOG_PRINT) )
        {
          /* Get Boiler Temperature
          and save it here => blEspressoProfile.temp_Boiler.*/
          //spim_ReadRTDconverter();
          f_BoilerTemp=(float)f_getBoilerTemperature();
          //f_BoilerTemp=blEspressoProfile.temp_Boiler;
          //Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)
          #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("STPFCN;%08d;%04d;" NRF_LOG_FLOAT_MARKER ";",
                            app_timeStamp,
                            app_HeatPwr,
                            NRF_LOG_FLOAT(f_BoilerTemp));
              NRF_LOG_FLUSH(); 
          #endif
          //Increment TimeStamp and StepCounter
          app_timeStamp+=500;
        }else{}
        //STATE JUMP: Mode2B
        if(stpfcn_tickCounter >= (uint32_t)STPFCN_START_DELAY)
        {
          //Reset counter
          stpfcn_tickCounter=0;
          stepfcnService_Status.sRunning= sf_Mode_2b;
        }
        //ACTION:Increment tick Counter
        stpfcn_tickCounter++;

        //SWITCH Deactivation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {}else{
          stepfcnService_Status.sRunning = sf_Mode_max;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::STOPPING STEP FUNCTION::");
            NRF_LOG_FLUSH(); 
          #endif
        }
    break;

    case sf_Mode_2b:
        //EXECUTION: Only one time
        if(!Stpfcn_HeatingStatus)
        {
          //ACTION: Activating Heating Element to 100%
          app_HeatPwr=STPFCN_HEATING_PWR;
          fcn_boilerSSR_pwrUpdate(app_HeatPwr);
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("\nSTP_FCN ::START::\n");
            NRF_LOG_FLUSH(); 
          #endif
          Stpfcn_HeatingStatus = true;
        }else{}
        //ACTION: NOT operation to [ tick_counter % 5 = 0 ]  
        if( !(stpfcn_tickCounter % (uint32_t)STPFCN_LOG_PRINT) )
        {
          /* Get Boiler Temperature
          and save it here => blEspressoProfile.temp_Boiler.*/
          spim_ReadRTDconverter();
          f_BoilerTemp=(float)f_getBoilerTemperature();
          //f_BoilerTemp=blEspressoProfile.temp_Boiler;
          //Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)
          #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("STPFCN;%08d;%04d;" NRF_LOG_FLOAT_MARKER ";",
                            app_timeStamp,
                            app_HeatPwr,
                            NRF_LOG_FLOAT(f_BoilerTemp));
              NRF_LOG_FLUSH(); 
          #endif
          //Increment TimeStamp and StepCounter
          app_timeStamp+=500;
        }else{}
        //ACTION: increment counter
        stpfcn_tickCounter++;

        //SWITCH Deactivation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {}else{
          app_timeStamp=0;
          Stpfcn_HeatingStatus = false;
          stepfcnService_Status.sRunning = sf_Mode_max;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::STOPPING STEP FUNCTION::");
            NRF_LOG_FLUSH(); 
          #endif
        }

    break;

    case sf_Mode_max:  
      //ACTION: Pump OFF 
      app_PumpPwr = PUMP_PWR_OFF;
      fcn_pumpSSR_pwrUpdate(app_PumpPwr);
      //ACTION: Solenoid OFF 
      fcn_SolenoidSSR_Off();
      //ACTION: Heat OFF 
      app_HeatPwr=PUMP_PWR_OFF;
      fcn_boilerSSR_pwrUpdate(app_HeatPwr);
      //Reset counter
      stpfcn_tickCounter=0;
      stepfcnService_Status.sRunning = sf_idle;
      #if(NRF_LOG_ENABLED == 1)
          NRF_LOG_INFO("STP_FCN ::STOP::");
          NRF_LOG_FLUSH();    
     #endif
    break;

  }
}

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	intservice_GetSwitcheState
 * Description: Read the state of each switch and reported back
 * Return:      switches states
 *****************************************************************************/
