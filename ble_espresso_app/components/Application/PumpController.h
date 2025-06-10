
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include  "x01_StateMachineControls.h"


//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
#define PUMP_RAMPUP_T_MS_DEFAULT          3000
#define PUMP_RAMPDOWN_T_MS_DEFAULT        1500

#define PUMP_PREINFUSSIONPWR_DEFAULT      300
#define PUMP_PREINFUSSION_T_MS_DEFAULT    5000

#define PUMP_BREWPWR_DEFAULT              1000
#define PUMP_PEAKPRESSURE_T_MS_DEFAULT    500


#define PUMP_DECLINING_PWR_DEFAULT        750
#define PUMP_DECLINING_T_MS_DEFAULT       5000

#define PUMP_BASETIME_T_MS                250

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
 typedef enum {
  PUMPCTRL_INIT_OK = 0,
  PUMPCTRL_INIT_ERROR,
} pumpCtrl_status_t;

  typedef struct
  {
      StateMachineCtrl_Struct smPump;
      uint16_t PumpPower;
      uint16_t PumpBrePwr;
      uint16_t preInfussionPwr;
      uint16_t decliningPressurePwr;
      uint16_t preInfussionT_ms;
      uint16_t peakPressureT_ms;
      uint16_t decliningPressureT_ms;
      uint16_t PumpRampUpT_ms;
      uint16_t PumpRampDownT_ms;
  }StateMachinePump_Struct;
//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
extern StateMachinePump_Struct smPumpCtrl;

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
pumpCtrl_status_t fcn_initPumpController(void);
void fcn_PumpStateDriver(void);
void fcn_StartBrew(void);
void fcn_CancelBrew(void);