/*NOTES: Solenoid valve either directs water into the steam valve or to the group head.*/
/*      Default state when power is active = water goes to the group of the machine.*/
/*      when switchinb syteam button, valve is deactivated.*/
/******************************************************************************
*
*		INCLUDE FILE SECTION FOR THIS MODULE
*
******************************************************************************/
#include "espressoMachineServices.h"
#include "tempController.h"
#include "spi_Devices.h"
#include "x01_StateMachineControls.h"
#include "solidStateRelay_Controller.h"

/******************************************************************************
*
*		PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
*
******************************************************************************/
#define PUMP_PWR_ON             1000    /*100%*/
#define PUMP_PWR_OFF            0       /*0%*/

#define TEMP_CTRL_PHI2_THR      1.0f
#define I_BOOST_RECOVERY_FACTOR 2.0f    /*Ki x2 from brew end until boiler reaches target*/
#define PROFILE_BOOST_STAGES    2U      /*Ki ramp spans pre-infusion + infusion*/

#define STPFCN_HEATING_PWR      1000    /*100%*/
#define STPFCN_TIMEBASE         0.1f                      /*seconds*/
#define STPFCN_START_DELAY      (30.0f/STPFCN_TIMEBASE)   /*Ticks*/
#define STPFCN_LOG_PRINT        (0.5f/STPFCN_TIMEBASE)    /*Ticks*/

#define SERVICE_BASE_TIME_MSECS       100                       /*time in ms*/
#define SERVICE_MONITOR_TICK          (500/SERVICE_BASE_TIME_MSECS)   /*Ticks*/

/* H5: Maximum continuous brew duration.  120 s @ 100 ms/tick = 1200 ticks. */
#define MAX_BREW_TICKS            (120000 / SERVICE_BASE_TIME_MSECS)

#define SVC_LOG_LEN             150

static const char *TAG_SYS_MONITOR ="[System]  <Monitor";
static const char *TAG_SYS_MSG   =  "[System]  <Message:";
static const char *TAG_SYS_FORMAT  ="[System]   <Format:";
static const char *TAG_PROF_MODE =  "[Profile]    <Mode";
static const char *TAG_CLAS_MODE =  "[Classic]    <Mode";

/******************************************************************************
*
*		PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
*
******************************************************************************/
typedef enum {
  CLASSIC_IDLE = 0,
  CLASSIC_MODE_1,
  CLASSIC_MODE_2,
  CLASSIC_MODE_3,
  CLASSIC_MODE_MAX
} s_espresso_status_t;

typedef enum {
  PROFILE_IDLE = 0,
  PROFILE_MODE_RAMP_STEP,
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
  bool            isPhase1;       /* brew boost active      */
  bool            isPhase2;       /* recovery boost active  */
  bool            isNormal;       /* Ki factor 1.0          */
}i_boost_flags_t;

typedef struct{
  uint16_t        heatingPwr;
  uint16_t        pumpPwr;
  uint32_t        svcStartT;
  i_boost_flags_t iBoost;
  bool            max_time_reached;
  bool            is_active;
}s_classic_data_t;

typedef struct{
  uint16_t        tick;
  uint16_t        tickTabTarget;
  uint16_t        heatingPwr;
  int16_t         delta_pumpPwr;
  uint16_t        base_pumpPwr;
  uint16_t        pumpPwr;
  uint32_t        svcStartT;
  const float     exp_growth_arr[14];
  const float     exp_decay_arr[14];
  const uint16_t  noTabs;
  const float     *ptrTab;
  uint16_t        tabCnt;
  float           kiFactor;
  float           kiStepFactor;
  i_boost_flags_t iBoost;
  bool            is_stopped;
  bool            max_time_reached;
  bool            profile_ended;
  bool            is_active;
}s_profile_data_t;

/******************************************************************************
*
*		PUBLIC VARIABLES
*
*****************************************************************************/
volatile espresso_user_config_t g_Espresso_user_config_s;

/******************************************************************************
*
*		PRIVATE VARIABLES
*
******************************************************************************/
static StateMachineCtrl_Struct Espresso_service_status_s = {CLASSIC_IDLE,CLASSIC_IDLE,CLASSIC_IDLE};
static StateMachineCtrl_Struct Profile_service_status_s = {PROFILE_IDLE,PROFILE_IDLE,PROFILE_IDLE};
static StateMachineCtrl_Struct Stepfcn_service_status_s = {SF_IDLE,SF_IDLE,SF_IDLE};
static s_classic_data_t Classic_data_s = {
                                      .pumpPwr=0,
                                      .iBoost={false,false,true}
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
                                      .noTabs = 10,   /* <= 14: exp_*_arr size */
                                      .kiFactor=1.0f,
                                      .kiStepFactor=0.0f,
                                      .iBoost={false,false,true},
                                      .is_stopped=false,
                                      .max_time_reached=false,
                                      .profile_ended=false
                                      };

/*Data to be printed into the Serial Terminal */
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
static bool     is_setpoint_changed = false;

/*This variable controls the timing inside this module:
  1-  Delay for the step function start
  2-  Log Print period
*/
static uint32_t stpfcn_tick_cnt;

/******************************************************************************
*
*		PRIVATE FUNCTIONS PROTOYPES
*
******************************************************************************/
static uint32_t get_switch_state(void);
static void start_i_boost(i_boost_flags_t *ptr_flags, float factor);
static void start_i_recovery(i_boost_flags_t *ptr_flags);
static void monitor_i_recovery(i_boost_flags_t *ptr_flags);
static void step_profile_integral_boost(void);
static void apply_boiler_setpoint(tempCtrl_LoadSP_t setpoint);

/******************************************************************************
*
*		PUBLIC FUNCTIONS SECTION
*
******************************************************************************/
/*****************************************************************************
 * Function: 	is_boiler_setpoint_changed
 * Description: Read-and-clear the setpoint-changed flag (BLE notify trigger).
 *****************************************************************************/
bool is_boiler_setpoint_changed(void)
{
  bool is_changed = is_setpoint_changed;
  is_setpoint_changed = false;
  return is_changed;
}

/*****************************************************************************
 * Function: 	fcn_service_EspressoApp
 * Prerequisite:fcn shall be called every 100ms
 * Parameters:
 * Return:
 *****************************************************************************/
void service_classic_mode(acInput_status_t swBrew, acInput_status_t swSteam)
{
  float svr_duration_secs;
  uint32_t brew_time_msecs;
  uint8_t  log_text_arr[SVC_LOG_LEN]={0};
  static bool is_app_initialized = false;  /* persists across calls */

  if (!is_app_initialized) {
    /* Code to run only once */
    #if(NRF_LOG_ENABLED == 1)
      sprintf((char*)log_text_arr,"%s;%08d;Espresso Machine enters into ::CLASSIC MODE::;",
                        TAG_SYS_MSG,
                        service_tick*100);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH();

      sprintf((char *)log_text_arr,
        "%s;System_Time_Miliseconds;Brew_time_Miliseconds;Boiler_Target_DegC;Boiler_Temp_DegC;Heating_Power;Pump_Power",
              TAG_SYS_FORMAT);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH();
    #endif
    is_app_initialized = true;
  }

  service_tick++;
  if( !(service_tick % SERVICE_MONITOR_TICK))
  {
    /* Get Boiler Temperature
       Get Target Temperature
       run Temperature Controller */
    boiler_temp_degC=(float)f_getBoilerTemperature();
    boiler_target_temp_degC = (float)g_Espresso_user_config_s.boilerTempSetpointDegC;
    Classic_data_s.heatingPwr = (uint16_t)temp_ctrl_update((espresso_user_config_t *)&g_Espresso_user_config_s);
    #if SERVICE_HEAT_ACTION_EN == 1
      boiler_ssr_pwr_update(Classic_data_s.heatingPwr);
    #endif

    /*Print: Time Stamp + Brew Time + Boiler Temperature + HeatPwr + PumpPwr. Delimeter symbol (;)*/
    #if(NRF_LOG_ENABLED == 1)
      brew_time_msecs = Classic_data_s.is_active ?
          (service_tick - Classic_data_s.svcStartT) * SERVICE_BASE_TIME_MSECS : 0U;
      sprintf((char *)log_text_arr,"%s;%08d;%08d;%.1f;%.2f;%04d;%04d;",
                        TAG_SYS_MONITOR,
                        service_tick*100,
                        brew_time_msecs,
                        boiler_target_temp_degC,
                        boiler_temp_degC,
                        Classic_data_s.heatingPwr,
                        app_pump_pwr);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH();
    #endif
  }else{}
  /*Recovery boost (Ki x2): return to Ki x1 once boiler reaches target. */
  monitor_i_recovery(&Classic_data_s.iBoost);

  switch(Espresso_service_status_s.sRunning)
  {
    case CLASSIC_IDLE:
        /*SWITCH Activation: Brew*/
        if(swBrew == AC_SWITCH_ASSERTED && !Classic_data_s.max_time_reached)
        {
          /*Save: Strating time & Reset extraction time*/
          Classic_data_s.svcStartT = service_tick;
          Classic_data_s.is_active = true;
          g_Espresso_user_config_s.extractionTimeMsecs = 0U;
          /*ACTION: Ki x pidIboostTerm (overrides any pending recovery boost)*/
          start_i_boost(&Classic_data_s.iBoost, g_Espresso_user_config_s.pidIboostTerm);
          /*ACTION: Solenoid valve ON*/
          solenoid_ssr_on();
          /*ACTION: Pump ON */
          Classic_data_s.pumpPwr = PUMP_PWR_ON;
          app_pump_pwr = Classic_data_s.pumpPwr;
          #if SERVICE_PUMP_ACTION_EN == 1
            pump_ssr_pwr_update(Classic_data_s.pumpPwr);
          #endif
          /*STATE JUMP: Mode1*/
          Espresso_service_status_s.sRunning= CLASSIC_MODE_1;
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Pulling a shot of espresso Start_time(ms)::;",
                              TAG_CLAS_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
          #endif
        }else{
          if (swBrew == AC_SWITCH_DEASSERTED) {
            Classic_data_s.max_time_reached = false;
          }
        }
        /*SWITCH Activation: Steam*/
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          /*ACTION: Setting new target temperatire for Steam generation*/
          apply_boiler_setpoint(SET_POINT_STEAM);
          /*STATE JUMP: Mode2A*/
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
          app_pump_pwr = Classic_data_s.pumpPwr;
          #if SERVICE_PUMP_ACTION_EN == 1
            pump_ssr_pwr_update(Classic_data_s.pumpPwr);
          #endif
          start_i_recovery(&Classic_data_s.iBoost);
          solenoid_ssr_off();
          Classic_data_s.max_time_reached = true;
          Classic_data_s.is_active = false;
          g_Espresso_user_config_s.extractionTimeMsecs =
              (service_tick - Classic_data_s.svcStartT) * SERVICE_BASE_TIME_MSECS;
          break;
        }
        /*SWITCH Deactivation: Brew*/
        if(swBrew == AC_SWITCH_ASSERTED )
        {}else{
          /*STATE JUMP: Mode_Max*/
          Espresso_service_status_s.sRunning = CLASSIC_IDLE;
          /*ACTION: Pump OFF */
          Classic_data_s.pumpPwr = PUMP_PWR_OFF;
          app_pump_pwr = Classic_data_s.pumpPwr;
          #if SERVICE_PUMP_ACTION_EN == 1
            pump_ssr_pwr_update(Classic_data_s.pumpPwr);
          #endif
          /*ACTION: Recovery boost Ki x2 until boiler reaches target*/
          start_i_recovery(&Classic_data_s.iBoost);
          Classic_data_s.is_active = false;
          /*ACTION: Solenoid OFF */
          solenoid_ssr_off();
          g_Espresso_user_config_s.extractionTimeMsecs =
              (service_tick - Classic_data_s.svcStartT) * SERVICE_BASE_TIME_MSECS;
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;msg::Service Stop_time(ms)::;",
                              TAG_CLAS_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
            svr_duration_secs = (float)g_Espresso_user_config_s.extractionTimeMsecs;
            svr_duration_secs = svr_duration_secs/1000.0f;

            sprintf((char *)log_text_arr,"%s;%08d;%.2f s;msg::Espresso shot Duration_time(s)::;",
                              TAG_CLAS_MODE,
                              service_tick*100,
                              svr_duration_secs);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
          #endif
        }
        /*BUTTON Activation: Steam*/
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          /*ACTION: Solenoid OFF */
          solenoid_ssr_off();
          /*ACTION: Setting new target temperatire for Steam generation*/
          apply_boiler_setpoint(SET_POINT_STEAM);
          /*STATE JUMP: Mode2A*/
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
      /*SWITCH Activation: Brew*/
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        Classic_data_s.pumpPwr = PUMP_PWR_ON;
        app_pump_pwr = Classic_data_s.pumpPwr;
        #if SERVICE_PUMP_ACTION_EN == 1
            pump_ssr_pwr_update(Classic_data_s.pumpPwr);
        #endif
        /*STATE JUMP: Mode2B*/
        Espresso_service_status_s.sRunning= CLASSIC_MODE_3;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Pump On + Solenoid Shut::;",
                            TAG_CLAS_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
        #endif
      }else{}
      /*SWITCH Activation: Steam*/
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        /*ACTION: shut pump down.*/
        Classic_data_s.pumpPwr = PUMP_PWR_OFF;
        app_pump_pwr = Classic_data_s.pumpPwr;
        #if SERVICE_PUMP_ACTION_EN == 1
            pump_ssr_pwr_update(Classic_data_s.pumpPwr);
        #endif
        /*ACTION: Setting target temperature back to brew setpoint*/
        apply_boiler_setpoint(SET_POINT_BREW);
        /*STATE JUMP: idle*/
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
      /*SWITCH Activation: Brew*/
      if(swBrew == AC_SWITCH_ASSERTED )
      {}else{
        /*ACTION: shut pump down.*/
        Classic_data_s.pumpPwr = PUMP_PWR_OFF;
        app_pump_pwr = Classic_data_s.pumpPwr;
        #if SERVICE_PUMP_ACTION_EN == 1
            pump_ssr_pwr_update(Classic_data_s.pumpPwr);
        #endif
        /*ACTION: entered from brew (MODE_1) -> start recovery boost*/
        if (Classic_data_s.iBoost.isPhase1) {
          start_i_recovery(&Classic_data_s.iBoost);
        } else {}
        /*STATE JUMP [easy]: To idle, then this stage will take care of the switch state*/
        Espresso_service_status_s.sRunning= CLASSIC_IDLE;
        /*STATE JUMP: Mode2B*/
        /*Espresso_service_status_s.sRunning= CLASSIC_MODE_2;*/
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;msg::Steam purge Stop_time(ms)::;",
                            TAG_CLAS_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
        #endif
      }
      /*SWITCH Activation: Steam*/
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        /*ACTION: Setting target temperature back to brew setpoint*/
        apply_boiler_setpoint(SET_POINT_BREW);
        /*STATE JUMP [easy]: To idle, then this stage will take care of the switch state*/
        Espresso_service_status_s.sRunning= CLASSIC_IDLE;
        /*STATE JUMP: Mode1A*/
        /*Espresso_service_status_s.sRunning= CLASSIC_MODE_1;*/
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
      Classic_data_s.is_active = false;
    break;
  }
}

/*****************************************************************************
 * Function: 	fcn_service_ProfileMode
 * Prerequisite:fcn shall be called every 100ms
 * Parameters:
 * Return:
 *****************************************************************************/
void service_profile_mode(acInput_status_t swBrew, acInput_status_t swSteam)
{
  float svr_duration_secs;
  uint32_t brew_time_msecs;
  uint8_t  log_text_arr[SVC_LOG_LEN]={0};
  static bool is_app_initialized = false;  /* persists across calls */

  if (!is_app_initialized) {
    /* Code to run only once */
    Profile_data_s.svcStartT = 0;
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

      sprintf((char *)log_text_arr,
        "%s;System_Time_Miliseconds;Brew_time_Miliseconds;Boiler_Target_DegC;Boiler_Temp_DegC;Heating_Power;Pump_Power",
              TAG_SYS_FORMAT);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH();
    #endif
    is_app_initialized = true;
  }

  service_tick++;
  if( !(service_tick % SERVICE_MONITOR_TICK))
  {
    /* Get Boiler Temperature
       Get Target Temperature
       run Temperature Controller */
    boiler_temp_degC=(float)f_getBoilerTemperature();
    boiler_target_temp_degC = (float)g_Espresso_user_config_s.boilerTempSetpointDegC;
    Profile_data_s.heatingPwr = (uint16_t)temp_ctrl_update((espresso_user_config_t *)&g_Espresso_user_config_s);
    #if SERVICE_HEAT_ACTION_EN == 1
      boiler_ssr_pwr_update(Profile_data_s.heatingPwr);
    #endif
    /*Print: Time Stamp + Brew Time + Boiler Temperature + HeatPwr + PumpPwr. Delimeter symbol (;)*/
    #if(NRF_LOG_ENABLED == 1)
      brew_time_msecs = Profile_data_s.is_active ?
          (service_tick - Profile_data_s.svcStartT) * SERVICE_BASE_TIME_MSECS : 0U;
      sprintf((char *)log_text_arr,"%s;%08d;%08d;%.1f;%.2f;%04d;%04d;",
                        TAG_SYS_MONITOR,
                        service_tick*100,
                        brew_time_msecs,
                        boiler_target_temp_degC,
                        boiler_temp_degC,
                        Profile_data_s.heatingPwr,
                        Profile_data_s.pumpPwr);
      NRF_LOG_RAW_INFO("%s\n",log_text_arr);
      NRF_LOG_FLUSH();
    #endif
  }else{}
  /*Recovery boost (Ki x2): return to Ki x1 once boiler reaches target. */
  monitor_i_recovery(&Profile_data_s.iBoost);

  switch(Profile_service_status_s.sRunning)
  {
    case PROFILE_IDLE:
        /*SWITCH Activation: Brew*/
        if(swBrew == AC_SWITCH_ASSERTED )
        {
          /*ACTION: Ki x1, linear ramp to x pidIboostTerm over pre-infusion + infusion tabs*/
          Profile_data_s.kiFactor     = 1.0f;
          Profile_data_s.kiStepFactor = (g_Espresso_user_config_s.pidIboostTerm - 1.0f)
                                      / (float)(PROFILE_BOOST_STAGES * Profile_data_s.noTabs);
          start_i_boost(&Profile_data_s.iBoost, Profile_data_s.kiFactor);
          /*LOAD: Data for Profiler state*/
          Profile_data_s.tickTabTarget = (uint16_t)(1000.0f*(g_Espresso_user_config_s.profPreInfuseTmr+0.999999f));
          Profile_data_s.tickTabTarget = (Profile_data_s.tickTabTarget / SERVICE_BASE_TIME_MSECS) / Profile_data_s.noTabs;
          Profile_data_s.tick          = Profile_data_s.tickTabTarget;
          /*Let's calculate delta power*/
          Profile_data_s.delta_pumpPwr= (int16_t)((g_Espresso_user_config_s.profPreInfusePwr
                                            - 30.0f) * 10.0f);

          Profile_data_s.base_pumpPwr=(uint16_t)(30 * 10.0f);
          /*Delta will always be positive -> Growth table will be use*/
          Profile_data_s.ptrTab=&Profile_data_s.exp_growth_arr[0];
          /*Load # of tab to go through*/
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
          Profile_data_s.pumpPwr   = (uint16_t)(((float)Profile_data_s.delta_pumpPwr)*(*Profile_data_s.ptrTab));
          Profile_data_s.pumpPwr+= Profile_data_s.base_pumpPwr;
          /*ACTION: Solenoid valve ON*/
          solenoid_ssr_on();
          /*ACTION: Pump ON */
          #if SERVICE_PUMP_ACTION_EN == 1
            pump_ssr_pwr_update(Profile_data_s.pumpPwr);
          #endif
          /*Save: Strating time & Reset Extraction-Time */
          Profile_data_s.svcStartT = service_tick;
          Profile_data_s.is_active = true;
          g_Espresso_user_config_s.extractionTimeMsecs = 0U;
          /*STATE JUMP: Profiler Move*/
          Profile_service_status_s.sRunning= PROFILE_MODE_RAMP_STEP;
          Profile_service_status_s.sNext = PROFILE_MODE_INFUSE;
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;--Pulling a shot of espresso--;",
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
            sprintf((char *)log_text_arr,"%s;%08d;:--Pre infusion Start_time(ms)--;",
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
          #endif
        }else{}
        /*SWITCH Activation: Steam*/
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          /*ACTION: Setting new target temperatire for Steam generation*/
          apply_boiler_setpoint(SET_POINT_STEAM);
          /*STATE JUMP: Mode2A*/
          Profile_service_status_s.sRunning = PROFILE_MODE_STEAM;
          #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;--Steam generation Start_time(ms)--;",
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
          #endif
        }else{}
    break;

    case PROFILE_MODE_RAMP_STEP:
      /* H5: enforce maximum brew duration — auto-stop after 120 s */
      if ((service_tick - Profile_data_s.svcStartT) >= MAX_BREW_TICKS) {
        Profile_data_s.max_time_reached = true;   //Profile time reach maximum duration time, flag -> false.
        Profile_data_s.is_stopped = false;
        Profile_data_s.profile_ended = false;
        Profile_service_status_s.sRunning = PROFILE_MODE_STOP;   // not IDLE
        break;
      }
      /*SWITCH deactivation: Brew*/
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        Profile_data_s.tick--;
        /*End of current profile step time?*/
        if(Profile_data_s.tick==0)
        {
          /*Move to next tab within table*/
          Profile_data_s.tabCnt--;
          /*Load timer.*/
          Profile_data_s.tick = Profile_data_s.tickTabTarget;
          /*LOAD: next Data for Profiler*/
          /*Let's calculate new step power for the pump*/
          Profile_data_s.ptrTab++;
          Profile_data_s.pumpPwr = (uint16_t)(((float)Profile_data_s.delta_pumpPwr)*(*Profile_data_s.ptrTab));
          Profile_data_s.pumpPwr+= Profile_data_s.base_pumpPwr;
          /*ACTION: Ki ramp step during pre-infusion/infusion; tapering keeps fixed x2*/
          if (Profile_service_status_s.sNext != PROFILE_MODE_STOP) {
            step_profile_integral_boost();
          } else {}
          /*ACTION: Introduce new power value.*/
          #if SERVICE_PUMP_ACTION_EN == 1
            pump_ssr_pwr_update(Profile_data_s.pumpPwr);
          #endif
        }else{}
        if(Profile_data_s.tabCnt==0)
        {
          if(Profile_service_status_s.sNext == PROFILE_MODE_STOP)
          {
            /* Natural end: all 3 profile stages completed on schedule */
            Profile_data_s.is_stopped       = false;
            Profile_data_s.profile_ended    = true;
            Profile_data_s.max_time_reached = false;
          }else{}
          Profile_service_status_s.sRunning = Profile_service_status_s.sNext;
        }else{}
      }else{
        Profile_service_status_s.sRunning = PROFILE_MODE_STOP;
        Profile_data_s.is_stopped = false;
        Profile_data_s.profile_ended = true;
        Profile_data_s.max_time_reached = false;
      } 
      /*SWITCH Activation: Steam -> IGNORED*/
    break;

    case PROFILE_MODE_INFUSE:
      /* H5: enforce maximum brew duration — auto-stop after 120 s */
      if ((service_tick - Profile_data_s.svcStartT) >= MAX_BREW_TICKS) {
        Profile_data_s.max_time_reached = true;   //Profile time reach maximum duration time, flag -> false.
        Profile_data_s.is_stopped = false;
        Profile_data_s.profile_ended = false;
        Profile_service_status_s.sRunning = PROFILE_MODE_STOP;   // not IDLE
        break;
      }
      /*SWITCH deactivation: Brew*/
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        /*LOAD: Data for Profiler state*/
        Profile_data_s.tickTabTarget = (uint16_t)(1000.0f*(g_Espresso_user_config_s.profInfuseTmr+0.999999f));
        Profile_data_s.tickTabTarget = (Profile_data_s.tickTabTarget / SERVICE_BASE_TIME_MSECS) / Profile_data_s.noTabs;
        Profile_data_s.tick          = Profile_data_s.tickTabTarget;
        /*Let's calculate delta power*/
        Profile_data_s.delta_pumpPwr= (int16_t)((g_Espresso_user_config_s.profInfusePwr
                                          - g_Espresso_user_config_s.profPreInfusePwr) * 10.0f);
        /*check oid delta pumpPwr is possitve or negative*/
        if( Profile_data_s.delta_pumpPwr>0)
        {
          /*Save base pumpPwr ->previous pumpPwr step*/
          Profile_data_s.base_pumpPwr = (uint16_t)(g_Espresso_user_config_s.profPreInfusePwr * 10.0f);
          /*positive Delta -> Growth table will be use*/
          Profile_data_s.ptrTab=&Profile_data_s.exp_growth_arr[0];
          /*Load # of tab to go through*/
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
        }else{
          Profile_data_s.delta_pumpPwr *= -1;
          /*Save base pumpPwr ->target pumpPwr step*/
          Profile_data_s.base_pumpPwr = (uint16_t)(g_Espresso_user_config_s.profTaperingPwr * 10.0f);
          /*negative Delta -> Decay table will be use*/
          Profile_data_s.ptrTab=&Profile_data_s.exp_decay_arr[0];
          /*Load # of tab to go through*/
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
        }
        Profile_data_s.pumpPwr  = (uint16_t)(((float)Profile_data_s.delta_pumpPwr)*(*Profile_data_s.ptrTab));
        Profile_data_s.pumpPwr += Profile_data_s.base_pumpPwr;
        /*ACTION: Pump ON */
        #if SERVICE_PUMP_ACTION_EN == 1
          pump_ssr_pwr_update(Profile_data_s.pumpPwr);
        #endif
        /*STATE JUMP: Profiler Move*/
        Profile_service_status_s.sRunning= PROFILE_MODE_RAMP_STEP;
        Profile_service_status_s.sNext = PROFILE_MODE_DECLINE;
        #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;--Infusion stage Start_time(ms)--;",
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
        #endif
      }else{
        Profile_service_status_s.sRunning = PROFILE_MODE_STOP;
        Profile_data_s.is_stopped = false;
        Profile_data_s.profile_ended = true;
        Profile_data_s.max_time_reached = false;
      }
      /*SWITCH Activation: Steam -> IGNORED*/
    break;

    case PROFILE_MODE_DECLINE:
      /* H5: enforce maximum brew duration — auto-stop after 120 s */
      if ((service_tick - Profile_data_s.svcStartT) >= MAX_BREW_TICKS) {
        Profile_data_s.max_time_reached = true;   //Profile time reach maximum duration time, flag -> false.
        Profile_data_s.is_stopped = false;
        Profile_data_s.profile_ended = false;
        Profile_service_status_s.sRunning = PROFILE_MODE_STOP;   // not IDLE
        break;
      }
      /*SWITCH deactivation: Brew*/
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        /*ACTION: Tapering -> recovery boost Ki x2 until boiler reaches target*/
        start_i_recovery(&Profile_data_s.iBoost);
        /*LOAD: Data for Profiler state*/
        Profile_data_s.tickTabTarget = (uint16_t)(1000.0f*(g_Espresso_user_config_s.profTaperingTmr+0.999999f));
        Profile_data_s.tickTabTarget = (Profile_data_s.tickTabTarget / SERVICE_BASE_TIME_MSECS) / Profile_data_s.noTabs;
        Profile_data_s.tick          = Profile_data_s.tickTabTarget;
        /*Let's calculate delta power*/
        Profile_data_s.delta_pumpPwr= (int16_t)((g_Espresso_user_config_s.profTaperingPwr
                                          - g_Espresso_user_config_s.profInfusePwr) * 10.0f);
        /*check oid delta pumpPwr is possitve or negative*/
        if( Profile_data_s.delta_pumpPwr>0)
        {
          /*Save base pumpPwr ->previous pumpPwr step*/
          Profile_data_s.base_pumpPwr = (uint16_t)(g_Espresso_user_config_s.profInfusePwr * 10.0f);
          /*positive Delta -> Growth table will be use*/
          Profile_data_s.ptrTab=&Profile_data_s.exp_growth_arr[0];
          /*Load # of tab to go through*/
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
        }else{
          Profile_data_s.delta_pumpPwr *= -1;
          /*Save base pumpPwr ->target pumpPwr step*/
          Profile_data_s.base_pumpPwr = (uint16_t)(g_Espresso_user_config_s.profTaperingPwr * 10.0f);
          /*negative Delta -> Decay table will be use*/
          Profile_data_s.ptrTab=&Profile_data_s.exp_decay_arr[0];
          /*Load # of tab to go through*/
          Profile_data_s.tabCnt=Profile_data_s.noTabs;
        }
        Profile_data_s.pumpPwr  = (uint16_t)(((float)Profile_data_s.delta_pumpPwr)*(*Profile_data_s.ptrTab));
        Profile_data_s.pumpPwr += Profile_data_s.base_pumpPwr;
        /*ACTION: Pump ON */
        #if SERVICE_PUMP_ACTION_EN == 1
          pump_ssr_pwr_update(Profile_data_s.pumpPwr);
        #endif
        /*STATE JUMP: Profiler Move*/
        Profile_service_status_s.sRunning= PROFILE_MODE_RAMP_STEP;
        Profile_service_status_s.sNext = PROFILE_MODE_STOP;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;--Decline stage Start_time(ms)--;",
                            TAG_PROF_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
          NRF_LOG_FLUSH();
        #endif
      }else{
        Profile_service_status_s.sRunning = PROFILE_MODE_STOP;
        Profile_data_s.is_stopped = false;
        Profile_data_s.profile_ended = true;
        Profile_data_s.max_time_reached = false;
      }
      /*SWITCH Activation: Steam -> IGNORED*/
    break;

    case PROFILE_MODE_STOP:
      if(!Profile_data_s.is_stopped)
      {
        Profile_data_s.is_stopped = true;
        /*ACTION: shut pump down.*/
        app_pump_pwr = PUMP_PWR_OFF;
        Profile_data_s.pumpPwr = app_pump_pwr;
        #if SERVICE_PUMP_ACTION_EN == 1
          pump_ssr_pwr_update(app_pump_pwr);
        #endif
        /*ACTION: Solenoid OFF */
        solenoid_ssr_off();
        /*ACTION: Ki x2 (kept if already set by tapering, skipped if already recovered)*/
        start_i_recovery(&Profile_data_s.iBoost);
        g_Espresso_user_config_s.extractionTimeMsecs =
            (service_tick - Profile_data_s.svcStartT) * SERVICE_BASE_TIME_MSECS;
      }else{}

      if(Profile_data_s.profile_ended)
      {
        Profile_data_s.profile_ended = false;
        #if(NRF_LOG_ENABLED == 1)
            svr_duration_secs = (float)g_Espresso_user_config_s.extractionTimeMsecs;
            svr_duration_secs = svr_duration_secs/1000.0f;
            sprintf((char *)log_text_arr,"%s;%08d;--Espresso shot time = %.2f(s)--;",
                              TAG_PROF_MODE,
                              service_tick*100,
                              svr_duration_secs);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
            sprintf((char *)log_text_arr,"%s;--Profile service ended--;",
                              TAG_PROF_MODE);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
        #endif
      }else{}
      if(Profile_data_s.max_time_reached)
      {
        Profile_data_s.max_time_reached = false;
        #if(NRF_LOG_ENABLED == 1)
            svr_duration_secs = (float)g_Espresso_user_config_s.extractionTimeMsecs;
            svr_duration_secs = svr_duration_secs/1000.0f;
            sprintf((char *)log_text_arr,"%s;%08d;--Espresso shot time = %.2f(s)--;",
                              TAG_PROF_MODE,
                              service_tick*100,
                              svr_duration_secs);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
            sprintf((char *)log_text_arr,"%s;--Profile service ended--;",
                              TAG_PROF_MODE);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
        #endif
      }else{}


      /*SWITCH Activation: Brew*/
      if(swBrew == AC_SWITCH_ASSERTED )
      {}else{
        /*STATE JUMP: IDLE*/
        Profile_service_status_s.sRunning= PROFILE_IDLE;
        Profile_data_s.is_active = false;
        Profile_data_s.is_stopped = false;
        Profile_data_s.max_time_reached = false;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;--Ready to pull a new espresso shot--;",
                              TAG_PROF_MODE,
                              service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
          NRF_LOG_FLUSH();
        #endif
      }
      /*SWITCH Activation: Steam*/
      if(swSteam == AC_SWITCH_ASSERTED && swBrew == AC_SWITCH_ASSERTED )
      {
        /*Go to Mode 3.*/
      }else{}
    break;

    case PROFILE_MODE_STEAM:
      /*SWITCH Activation: Brew*/
      if(swBrew == AC_SWITCH_ASSERTED )
      {
        app_pump_pwr = PUMP_PWR_ON;
        Profile_data_s.pumpPwr = app_pump_pwr;
        #if SERVICE_PUMP_ACTION_EN == 1
          pump_ssr_pwr_update(app_pump_pwr);
        #endif
        /*STATE JUMP: Mode2B*/
        Profile_service_status_s.sRunning= PROFILE_MODE_STEAM_BREW;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;--Pump On + Solenoid Shut--;",
                            TAG_PROF_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
        #endif
      }else{}
      /*SWITCH Activation: Steam*/
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        /*ACTION: shut pump down.*/
        app_pump_pwr = PUMP_PWR_OFF;
        Profile_data_s.pumpPwr = app_pump_pwr;
        #if SERVICE_PUMP_ACTION_EN == 1
          pump_ssr_pwr_update(app_pump_pwr);
        #endif
        /*ACTION: Setting target temperature back to brew setpoint*/
        apply_boiler_setpoint(SET_POINT_BREW);
        /*STATE JUMP: idle*/
        Profile_service_status_s.sRunning= PROFILE_IDLE;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;--Steam Generation Stop_time(ms)--;",
                            TAG_PROF_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
          NRF_LOG_FLUSH();
        #endif
      }
    break;

    case PROFILE_MODE_STEAM_BREW:
      /*SWITCH Activation: Brew*/
      if(swBrew == AC_SWITCH_ASSERTED )
      {}else{
        /*ACTION: shut pump down.*/
        app_pump_pwr = PUMP_PWR_OFF;
        Profile_data_s.pumpPwr = app_pump_pwr;
        #if SERVICE_PUMP_ACTION_EN == 1
          pump_ssr_pwr_update(app_pump_pwr);
        #endif
        /*STATE JUMP: Mode2B*/
        Profile_service_status_s.sRunning= PROFILE_MODE_STEAM;
        #if(NRF_LOG_ENABLED == 1)
            sprintf((char *)log_text_arr,"%s;%08d;--Steam generation Start_time(ms)--;",
                              TAG_PROF_MODE,
                              service_tick*100);
            NRF_LOG_RAW_INFO("%s\n",log_text_arr);
            NRF_LOG_FLUSH();
        #endif
      }
      /*SWITCH Activation: Steam*/
      if(swSteam == AC_SWITCH_ASSERTED )
      {}else{
        /*ACTION: Setting target temperature back to brew setpoint*/
        apply_boiler_setpoint(SET_POINT_BREW);
        /*STATE JUMP: Mode1A*/
        Profile_service_status_s.sRunning= PROFILE_IDLE;
        #if(NRF_LOG_ENABLED == 1)
          sprintf((char *)log_text_arr,"%s;%08d;--Pulling a shot of espresso Start_time(ms)--;",
                            TAG_PROF_MODE,
                            service_tick*100);
          NRF_LOG_RAW_INFO("%s\n",log_text_arr);
        #endif
      }
    break;

    case PROFILE_MODE_MAX:
      Profile_service_status_s.sRunning= PROFILE_IDLE;
      Profile_data_s.is_active = false;
    break;
  }
}


/*****************************************************************************
 * Function: 	fcn_service_StepFunction
 * Prerequisite:fcn shall be called every 100ms
 * Parameters:
 * Return:
 *****************************************************************************/
void service_step_function(acInput_status_t swBrew, acInput_status_t swSteam)
{
  static bool is_stpfcn_initialized = false;  /* persists across calls */
  static bool is_stpfcn_heating = false;  /* persists across calls */

  if (!is_stpfcn_initialized) {
    /* Code to run only once */
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
        /*SWITCH Activation: Brew*/
        if(swBrew == AC_SWITCH_ASSERTED )
        {
          /*ACTION: Solenoid valve ON*/
          solenoid_ssr_on();
          /*ACTION: Pump ON */
          app_pump_pwr = PUMP_PWR_ON;
          #if SERVICE_PUMP_ACTION_EN == 1
            pump_ssr_pwr_update(app_pump_pwr);
          #endif
          /*STATE JUMP: Mode1*/
          Stepfcn_service_status_s.sRunning= SF_MODE_1;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::FILLING BOILER::");
            NRF_LOG_FLUSH();
          #endif
        }else{}
        /*SWITCH Activation: Steam*/
        if(swSteam == AC_SWITCH_ASSERTED )
        {
          /*STATE JUMP: Mode2A*/
          Stepfcn_service_status_s.sRunning = SF_MODE_2A;
          /*Set intial parameters before jumping into next mode*/
          app_timestamp_msecs=0;
          app_heat_pwr=0;
          /*REset stepCounter. this var counts for 5s to get initial temperature of the boiler*/
          /*before the step function stars*/
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
        /*SWITCH Deactivation: Brew*/
        if(swBrew == AC_SWITCH_ASSERTED )
        {}else{
          /*STATE JUMP: Mode_Max*/
          Stepfcn_service_status_s.sRunning= SF_MODE_MAX;
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("STP_FCN ::STOPPING FILLING BOILER::");
            NRF_LOG_FLUSH();
          #endif
        }
    break;

    case SF_MODE_2A:
        /*ACTION: NOT operation to [ tick_counter % 5 = 0 ]  */
        if( !(stpfcn_tick_cnt % (uint32_t)STPFCN_LOG_PRINT) )
        {
          /* Get Boiler Temperature
          and save it here => g_Espresso_user_config_s.boilerTempDegC.*/
          boiler_temp_degC=(float)f_getBoilerTemperature();
          /*g_Espresso_user_config_s.boilerTempDegC;*/
          /*Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)*/
          #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("STPFCN;%08d;%04d;" NRF_LOG_FLOAT_MARKER ";",
                            app_timestamp_msecs,
                            app_heat_pwr,
                            NRF_LOG_FLOAT(boiler_temp_degC));
              NRF_LOG_FLUSH();
          #endif
          /*Increment TimeStamp and StepCounter*/
          app_timestamp_msecs+=500;
        }else{}
        /*STATE JUMP: Mode2B*/
        if(stpfcn_tick_cnt >= (uint32_t)STPFCN_START_DELAY)
        {
          /*Reset counter*/
          stpfcn_tick_cnt=0;
          Stepfcn_service_status_s.sRunning= SF_MODE_2B;
        }
        /*ACTION:Increment tick Counter*/
        stpfcn_tick_cnt++;

        /*SWITCH Deactivation: Steam*/
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
          #if SERVICE_HEAT_ACTION_EN == 1
            boiler_ssr_pwr_update(app_heat_pwr);
          #endif
          is_stpfcn_heating = false;
          Stepfcn_service_status_s.sRunning = SF_MODE_MAX;
          break;
        }
        /*EXECUTION: Only one time*/
        if(!is_stpfcn_heating)
        {
          /*ACTION: Activating Heating Element to 100%*/
          app_heat_pwr=STPFCN_HEATING_PWR;
          #if SERVICE_HEAT_ACTION_EN == 1
            boiler_ssr_pwr_update(app_heat_pwr);
          #endif
          #if(NRF_LOG_ENABLED == 1)
            NRF_LOG_INFO("\nSTP_FCN ::START::\n");
            NRF_LOG_FLUSH();
          #endif
          is_stpfcn_heating = true;
        }else{}
        /*ACTION: NOT operation to [ tick_counter % 5 = 0 ]  */
        if( !(stpfcn_tick_cnt % (uint32_t)STPFCN_LOG_PRINT) )
        {
          /* Get Boiler Temperature
          and save it here => g_Espresso_user_config_s.steamTempDegC.*/
          spim_ReadRTDconverter();
          boiler_temp_degC=(float)f_getBoilerTemperature();
          /*boiler_temp_degC=g_Espresso_user_config_s.steamTempDegC;*/
          /*Print: Time Stamp + HeatPwr + Boiler Temperature. Delimeter symbol (;)*/
          #if(NRF_LOG_ENABLED == 1)
              NRF_LOG_INFO("STPFCN;%08d;%04d;" NRF_LOG_FLOAT_MARKER ";",
                            app_timestamp_msecs,
                            app_heat_pwr,
                            NRF_LOG_FLOAT(boiler_temp_degC));
              NRF_LOG_FLUSH();
          #endif
          /*Increment TimeStamp and StepCounter*/
          app_timestamp_msecs+=500;
        }else{}
        /*ACTION: increment counter*/
        stpfcn_tick_cnt++;

        /*SWITCH Deactivation: Steam*/
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
      /*ACTION: Pump OFF */
      app_pump_pwr = PUMP_PWR_OFF;
      #if SERVICE_PUMP_ACTION_EN == 1
        pump_ssr_pwr_update(app_pump_pwr);
      #endif
      /*ACTION: Solenoid OFF */
      solenoid_ssr_off();
      /*ACTION: Heat OFF */
      app_heat_pwr=PUMP_PWR_OFF;
      #if SERVICE_HEAT_ACTION_EN == 1
        boiler_ssr_pwr_update(app_heat_pwr);
      #endif
      /*Reset counter*/
      stpfcn_tick_cnt=0;
      Stepfcn_service_status_s.sRunning = SF_IDLE;
      #if(NRF_LOG_ENABLED == 1)
          NRF_LOG_INFO("STP_FCN ::STOP::");
          NRF_LOG_FLUSH();
     #endif
    break;

  }
}

/******************************************************************************
*
*		PRIVATE FUNCTIONS SECTION
*
******************************************************************************/

/*****************************************************************************
 * Function: 	start_i_boost
 * Description: Brew start: clear recovery phase and load the initial Ki factor.
 *****************************************************************************/
static void start_i_boost(i_boost_flags_t *ptr_flags, float factor)
{
  ptr_flags->isPhase1 = true;
  ptr_flags->isPhase2 = false;
  ptr_flags->isNormal = false;
  (void)temp_ctrl_scale_integral_gain(factor);
}

/*****************************************************************************
 * Function: 	start_i_recovery
 * Description: Brew end: Ki x2 unless already recovered.
 *              monitor_i_recovery() returns Ki to x1 once target is reached.
 *****************************************************************************/
static void start_i_recovery(i_boost_flags_t *ptr_flags)
{
  ptr_flags->isPhase1 = false;
  if (!ptr_flags->isNormal) {
    ptr_flags->isPhase2 = true;
    (void)temp_ctrl_scale_integral_gain(I_BOOST_RECOVERY_FACTOR);
  } else {}
}

/*****************************************************************************
 * Function: 	monitor_i_recovery
 * Description: Every tick: drop recovery boost to Ki x1 once boiler reaches target.
 *****************************************************************************/
static void monitor_i_recovery(i_boost_flags_t *ptr_flags)
{
  if ((ptr_flags->isPhase2) &&
      ((boiler_temp_degC + TEMP_CTRL_PHI2_THR) > boiler_target_temp_degC)) {
    ptr_flags->isPhase2 = false;
    ptr_flags->isNormal = true;
    (void)temp_ctrl_scale_integral_gain(1.0f);
  } else {}
}

/*****************************************************************************
 * Function: 	step_profile_integral_boost
 * Description: Advance Ki one linear step toward pidIboostTerm
 *              (pre-infusion + infusion, one step per tab rollover).
 *              BLE writes are blocked by the app while a shot is in progress.
 *****************************************************************************/
static void step_profile_integral_boost(void)
{
  Profile_data_s.kiFactor += Profile_data_s.kiStepFactor;
  if (Profile_data_s.kiFactor > g_Espresso_user_config_s.pidIboostTerm) {
    Profile_data_s.kiFactor = g_Espresso_user_config_s.pidIboostTerm;
  } else {}
  (void)temp_ctrl_scale_integral_gain(Profile_data_s.kiFactor);
}

/*****************************************************************************
 * Function: 	apply_boiler_setpoint
 * Description: Switch-driven setpoint change: load brew/steam temp, reset PID
 *              integral (M1 fix) and flag a BLE notify of 0x1403.
 *****************************************************************************/
static void apply_boiler_setpoint(tempCtrl_LoadSP_t setpoint)
{
  (void)temp_ctrl_set_boiler_setpoint(&g_Espresso_user_config_s, setpoint);
  is_setpoint_changed = true;
}

/*****************************************************************************
 * Function: 	get_switch_state
 * Description: Read the state of each switch and reported back
 * Return:      switches states
 *****************************************************************************/
