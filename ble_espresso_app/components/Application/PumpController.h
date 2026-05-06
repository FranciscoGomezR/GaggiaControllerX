//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include <stdbool.h>
#include "espressoMachineServices.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
#define RAMP_UP_TIME_MSECS_DEFAULT    2500
#define RAMP_DOWN_TIME_MSECS_DEFAULT  2500

/*  STEP  1 */
/*  Power is fixed point one decimal; from 0000 to 1000 (0.0 to 100.0)  */
#define PRE_INFUSE_PWR_DEFAULT        500
#define PRE_INFUSE_TIME_MSECS_DEFAULT 5000
/*  STEP  2 */
/*  Power is fixed point one decimal; from 0000 to 1000 (0.0 to 100.0)  */
#define BREW_PWR_DEFAULT              1000
#define BREW_TIME_MSECS_DEFAULT       5000
/*  STEP  3 */
/*  Power is fixed point one decimal; from 0000 to 1000 (0.0 to 100.0)  */
#define TAPER_PWR_DEFAULT             900
#define TAPER_TIME_MSECS_DEFAULT      5000
/*  STEP  4 */
/* Shutting the pump down Adn open the valve  */

#define PUMP_BASE_TIME_MSECS          250

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
pumpCtrl_status_t init_pump_controller(void);
pumpCtrl_status_t pump_state_driver(void);
pumpCtrl_status_t load_new_pump_parameters(espresso_user_config_t *ptr_prof_data);

void start_brew(void);
void cancel_brew(void);