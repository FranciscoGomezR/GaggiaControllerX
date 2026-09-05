#ifndef ESPRESSOMACHINESERVICES_H__
#define ESPRESSOMACHINESERVICES_H__
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>

#include "ac_inputs_drv.h"
//#include "bluetooth_drv.h"
//#include "nrf_log.h"
//#include "nrf_log_ctrl.h"
//#include "nrf_log_default_backends.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
// 
//*****************************************************************************

#define FACTORY_DEFAULT_NO_NVM        0
#define TEST_VALUES_NO_NVM            1
#define USER_DATA_NVM                 2
#define ESPRESSO_CFG_DATA_SOURCE      USER_DATA_NVM

/* Developer/debug one-shot: erases NVM_PARAM_MEM_KEY + the rest of the user
 * config from the NVM chip on next boot. Set 1, flash, boot once to erase,
 * then set back to 0 and reflash. Never leave set to 1 in a release build. */
#define ESPRESSO_CFG_ERASE_NVM_KEY    0

#define SERVICE_PUMP_ACTION_EN        1
#define SERVICE_HEAT_ACTION_EN        1

/* ---------------------------------------------------------------------------
 * espresso_user_config_t Developer Test values.
 *
 * Single source of truth for Factory values to be used by espresso_user_config_t 
 * and to be stored into NVM first time MCU boots beacuse NVM_PARAM_MEM_KEY 
 * is not present
 * --------------------------------------------------------------------------- */
/* ---- Temperature setpoints (degC) ---- */
#define BOILER_SETPOINT_TEMP_TEST_DEGC  95.5f
#define BREW_TEMP_TEST_DEGC             95.0f
#define STEAM_TEMP_TEST_DEGC            130.0f

/* ---- Brew profile power (pwr %) ---- */
#define PROF_PREINFUSE_PWR_TEST_PWR     80.0f
#define PROF_INFUSE_PWR_TEST_PWR        100.0f
#define PROF_TAPERING_PWR_TEST_PWR      85.0f

/* ---- Brew profile timers (secs) ---- */
#define PROF_PREINFUSE_TMR_TEST_SECS    8.0f
#define PROF_INFUSE_TMR_TEST_SECS       10.0f
#define PROF_TAPERING_TMR_TEST_SECS     10.0f

/* ---- PID gains ---- */
#define PID_P_TERM_TEST                 9.52156f
#define PID_I_TERM_TEST                 0.3f
#define PID_I_MAX_TERM_TEST             100.0f
#define PID_D_TERM_TEST                 0.0f
#define PID_P_BOOST_TERM_TEST           1.0f
#define PID_I_BOOST_TERM_TEST           6.5f

/* ---------------------------------------------------------------------------
 * espresso_user_config_t Factory values.
 *
 * Single source of truth for Factory values to be used by espresso_user_config_t 
 * and to be stored into NVM first time MCU boots beacuse NVM_PARAM_MEM_KEY 
 * is not present
 * --------------------------------------------------------------------------- */
/* ---- Temperature setpoints (degC) ---- */
#define BOILER_SETPOINT_TEMP_FACTORY_DEFAULT_DEGC  95.5f
#define BREW_TEMP_FACTORY_DEFAULT_DEGC             95.0f
#define STEAM_TEMP_FACTORY_DEFAULT_DEGC            125.0f

/* ---- Brew profile power (pwr %) ---- */
#define PROF_PREINFUSE_PWR_FACTORY_DEFAULT_PWR     75.0f
#define PROF_INFUSE_PWR_FACTORY_DEFAULT_PWR        100.0f
#define PROF_TAPERING_PWR_FACTORY_DEFAULT_PWR      85.0f

/* ---- Brew profile timers (secs) ---- */
#define PROF_PREINFUSE_TMR_FACTORY_DEFAULT_SECS    8.0f
#define PROF_INFUSE_TMR_FACTORY_DEFAULT_SECS       8.0f
#define PROF_TAPERING_TMR_FACTORY_DEFAULT_SECS     8.0f

/* ---- PID gains ---- */
#define PID_P_TERM_FACTORY_DEFAULT                 9.5f
#define PID_I_TERM_FACTORY_DEFAULT                 0.3f
#define PID_I_MAX_TERM_FACTORY_DEFAULT             100.0f
#define PID_D_TERM_FACTORY_DEFAULT                 0.0f
#define PID_P_BOOST_TERM_FACTORY_DEFAULT           1.0f
#define PID_I_BOOST_TERM_FACTORY_DEFAULT           6.5f

/* ---------------------------------------------------------------------------
 * espresso_user_config_t field limits.
 *
 * Single source of truth for validate_float_in_range() callers
 * (StorageController.c validate_clamp_data(), bluetooth_drv.c BLE RX handlers).
 * StorageController.c values are canonical; bluetooth_drv.c was previously
 * hardcoding its own, drifted, literals for the same fields (STATUS TODO).
 * --------------------------------------------------------------------------- */
/* ---- Temperature setpoints (degC) ---- */
#define BOILER_SETPOINT_TEMP_MIN_DEGC      20.0f
#define BOILER_SETPOINT_TEMP_MAX_DEGC      135.0f
#define BOILER_SETPOINT_TEMP_DEFAULT_DEGC  BOILER_SETPOINT_TEMP_FACTORY_DEFAULT_DEGC

#define BREW_TEMP_MIN_DEGC                 20.0f
#define BREW_TEMP_MAX_DEGC                 110.0f
#define BREW_TEMP_DEFAULT_DEGC             BREW_TEMP_FACTORY_DEFAULT_DEGC

#define STEAM_TEMP_MIN_DEGC                100.0f
#define STEAM_TEMP_MAX_DEGC                135.0f
#define STEAM_TEMP_DEFAULT_DEGC            STEAM_TEMP_FACTORY_DEFAULT_DEGC

/* ---- Brew profile power (pwr %) ---- */
#define PROF_PREINFUSE_PWR_MIN_PWR         0.0f
#define PROF_PREINFUSE_PWR_MAX_PWR         100.0f
#define PROF_PREINFUSE_PWR_DEFAULT_PWR     PROF_PREINFUSE_PWR_FACTORY_DEFAULT_PWR

#define PROF_INFUSE_PWR_MIN_PWR            0.0f
#define PROF_INFUSE_PWR_MAX_PWR            100.0f
#define PROF_INFUSE_PWR_DEFAULT_PWR        PROF_INFUSE_PWR_FACTORY_DEFAULT_PWR

#define PROF_TAPERING_PWR_MIN_PWR          0.0f
#define PROF_TAPERING_PWR_MAX_PWR          100.0f
#define PROF_TAPERING_PWR_DEFAULT_PWR      PROF_TAPERING_PWR_FACTORY_DEFAULT_PWR

/* ---- Brew profile timers (secs) ---- */
#define PROF_PREINFUSE_TMR_MIN_SECS        0.0f
#define PROF_PREINFUSE_TMR_MAX_SECS        30.0f
#define PROF_PREINFUSE_TMR_DEFAULT_SECS    PROF_PREINFUSE_TMR_FACTORY_DEFAULT_SECS

#define PROF_INFUSE_TMR_MIN_SECS           0.0f
#define PROF_INFUSE_TMR_MAX_SECS           30.0f
#define PROF_INFUSE_TMR_DEFAULT_SECS       PROF_INFUSE_TMR_FACTORY_DEFAULT_SECS

#define PROF_TAPERING_TMR_MIN_SECS         0.0f
#define PROF_TAPERING_TMR_MAX_SECS         30.0f
#define PROF_TAPERING_TMR_DEFAULT_SECS     PROF_TAPERING_TMR_FACTORY_DEFAULT_SECS

/* ---- PID gains ---- */
#define PID_P_TERM_MIN                     0.0f
#define PID_P_TERM_MAX                     50.0f
#define PID_P_TERM_DEFAULT                 PID_P_TERM_FACTORY_DEFAULT

#define PID_I_TERM_MIN                     0.0f
#define PID_I_TERM_MAX                     30.0f
#define PID_I_TERM_DEFAULT                 PID_I_TERM_FACTORY_DEFAULT

#define PID_I_MAX_TERM_MIN                 0.0f
#define PID_I_MAX_TERM_MAX                 500.0f
#define PID_I_MAX_TERM_DEFAULT             PID_I_MAX_TERM_FACTORY_DEFAULT

#define PID_D_TERM_MIN                     0.0f
#define PID_D_TERM_MAX                     10.0f
#define PID_D_TERM_DEFAULT                 PID_D_TERM_FACTORY_DEFAULT

#define PID_P_BOOST_TERM_MIN               0.0f
#define PID_P_BOOST_TERM_MAX               10.0f
#define PID_P_BOOST_TERM_DEFAULT           PID_P_BOOST_TERM_FACTORY_DEFAULT

#define PID_I_BOOST_TERM_MIN               0.0f
#define PID_I_BOOST_TERM_MAX               10.0f
#define PID_I_BOOST_TERM_DEFAULT           PID_I_BOOST_TERM_FACTORY_DEFAULT

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
  typedef struct{
    uint32_t nvmWcycles;
    uint32_t nvmKey;

    float boilerTempDegC;
    float boilerTempSetpointDegC;
    float brewTempDegC;
    float steamTempDegC;

    float profPreInfusePwr;
    float profPreInfuseTmr;
    float profInfusePwr;
    float profInfuseTmr;
    float profTaperingPwr;
    float profTaperingTmr;

    float pidPTerm;
    float pidITerm;
    float pidImaxTerm;
    float pidDTerm;
    float pidPboostTerm;
    float pidIboostTerm;
   }espresso_user_config_t;
   //Format
   //   12 floats
   //   2 uint32_t
   // Equal to 56 bytes

typedef enum {
  ESPRESSO_MODE__TUNE = 0,
  ESPRESSO_MODE__MANUAL,
  ESPRESSO_MODE__AUTOMATIC
} machine_mode_t;

//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
extern volatile espresso_user_config_t g_Espresso_user_config_s;
static uint32_t g_operation_mode = ESPRESSO_MODE__MANUAL;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
/*
Default:
  - Run Boiler Temp. Controller.

  No switch     = idle
  Brew switch   = Mode 1
  Steam Switch  = Mode 2
  Both swtiches = Mode 3

Mode1:
Active Elements:
  - 1st Solenoid vale 
  - 2nd Pump (100ms after)(contonously)
Goal:
  - pull a shot of espresso
Description:
  - Forces water from the reservoir tank into the boiler and then into the group head;
    Water is at target Temperature (The electric resistors ARE ACTIVE)
 
Mode2:
Active Elements:
  - Electric resistors
Goal:
  - Generate steam
Description:
  - 

Mode3:
Active Elements:
  - Electric resistors
  - Pump
Goal:
  - circulate hot water through the group head without activarting the solenoid;
Description:
  - 
*/
void service_classic_mode(acInput_status_t swBrew, acInput_status_t swSteam);

/*
Default:
  - Run Boiler Temp. Controller.

  Brew switch   = Preinfuse -> Profiler -> Infuse -> Profiler -> Decline -> Profiler -> Halt
  Steam Switch  = Mode 2
  Both swtiches = Mode 3

Mode1:
Active Elements:
  - 1st Solenoid vale 
  - 2nd Pump (100ms after)(contonously)
Goal:
  - pull a shot of espresso
Description:
  - Forces water from the reservoir tank into the boiler and then into the group head;
    Water is at target Temperature (The electric resistors ARE ACTIVE)
 
Mode2:
Goal: Same behaviour as classic mode
  - 
Mode3:
Goal: Same behaviour as classic mode
  - 
*/
void service_profile_mode(acInput_status_t swBrew, acInput_status_t swSteam);

/*
    Brew switch   = Mode 1
    Steam Switch  = Mode 2
Mode1:
Active Elements:
  - 1st Solenoid vale 
  - 2nd Pump (500ms after)(contonously)
Goal:
  - Fill the boiler with water
Description:
  - Forces water from the reservoir tank into the boiler and then into the group head;
    Water is not heated (The electric resistors are not active)

Mode2:
Active Elements:
  - Electric resistors
Goal:
  - Run Step Function
  - Print boiler temp into COM port (500ms)
Description:
  - Start printing boiler temperature every 500ms.
  - Wait for 5 seconds before activating electric resistors.
  - activate resistors at full power
  - User shall observe the boiler temperature during this mode;
    machine can be switch off by the safety thermostat at ~167 C
  - https://www.drtradingshop.nl/a-42819926/onderdelen-gaggia-classic/veiligheidsthermostaat-1670c/#description
*/
void service_step_function(acInput_status_t swBrew, acInput_status_t swSteam);

#endif // ESPRESSOMACHINESERVICES_H__