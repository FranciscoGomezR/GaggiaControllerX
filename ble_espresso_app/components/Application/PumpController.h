
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include "nrf.h"
#include  "x01_StateMachineControls.h"
#include "BLEspressoServices.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
#define PUMP_RAMPUP_T_MS_DEFAULT          2500
#define PUMP_RAMPDOWN_T_MS_DEFAULT        2500

/*  STEP  1 */
/*  Power is fixed point one decimal; from 0000 to 1000 (0.0 to 100.0)  */
#define PUMP_PREINFUSSIONPWR_DEFAULT      350
#define PUMP_PREINFUSSION_T_MS_DEFAULT    6000
/*  STEP  2 */
/*  Power is fixed point one decimal; from 0000 to 1000 (0.0 to 100.0)  */
#define PUMP_BREWPWR_DEFAULT              1000
#define PUMP_PEAKPRESSURE_T_MS_DEFAULT    5000
/*  STEP  3 */
/*  Power is fixed point one decimal; from 0000 to 1000 (0.0 to 100.0)  */
#define PUMP_DECLINING_PWR_DEFAULT        800
#define PUMP_DECLINING_T_MS_DEFAULT       6000
/*  STEP  4 */
/* Shutting the pump down Adn open the valve  */

#define PUMP_BASETIME_T_MS                250

/*
1000 ++-------+--------+-------PWR------PWR-------+--------+--------+-------++
     |                          *        *                                   |
0900 ++                         *        *                                  ++
     |                          *        *                                   |
0800 ++                         *        *                                  ++
     |                          *        *       PWR      PWR                |
0700 ++                         *        *        *        *                ++
     |                          *        *        *        *                 |
0600 ++                         *        *        *        *                ++
     |                          *        *        *        *                 |
0500 ++                         *        *        *        *                ++
     |                          *        *        *        *                 |
0400 ++                         *        *        *        *                ++
     |                          *        *        *        *                 |
0300 ++      PWR      PWR       *        *        *        *                ++
     |        *        *        *        *        *        *                 |
0200 ++       *        *        *        *        *        *                ++
     |        *        *        *        *        *        *                 |
0100 ++       *        *        *        *        *        *                ++
     |        *        *        *        *        *        *       Off       |
0000 ++-------+--------+--------+--------+--------+--------+--------+-------++
     0        1        2        3        4        5        6        7
     (Step)
 */

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************
 typedef enum {
  PUMPCTRL_INIT_OK = 0,
  PUMPCTRL_INIT_ERROR,
  PUMPCTRL_LOAD_OK,
  PUMPCTRL_IDLE,
  PUMPCTRL_STEP_1ST,
  PUMPCTRL_STEP_2ND,
  PUMPCTRL_STEP_3RD,
  PUMPCTRL_STEP_STOP,
  PUMPCTRL_STEP_TIME,
  PUMPCTRL_ERROR
} pumpCtrl_status_t;

//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
pumpCtrl_status_t fcn_initPumpController(void);
pumpCtrl_status_t fcn_PumpStateDriver(void);
pumpCtrl_status_t fcn_LoadNewPumpParameters(bleSpressoUserdata_struct *prt_profData);

void fcn_StartBrew(void);
void fcn_CancelBrew(void);