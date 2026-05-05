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

#define LOAD_USERDATA_FROM_NVM_EN     0
#define ALLOW_USERDATA_WR_NVM__EN     1
#define SET_TEST_USERDATA_EN          1

#define SERVICE_PUMP_ACTION_EN        1
#define SERVICE_HEAT_ACTION_EN        1

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
    float pidIboostTerm;
    float pidImaxTerm;
    bool  pidIwindupTerm;
    float pidDTerm;

    float pidDlpfTerm;  //to Be Deleted
    float pidGainTerm;  //to Be Deleted
   }espresso_user_config_t;
   //Old Format
   //   14 floats
   //   2 uint32_t
   //   1  byte
   // Equal to 65 bytes

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
void fcn_service_ClassicMode(acInput_status_t swBrew, acInput_status_t swSteam);

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
void fcn_service_ProfileMode(acInput_status_t swBrew, acInput_status_t swSteam);

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
void fcn_service_StepFunction(acInput_status_t swBrew, acInput_status_t swSteam);

#endif // ESPRESSOMACHINESERVICES_H__