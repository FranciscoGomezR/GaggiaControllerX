//NOTES: Solenoid valve either directs water into the steam valve or to the group head.
//      Default state when power is active = water goes to the group of the machine.
//      when switchinb syteam button, valve is deactivated.
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "espressoMachineServices.h"
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

/* H5: Maximum continuous brew duration.  120 s @ 100 ms/tick = 1200 ticks. */
#define MAX_BREW_TICKS            (120000 / SERVICE_BASE_T_MS)

#define SVC_LOG_LEN             90

static const char *TAG_SYS_MONITOR ="[System]  <Monitor:";
static const char *TAG_SYS_MSG   =  "[System]  <Message:";
static const char *TAG_SYS_FORMAT  ="[System]   <Format:";
static const char *TAG_PROF_MODE =  "[Profile]    <Mode:";
static const char *TAG_CLAS_MODE =  "[Classic]    <Mode:";

//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
typedef enum {
  CLASSIC_IDLE = 0,
  CLASSIC_MODE_1,
  CLASSIC_MODE_2,
  CLASSIC_MODE_3,
  CLASSIC_MODE_MAX
} s_espresso_status_t;

typedef enum {
  PROFILE_IDLE = 0,
  PROFILE_MODE_PREINFUSE,
  PROFILE_MODE_INFUSE,
  PROFILE_MODE_DECLINE,
  PROFILE_MODE_STOP,
  PROFILE_MODE_STEAM,
  PROFILE_MODE_STEAM_BREW,
  PROFILE_MODE_MAX
} s_profile_status_t;

typedef enum {
  SF_IDLE = 0,
  SF_MODE_1,
  SF_MODE_2A,
  SF_MODE_2B,
  SF_MODE_MAX
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
  bool            is_boostI_phase1;
  bool            is_boostI_phase2;
  bool            is_normalI;
}s_classic_data_t;

typedef struct{
  uint16_t        tick;
  uint16_t        tickTabTarget;
  uint16_t        heatingPwr;
  int16_t         delta_pumpPwr;
  uint16_t        base_pumpPwr;
  uint16_t        pumpPwr;
  uint16_t        svcStartT;
  const float     exp_growth_arr[14]; 
  const float     exp_decay_arr[14];
  const uint16_t  noTabs; 
  const float     *ptrTab;
  uint16_t        tabCnt; 
  bool            is_active;
  bool            is_boostI_phase1;
  bool            is_boostI_phase2;
  bool            is_normalI;
}s_profile_data_t;

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************
volatile espresso_user_config_t g_Espresso_user_config_s;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
static StateMachineCtrl_Struct Espresso_service_status_s = {CLASSIC_IDLE,CLASSIC_IDLE,CLASSIC_IDLE};
static StateMachineCtrl_Struct Profile_service_status_s = {PROFILE_IDLE,PROFILE_IDLE,PROFILE_IDLE};
static StateMachineCtrl_Struct Stepfcn_service_status_s = {SF_IDLE,SF_IDLE,SF_IDLE};
static s_classic_data_t Classic_data_s = {
                                      .pumpPwr=0,
                                      .is_normalI=true,
                                      .is_boostI_phase1=false,
                                      .is_boostI_phase2=false
                                      };

static s_profile_data_t Profile_data_s = {
                                      .tick=0,
                                      .pumpPwr=0,
                                      .exp_growth_arr= {
                                                    0.39f,0.63f,0.75f,0.86f,0.90f,0.95f,0.97f,
                                                    0.98f,0.99f,1.00f,1.00f,1.00f,1.00f,1.00f
                                                    },
                                      .exp_decay_arr= {
                                                    0.60f,0.37f,0.25f,0.13f,0.09f,0.05f,0.03f,
                                                    0.02f,0.01f,0.00f,0.00f,0.00f,0.00f,0.00f
                                                    },
                                      .noTabs = 10,
                                      .is_normalI=true,
                                      .is_boostI_phase1=false,
                                      .is_boostI_phase2=false
                                      };

//Data to be printed into the Serial Terminal 
static uint32_t service_tick=0;

#ifdef TEST
void     test_set_service_tick(uint32_t t) { service_tick = t; }
uint32_t test_get_service_tick(void)       { return service_tick; }
#endif

static uint32_t app_timestamp_msecs;
static uint16_t app_pump_pwr;
static uint16_t app_heat_pwr;
static float    boiler_temp_degC;
static float    boiler_target_temp_degC;
static bool     is_phase2 = false;

/*This variable controls the timing inside this module: 
  1-  Delay for the step function start
  2-  Log Print period
*/
static uint32_t stpfcn_tick_cnt;

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************
static uint32_t get_switch_state(void);

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
  float svr_duration_msecs;
  uint8_t  log_text_arr[SVC_LOG_LEN]={0};
  static bool is_app_initialized = false;  // persists across calls

  if (!is_app_initialized) {
    // Code to run only once
    #if(NRF_LOG_ENABLED == 1)
      sprintf((char*)log_text_arr,"%s;%08d;Espresso Machine enters into ::CLASSIC MODE::;", 
                        TAG_SYS_MSG,
                        service_tick*100);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH(); 
    #endif
    is_app_initialized = true;
  }

  service_tick++;
  if( !(service_tick % SVC_MONITOR_TICK))
  {
    /* Get Boiler Temperature
       Get Target Temperature
       run Temperature Controller */
    boiler_temp_degC=(float)f_getBoilerTemperature();
    boiler_target_temp_degC = (float)g_Espresso_user_config_s.boilerTempSetpointDegC;
    Classic_data_s.heatingPwr = (uint16_t)temp_ctrl_update((espresso_user_config_t *)&g_Espresso_user_config_s);
    #if SERVICE_HEAT_ACTION_EN == 1
      fcn_boilerSSR_pwrUpdate(Classic_data_s.heatingPwr);
    #endif

    //Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)
    #if(NRF_LOG_ENABLED == 1)
      sprintf((char *)log_text_arr,"%s;%08d;%04d;%.1f;%.2f;%04d;", 
                        TAG_SYS_MONITOR,
                        service_tick*100,
                        Classic_data_s.heatingPwr,
                        boiler_target_temp_degC,
                        boiler_temp_degC,
                        app_pump_pwr);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH(); 
    #endif
  }else{} 
  //Controller is running Phase2-I gain, now let's monitor Temp to return to main I gain. 
  if(Classic_data_s.is_boostI_phase2 == true)
  {
    if( (float)(boiler_temp_degC + TEMP_CTRL_PHI2_THR) > (float)boiler_target_temp_degC )
    {
      //When boiler temp. reach target temp controller revert to main gains. (I-gain)
      Classic_data_s.is_boostI_phase2 = false;
      Classic_data_s.is_normalI = true;
      temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s,1.0f);
    }else{}
  }else{}

  switch(Espresso_service_status_s.sRunning)
  {
    case CLASSIC_IDLE:
        //SWITCH Activation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {
          /* M5 fix: revert phase2 recovery gain before applying a new I-boost.
           * Prevents gain stacking when the user cycles brew ON/OFF rapidly
           * before the boiler recovers to target temperature. */
          if (Classic_data_s.is_boostI_phase2) {
            temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s, 1.0f);
            Classic_data_s.is_boostI_phase2 = false;
            Classic_data_s.is_normalI = true;
          }
          //Save: Strating time
          Classic_data_s.svcStartT = service_tick;
          //ACTION: Increase I gain
          temp_ctrl_set_operational_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s);
          Classic_data_s.is_normalI=false;
          Classic_data_s.is_boostI_phase1=true;
          //ACTION: Solenoid valve ON
          fcn_SolenoidSSR_On();
          //ACTION: Pump ON 
          Classic_data_s.pumpPwr = PUMP_PWR_ON;
          #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(Classic_data_s.pumpPwr);
          #endif
          //STATE JUMP: Mode1
          Espresso_service_status_s.sRunning= CLASSIC_MODE_1;
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Pulling a shot of espresso Start_time(ms)::;", 
                              TAG_CLAS_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
          #endif
        }else{}
        //SWITCH Activation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          //ACTION: Setting new target temperatire for Steam generation
          g_Espresso_user_config_s.boilerTempSetpointDegC = g_Espresso_user_config_s.steamTempDegC;
          //STATE JUMP: Mode2A
          Espresso_service_status_s.sRunning = CLASSIC_MODE_2;
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Steam generation Start_time(ms)::;", 
                              TAG_CLAS_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
    break;

    case CLASSIC_MODE_1:
        /* H5: enforce maximum brew duration — auto-stop after 120 s */
        if ((service_tick - Classic_data_s.svcStartT) >= MAX_BREW_TICKS) {
          Espresso_service_status_s.sRunning = CLASSIC_IDLE;
          Classic_data_s.pumpPwr = PUMP_PWR_OFF;
          #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(Classic_data_s.pumpPwr);
          #endif
          temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s, 2.0f);
          Classic_data_s.is_boostI_phase1 = false;
          Classic_data_s.is_boostI_phase2 = true;
          is_phase2 = true;
          fcn_SolenoidSSR_Off();
          break;
        }
        //SWITCH Deactivation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {}else{
          //STATE JUMP: Mode_Max
          Espresso_service_status_s.sRunning = CLASSIC_IDLE;
          //ACTION: Pump OFF 
          Classic_data_s.pumpPwr = PUMP_PWR_OFF;
          #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(Classic_data_s.pumpPwr);
          #endif
          //ACTION: Decrease I gain to Ki*2 and set flag monitor
          temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s,2.0f);
          Classic_data_s.is_boostI_phase1=false;
          Classic_data_s.is_boostI_phase2=true;
          is_phase2 = true;
          //ACTION: Solenoid OFF 
          fcn_SolenoidSSR_Off();
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Service Stop_time(ms)::;", 
                              TAG_CLAS_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
            svr_duration_msecs = (float)((service_tick-Classic_data_s.svcStartT)*100);
            svr_duration_msecs = svr_duration_msecs/1000.0f;
            
            sprintf((char *)log_text_arr,"%s;%08d;%.2f s;msg::Espresso shot Duration_time(s)::;", 
                              TAG_CLAS_MODE,
                              service_tick*100,
                              svr_duration_msecs);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
          #endif
        }
        //BUTTON Activation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          //ACTION: Solenoid OFF 
          fcn_SolenoidSSR_Off();
          //ACTION: Setting new target temperatire for Steam generation
          g_Espresso_user_config_s.boilerTempSetpointDegC = g_Espresso_user_config_s.steamTempDegC;
          //STATE JUMP: Mode2A
          Espresso_service_status_s.sRunning = CLASSIC_MODE_3;
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Pump On + Solenoid Shut::;", 
                              TAG_CLAS_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
          #endif
        }else{}
    break;

    case CLASSIC_MODE_2:
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        Classic_data_s.pumpPwr = PUMP_PWR_ON;
        #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(Classic_data_s.pumpPwr);
        #endif
        //STATE JUMP: Mode2B
        Espresso_service_status_s.sRunning= CLASSIC_MODE_3;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Pump On + Solenoid Shut::;", 
                            TAG_CLAS_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr); 
        #endif
      }else{}
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: shut pump down.
        Classic_data_s.pumpPwr = PUMP_PWR_OFF;
        #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(Classic_data_s.pumpPwr);
        #endif
        //ACTION: Setting target temperature back to brew setpoint
        g_Espresso_user_config_s.boilerTempSetpointDegC = g_Espresso_user_config_s.brewTempDegC;
        //STATE JUMP: idle
        Espresso_service_status_s.sRunning= CLASSIC_IDLE;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Steam Generation Stop_time(ms)::;", 
                            TAG_CLAS_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
          NRF_LOG_FLUSH();  
        #endif
      }
    break;

    case CLASSIC_MODE_3:
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: shut pump down.
        Classic_data_s.pumpPwr = PUMP_PWR_OFF;
        #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(Classic_data_s.pumpPwr);
        #endif
        //STATE JUMP [easy]: To idle, then this stage will take care of the switch state
        Espresso_service_status_s.sRunning= CLASSIC_IDLE;
        //STATE JUMP: Mode2B
        //Espresso_service_status_s.sRunning= CLASSIC_MODE_2;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Steam purge Stop_time(ms)::;", 
                            TAG_CLAS_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
        #endif
      }
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: Setting target temperature back to brew setpoint
        g_Espresso_user_config_s.boilerTempSetpointDegC = g_Espresso_user_config_s.brewTempDegC;
        //STATE JUMP [easy]: To idle, then this stage will take care of the switch state
        Espresso_service_status_s.sRunning= CLASSIC_IDLE;
        //STATE JUMP: Mode1A
        //Espresso_service_status_s.sRunning= CLASSIC_MODE_1;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Steam purge Stop_time(ms)::;", 
                            TAG_CLAS_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
        #endif
      }
    break;

    case CLASSIC_MODE_MAX:      
      Espresso_service_status_s.sRunning= CLASSIC_IDLE;
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
  float svr_duration_msecs;
  uint8_t  log_text_arr[SVC_LOG_LEN]={0};
  static bool is_app_initialized = false;  // persists across calls

  if (!is_app_initialized) {
    // Code to run only once
    #if(NRF_LOG_ENABLED == 1)
      /* Print time at 0 msecs */
      sprintf((char *)log_text_arr,"%s;%08d",
                          TAG_SYS_MSG,
                          service_tick*100);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH(); 

      sprintf((char *)log_text_arr,"%s;Espresso Machine enters into ::PROFILE MODE::;", 
                              TAG_SYS_MSG);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH(); 

      sprintf((char *)log_text_arr,"%s;Time_Miliseconds;Heating_Power;Boiler_Target_DegC;Boiler_Temp_DegC;Pump_Power",
                              TAG_SYS_FORMAT);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH(); 
    #endif
    is_app_initialized = true;
  }

  service_tick++;
  if( !(service_tick % SVC_MONITOR_TICK))
  {
    /* Get Boiler Temperature
       Get Target Temperature
       run Temperature Controller */
    boiler_temp_degC=(float)f_getBoilerTemperature();
    boiler_target_temp_degC = (float)g_Espresso_user_config_s.boilerTempSetpointDegC;
    Profile_data_s.heatingPwr = (uint16_t)temp_ctrl_update((espresso_user_config_t *)&g_Espresso_user_config_s);
    #if SERVICE_HEAT_ACTION_EN == 1
      fcn_boilerSSR_pwrUpdate(Profile_data_s.heatingPwr);
    #endif
    //Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)
    #if(NRF_LOG_ENABLED == 1)
      sprintf((char *)log_text_arr,"%s;%08d;%04d;%.1f;%.2f;%04d;", 
                        TAG_SYS_MONITOR,
                        service_tick*100,
                        Profile_data_s.heatingPwr,
                        boiler_target_temp_degC,
                        boiler_temp_degC,
                        Profile_data_s.pumpPwr);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH(); 
    #endif
  }else{} 
  //Controller is running Phase2-I gain, now let's monitor Temp to return to main I gain. 
  if(Profile_data_s.is_boostI_phase2 == true)
  {
    if( (float)(boiler_temp_degC + TEMP_CTRL_PHI2_THR) > (float)boiler_target_temp_degC )
    {
      //When boiler temp. reach target temp controller revert to main gains. (I-gain)
      Profile_data_s.is_boostI_phase2 = false;
      Profile_data_s.is_normalI = true;
      temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s,1.0f);
    }else{}
  }else{}

  switch(Profile_service_status_s.sRunning)
  {
    case PROFILE_IDLE:
        //SWITCH Activation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {
          /* M5 fix: revert phase2 recovery gain before starting a new brew cycle. */
          if (Profile_data_s.is_boostI_phase2) {
            temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s, 1.0f);
            Profile_data_s.is_boostI_phase2 = false;
            Profile_data_s.is_normalI = true;
          }
          Profile_data_s.is_active=true;
          Profile_data_s.is_normalI=false;
          Profile_data_s.is_boostI_phase1=true;
          //ACTION: Increase I gain
          //LOAD: Data for Profiler state
          Profile_data_s.tickTabTarget = (uint16_t)(1000.0f*(g_Espresso_user_config_s.profPreInfuseTmr+0.999999f));
          Profile_data_s.tickTabTarget = (Profile_data_s.tickTabTarget / SERVICE_BASE_T_MS) / Profile_data_s.noTabs;
          Profile_data_s.tick          = Profile_data_s.tickTabTarget;
          //Let's calculate delta power
          Profile_data_s.delta_pumpPwr= (int16_t)((g_Espresso_user_config_s.profPreInfusePwr
                                            - 30.0f) * 10.0f);

          Profile_data_s.base_pumpPwr=(uint16_t)(30 * 10.0f);
          //Delta will always be positive -> Growth table will be use
          Profile_data_s.ptrTab=&Profile_data_s.exp_growth_arr[0];
          //Load # of tab to go through
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
          Profile_data_s.pumpPwr   = (uint16_t)(((float)Profile_data_s.delta_pumpPwr)*(*Profile_data_s.ptrTab));
          Profile_data_s.pumpPwr+= Profile_data_s.base_pumpPwr; 
          //ACTION: Solenoid valve ON
          fcn_SolenoidSSR_On();
          //ACTION: Pump ON 
          #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(Profile_data_s.pumpPwr);
          #endif
          //Save: Strating time
          Profile_data_s.svcStartT = service_tick;
          //STATE JUMP: Profiler Move
          Profile_service_status_s.sRunning= PROFILE_MODE_PREINFUSE;
          Profile_service_status_s.sNext = PROFILE_MODE_INFUSE;
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Pulling a shot of espresso::;", 
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
            sprintf((char *)log_text_arr,"%s;%08d;msg::Pre infusion Start_time(ms)::;", 
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
        //SWITCH Activation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          //ACTION: Setting new target temperatire for Steam generation
          g_Espresso_user_config_s.boilerTempSetpointDegC = g_Espresso_user_config_s.steamTempDegC;
          //STATE JUMP: Mode2A
          Profile_service_status_s.sRunning = PROFILE_MODE_STEAM;
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Steam generation Start_time(ms)::;", 
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
    break;

    case PROFILE_MODE_PREINFUSE:
      /* H5: enforce maximum brew duration — auto-stop after 120 s */
      if ((service_tick - Profile_data_s.svcStartT) >= MAX_BREW_TICKS) {
        Profile_data_s.is_active = false;
        Profile_data_s.is_boostI_phase1 = false;
        Profile_data_s.is_boostI_phase2 = true;
        temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s, 2.0f);
        app_pump_pwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_pump_pwr);
        fcn_SolenoidSSR_Off();
        Profile_service_status_s.sRunning = PROFILE_IDLE;
        break;
      }
      //SWITCH deactivation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        Profile_data_s.tick--;
        //End of current profile step time?
        if(Profile_data_s.tick==0)
        {
          //Move to next tab within table
          Profile_data_s.tabCnt--;
          //Load timer.
          Profile_data_s.tick = Profile_data_s.tickTabTarget;
          //ACTION: Increase I gain
          //LOAD: next Data for Profiler
          //Let's calculate new step power for the pump
          Profile_data_s.ptrTab++;
          Profile_data_s.pumpPwr = (uint16_t)(((float)Profile_data_s.delta_pumpPwr)*(*Profile_data_s.ptrTab));
          Profile_data_s.pumpPwr+= Profile_data_s.base_pumpPwr;
          //Action: Increment Integral Gain accordingly to pump PWR. e.g. 100%PWR Igain=I-boost & 0%PWR Igain=I
          temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s,
                                          (float)(1.0f+((float)Profile_data_s.pumpPwr/10.0f)) );
          //ACTION: Introduce new power value.
          #if SERVICE_PUMP_ACTION_EN == 1
            fcn_pumpSSR_pwrUpdate(Profile_data_s.pumpPwr);
          #endif
        }else{}
        if(Profile_data_s.tabCnt==0)
        {
          Profile_service_status_s.sRunning = Profile_service_status_s.sNext;
        }else{}
      }else{
        Profile_data_s.is_active=false;
        Profile_data_s.is_boostI_phase1=false;
        Profile_data_s.is_boostI_phase2=true;
        temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s,2.0f);
        //ACTION: shut pump down.
        app_pump_pwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_pump_pwr);
        //ACTION: Solenoid OFF 
        fcn_SolenoidSSR_Off();
        //STATE JUMP: IDLE
        Profile_service_status_s.sRunning= PROFILE_IDLE;
        #if(NRF_LOG_ENABLED == 1)
            svr_duration_msecs = (float)((service_tick-Profile_data_s.svcStartT)*100);
            svr_duration_msecs = (float)(svr_duration_msecs/1000.0f);

            sprintf((char *)log_text_arr,"%s;%08d;msg::Service Stop_time(ms)::;", 
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 

            sprintf((char *)log_text_arr,"%s;%08d;%.2fs;msg::Espresso shot Duration_time(s)::;", 
                              TAG_PROF_MODE,
                              service_tick*100,
                              svr_duration_msecs);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
        #endif
      }
      //SWITCH Activation: Steam -> IGNORED
    break;

    case PROFILE_MODE_INFUSE:
      //SWITCH deactivation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        //ACTION: Increase I gain
        //LOAD: Data for Profiler state
        Profile_data_s.tickTabTarget = (uint16_t)(1000.0f*(g_Espresso_user_config_s.profInfuseTmr+0.999999f));
        Profile_data_s.tickTabTarget = (Profile_data_s.tickTabTarget / SERVICE_BASE_T_MS) / Profile_data_s.noTabs;
        Profile_data_s.tick          = Profile_data_s.tickTabTarget;
        //Let's calculate delta power
        Profile_data_s.delta_pumpPwr= (int16_t)((g_Espresso_user_config_s.profInfusePwr
                                          - g_Espresso_user_config_s.profPreInfusePwr) * 10.0f);
        //check oid delta pumpPwr is possitve or negative
        if( Profile_data_s.delta_pumpPwr>0)
        {
          //Save base pumpPwr ->previous pumpPwr step
          Profile_data_s.base_pumpPwr = (uint16_t)(g_Espresso_user_config_s.profPreInfusePwr * 10.0f);
          //positive Delta -> Growth table will be use
          Profile_data_s.ptrTab=&Profile_data_s.exp_growth_arr[0];
          //Load # of tab to go through
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
        }else{
          Profile_data_s.delta_pumpPwr *= -1;
          //Save base pumpPwr ->target pumpPwr step
          Profile_data_s.base_pumpPwr = (uint16_t)(g_Espresso_user_config_s.profTaperingPwr * 10.0f);
          //negative Delta -> Decay table will be use
          Profile_data_s.ptrTab=&Profile_data_s.exp_decay_arr[0];
          //Load # of tab to go through
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
        }
        Profile_data_s.pumpPwr  = (uint16_t)(((float)Profile_data_s.delta_pumpPwr)*(*Profile_data_s.ptrTab));
        Profile_data_s.pumpPwr += Profile_data_s.base_pumpPwr; 
        //ACTION: Pump ON 
        #if SERVICE_PUMP_ACTION_EN == 1
          fcn_pumpSSR_pwrUpdate(Profile_data_s.pumpPwr);
        #endif
        //STATE JUMP: Profiler Move
        Profile_service_status_s.sRunning= PROFILE_MODE_PREINFUSE;
        Profile_service_status_s.sNext = PROFILE_MODE_DECLINE;
        #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Infusion stage Start_time(ms)::;", 
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
        #endif  
      }else{
        Profile_data_s.is_active=false;
        Profile_data_s.is_boostI_phase1=false;
        Profile_data_s.is_boostI_phase2=true;
        temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s,2.0f);
        //ACTION: shut pump down.
        app_pump_pwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_pump_pwr);
        //ACTION: Solenoid OFF 
        fcn_SolenoidSSR_Off();
        //STATE JUMP: IDLE
        Profile_service_status_s.sRunning= PROFILE_IDLE;
        #if(NRF_LOG_ENABLED == 1)
            svr_duration_msecs = (float)((service_tick-Profile_data_s.svcStartT)*100);
            svr_duration_msecs = svr_duration_msecs/1000.0f;
            
            sprintf((char *)log_text_arr,"%s;%08d;msg::Service Stop_time(ms)::;", 
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 

            sprintf((char *)log_text_arr,"%s;%08d;%.2f s;msg::Espresso shot Duration_time(s)::;", 
                              TAG_PROF_MODE,
                              service_tick*100,
                              svr_duration_msecs);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
        #endif
      }
      //SWITCH Activation: Steam -> IGNORED
    break;

    case PROFILE_MODE_DECLINE:
      //SWITCH deactivation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        //ACTION: Increase I gain
        //LOAD: Data for Profiler state
        Profile_data_s.tickTabTarget = (uint16_t)(1000.0f*(g_Espresso_user_config_s.profTaperingTmr+0.999999f));
        Profile_data_s.tickTabTarget = (Profile_data_s.tickTabTarget / SERVICE_BASE_T_MS) / Profile_data_s.noTabs;
        Profile_data_s.tick          = Profile_data_s.tickTabTarget;
        //Let's calculate delta power
        Profile_data_s.delta_pumpPwr= (int16_t)((g_Espresso_user_config_s.profTaperingPwr
                                          - g_Espresso_user_config_s.profInfusePwr) * 10.0f);
        //check oid delta pumpPwr is possitve or negative
        if( Profile_data_s.delta_pumpPwr>0)
        {
          //Save base pumpPwr ->previous pumpPwr step
          Profile_data_s.base_pumpPwr = (uint16_t)(g_Espresso_user_config_s.profInfusePwr * 10.0f);
          //positive Delta -> Growth table will be use
          Profile_data_s.ptrTab=&Profile_data_s.exp_growth_arr[0];
          //Load # of tab to go through
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
        }else{
          Profile_data_s.delta_pumpPwr *= -1;
          //Save base pumpPwr ->target pumpPwr step
          Profile_data_s.base_pumpPwr = (uint16_t)(g_Espresso_user_config_s.profTaperingPwr * 10.0f);
          //negative Delta -> Decay table will be use
          Profile_data_s.ptrTab=&Profile_data_s.exp_decay_arr[0];
          //Load # of tab to go through
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
        }
        Profile_data_s.pumpPwr  = (uint16_t)(((float)Profile_data_s.delta_pumpPwr)*(*Profile_data_s.ptrTab));
        Profile_data_s.pumpPwr += Profile_data_s.base_pumpPwr; 
        //ACTION: Pump ON 
        #if SERVICE_PUMP_ACTION_EN == 1
          fcn_pumpSSR_pwrUpdate(Profile_data_s.pumpPwr);
        #endif
        //STATE JUMP: Profiler Move
        Profile_service_status_s.sRunning= PROFILE_MODE_PREINFUSE;
        Profile_service_status_s.sNext = PROFILE_MODE_STOP;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Decline stage Start_time(ms)::;", 
                            TAG_PROF_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
          NRF_LOG_FLUSH(); 
        #endif  
      }else{
        Profile_data_s.is_active=false;
        Profile_data_s.is_boostI_phase1=false;
        Profile_data_s.is_boostI_phase2=true;
        temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s,2.0f);
        //ACTION: shut pump down.
        app_pump_pwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_pump_pwr);
        //ACTION: Solenoid OFF 
        fcn_SolenoidSSR_Off();
        //STATE JUMP: IDLE
        Profile_service_status_s.sRunning= PROFILE_IDLE;
        #if(NRF_LOG_ENABLED == 1)
            svr_duration_msecs = (float)((service_tick-Profile_data_s.svcStartT)*100);
            svr_duration_msecs = svr_duration_msecs/1000.0f;
            sprintf((char *)log_text_arr,"%s;%08d;msg::Service Stop_time(ms)::;", 
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
            sprintf((char *)log_text_arr,"%s;%08d;%.2f s;msg::Espresso shot Duration_time(s)::;", 
                              TAG_PROF_MODE,
                              service_tick*100,
                              svr_duration_msecs);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
        #endif
      }
      //SWITCH Activation: Steam -> IGNORED
    break;

    case PROFILE_MODE_STOP:
      if(Profile_data_s.is_active==true)
      {
        Profile_data_s.is_active=false;
        Profile_data_s.is_boostI_phase1=false;
        Profile_data_s.is_boostI_phase2=true;
        temp_ctrl_scale_integral_gain((espresso_user_config_t*)&g_Espresso_user_config_s,2.0f);
        //ACTION: shut pump down.
        app_pump_pwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_pump_pwr);
        //ACTION: Solenoid OFF 
        fcn_SolenoidSSR_Off();
        #if(NRF_LOG_ENABLED == 1)
            svr_duration_msecs = (float)((service_tick-Profile_data_s.svcStartT)*100);
            svr_duration_msecs = svr_duration_msecs/1000.0f;
            sprintf((char *)log_text_arr,"%s;%08d;msg::Ready::;\n\t\t\t\tmsg::Espresso shot time%.2f(s)::;", 
                              TAG_PROF_MODE,
                              service_tick*100,
                              svr_duration_msecs);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
            sprintf((char *)log_text_arr,"%s;msg::Deactive Brew switch to reset::;", 
                              TAG_PROF_MODE);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
        #endif 
      }else{}
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {}else{
        //STATE JUMP: IDLE
        Profile_service_status_s.sRunning= PROFILE_IDLE;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Ready to pull a new espresso shot::;", 
                              TAG_PROF_MODE,
                              service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
          NRF_LOG_FLUSH();
        #endif 
      }
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED && swBrew == AC_SWITCH_ASSERTED )
      {
        //Go to Mode 3.
      }else{}
    break;

    case PROFILE_MODE_STEAM:
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {       
        app_pump_pwr = PUMP_PWR_ON;
        fcn_pumpSSR_pwrUpdate(app_pump_pwr);
        //STATE JUMP: Mode2B
        Profile_service_status_s.sRunning= PROFILE_MODE_STEAM_BREW;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Pump On + Solenoid Shut::;", 
                            TAG_PROF_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr); 
        #endif
      }else{}
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: shut pump down.
        app_pump_pwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_pump_pwr);
        //ACTION: Setting target temperature back to brew setpoint
        g_Espresso_user_config_s.boilerTempSetpointDegC = g_Espresso_user_config_s.brewTempDegC;
        //STATE JUMP: idle
        Profile_service_status_s.sRunning= PROFILE_IDLE;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Steam Generation Stop_time(ms)::;", 
                            TAG_PROF_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
          NRF_LOG_FLUSH();
        #endif
      }
    break;

    case PROFILE_MODE_STEAM_BREW:
      //SWITCH Activation: Brew
      if(swBrew == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: shut pump down.
        app_pump_pwr = PUMP_PWR_OFF;
        fcn_pumpSSR_pwrUpdate(app_pump_pwr);
        //STATE JUMP: Mode2B
        Profile_service_status_s.sRunning= PROFILE_MODE_STEAM;
        #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Steam generation Start_time(ms)::;", 
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH(); 
        #endif
      }
      //SWITCH Activation: Steam
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        //ACTION: Setting target temperature back to brew setpoint
        g_Espresso_user_config_s.boilerTempSetpointDegC = g_Espresso_user_config_s.brewTempDegC;
        //STATE JUMP: Mode1A
        Profile_service_status_s.sRunning= PROFILE_IDLE;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Pulling a shot of espresso Start_time(ms)::;", 
                            TAG_PROF_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
        #endif
      }
    break;

    case PROFILE_MODE_MAX:
      Profile_service_status_s.sRunning= PROFILE_IDLE;      
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
  static bool is_stpfcn_initialized = false;  // persists across calls
  static bool is_stpfcn_heating = false;  // persists across calls

  if (!is_stpfcn_initialized) {
    // Code to run only once
    #if(NRF_LOG_ENABLED == 1)
    NRF_LOG_INFO("BLE Espresso Machine has entered into: \r\n Step Function Mode \r\n this mode is helps to fast-tune \n the PID controller for the boiler temperatuer\r\n");
    NRF_LOG_INFO("STP_FCN ::IDLE::");
    NRF_LOG_FLUSH(); 
    #endif
    is_stpfcn_initialized = true;
  }

  switch(Stepfcn_service_status_s.sRunning)
  {
    case SF_IDLE:
        //SWITCH Activation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {
          //ACTION: Solenoid valve ON
          fcn_SolenoidSSR_On();
          //ACTION: Pump ON 
          app_pump_pwr = PUMP_PWR_ON;
          fcn_pumpSSR_pwrUpdate(app_pump_pwr);
          //STATE JUMP: Mode1
          Stepfcn_service_status_s.sRunning= SF_MODE_1;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::FILLING BOILER::");
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
        //SWITCH Activation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          //STATE JUMP: Mode2A
          Stepfcn_service_status_s.sRunning = SF_MODE_2A;
          //Set intial parameters before jumping into next mode
          app_timestamp_msecs=0;
          app_heat_pwr=0;
          //REset stepCounter. this var counts for 5s to get initial temperature of the boiler
          //before the step function stars
          stpfcn_tick_cnt=0;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::INIT STEP FUNCTION::");
            NRF_LOG_INFO("STP_FCN ::5second delay     ::");
            NRF_LOG_INFO("STP_FCN ::FORMAT            ::");
            NRF_LOG_INFO("STP_FCN;TimeStamp;HeatPWR;BoilerTemperature");
            NRF_LOG_FLUSH(); 
          #endif
        }else{}
    break;

    case SF_MODE_1:
        //SWITCH Deactivation: Brew
        if(swBrew == AC_SWITCH_ASSERTED )
        {}else{
          //STATE JUMP: Mode_Max
          Stepfcn_service_status_s.sRunning= SF_MODE_MAX;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::STOPPING FILLING BOILER::");
            NRF_LOG_FLUSH(); 
          #endif
        }
    break;

    case SF_MODE_2A:
        //ACTION: NOT operation to [ tick_counter % 5 = 0 ]  
        if( !(stpfcn_tick_cnt % (uint32_t)STPFCN_LOG_PRINT) )
        {
          /* Get Boiler Temperature
          and save it here => g_Espresso_user_config_s.boilerTempDegC.*/
          boiler_temp_degC=(float)f_getBoilerTemperature();
          //g_Espresso_user_config_s.boilerTempDegC;
          //Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)
          #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("STPFCN;%08d;%04d;" NRF_LOG_FLOAT_MARKER ";",
                            app_timestamp_msecs,
                            app_heat_pwr,
                            NRF_LOG_FLOAT(boiler_temp_degC));
              NRF_LOG_FLUSH(); 
          #endif
          //Increment TimeStamp and StepCounter
          app_timestamp_msecs+=500;
        }else{}
        //STATE JUMP: Mode2B
        if(stpfcn_tick_cnt >= (uint32_t)STPFCN_START_DELAY)
        {
          //Reset counter
          stpfcn_tick_cnt=0;
          Stepfcn_service_status_s.sRunning= SF_MODE_2B;
        }
        //ACTION:Increment tick Counter
        stpfcn_tick_cnt++;

        //SWITCH Deactivation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {}else{
          Stepfcn_service_status_s.sRunning = SF_MODE_MAX;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::STOPPING STEP FUNCTION::");
            NRF_LOG_FLUSH(); 
          #endif
        }
    break;

    case SF_MODE_2B:
        /* H4 fix: continuous overheat guard.
         * If the boiler temperature exceeds the safe operating limit while the
         * step function is actively applying 100 % heater power, shut the heater
         * off immediately and exit this mode.  Without this check the heater
         * runs at 100 % indefinitely even if the temperature overshoots to an
         * unsafe level. */
        if ((float)g_Espresso_user_config_s.steamTempDegC > 150.0f)
        {
          app_heat_pwr = PUMP_PWR_OFF;
          fcn_boilerSSR_pwrUpdate(app_heat_pwr);
          is_stpfcn_heating = false;
          Stepfcn_service_status_s.sRunning = SF_MODE_MAX;
          break;
        }
        //EXECUTION: Only one time
        if(!is_stpfcn_heating)
        {
          //ACTION: Activating Heating Element to 100%
          app_heat_pwr=STPFCN_HEATING_PWR;
          fcn_boilerSSR_pwrUpdate(app_heat_pwr);
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("\nSTP_FCN ::START::\n");
            NRF_LOG_FLUSH(); 
          #endif
          is_stpfcn_heating = true;
        }else{}
        //ACTION: NOT operation to [ tick_counter % 5 = 0 ]  
        if( !(stpfcn_tick_cnt % (uint32_t)STPFCN_LOG_PRINT) )
        {
          /* Get Boiler Temperature
          and save it here => g_Espresso_user_config_s.steamTempDegC.*/
          spim_ReadRTDconverter();
          boiler_temp_degC=(float)f_getBoilerTemperature();
          //boiler_temp_degC=g_Espresso_user_config_s.steamTempDegC;
          //Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)
          #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("STPFCN;%08d;%04d;" NRF_LOG_FLOAT_MARKER ";",
                            app_timestamp_msecs,
                            app_heat_pwr,
                            NRF_LOG_FLOAT(boiler_temp_degC));
              NRF_LOG_FLUSH(); 
          #endif
          //Increment TimeStamp and StepCounter
          app_timestamp_msecs+=500;
        }else{}
        //ACTION: increment counter
        stpfcn_tick_cnt++;

        //SWITCH Deactivation: Steam
        if(swSteam == AC_SWITCH_ASSERTED )
        {}else{
          app_timestamp_msecs=0;
          is_stpfcn_heating = false;
          Stepfcn_service_status_s.sRunning = SF_MODE_MAX;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::STOPPING STEP FUNCTION::");
            NRF_LOG_FLUSH(); 
          #endif
        }

    break;

    case SF_MODE_MAX:  
      //ACTION: Pump OFF 
      app_pump_pwr = PUMP_PWR_OFF;
      fcn_pumpSSR_pwrUpdate(app_pump_pwr);
      //ACTION: Solenoid OFF 
      fcn_SolenoidSSR_Off();
      //ACTION: Heat OFF 
      app_heat_pwr=PUMP_PWR_OFF;
      fcn_boilerSSR_pwrUpdate(app_heat_pwr);
      //Reset counter
      stpfcn_tick_cnt=0;
      Stepfcn_service_status_s.sRunning = SF_IDLE;
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
 * Function: 	get_switch_state
 * Description: Read the state of each switch and reported back
 * Return:      switches states
 *****************************************************************************/
