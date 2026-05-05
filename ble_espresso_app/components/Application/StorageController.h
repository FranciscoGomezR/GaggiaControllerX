
/*************************************************************************************
* 	Revision History:
*
*   Date          	CP#           Author
*   DD-MM-YYYY      XXXXX:1		Initials	Description of change
*   -----------   ------------  ---------   ------------------------------------
*  	XX-XX-XXXX		X.X			ABCD		"CHANGE"	
*
*************************************************************************************
*/
#ifndef STORAGECONTROLLER_H__
#define STORAGECONTROLLER_H__
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf.h"
#include "nrf_soc.h"
#include "nordic_common.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "espressoMachineServices.h"
#include "spi_Devices.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************


//https://devzone.nordicsemi.com/f/nordic-q-a/31017/fstorage-vs-softdevice-activity-application-halts-when-writing-to-flash-if-waiting-to-write-or-data-not-correctly-written-if-not-waiting

typedef enum {
  STORAGE_INIT_OK = 0,
  STORAGE_INIT_ERROR,
  STORAGE_USERDATA_EMPTY,
  STORAGE_USERDATA_FIRSTW,
  STORAGE_USERDATA_LOADED,
  STORAGE_USERDATA_STORED,
  STORAGE_USERDATA_PRINTED,
  STORAGE_PROFILEDATA_STORED,
  STORAGE_CONTROLLERDATA_STORED,
  STORAGE_USERDATA_ERROR
} storage_controller_status_t;

typedef enum {
    PROFILE_VALID    = 0,  /* all fields within range — no change */
    PROFILE_CLAMPED  = 1,  /* one or more fields were out of range and clamped to safe default */
    PROFILE_REJECTED = 2   /* NaN or Inf detected — all fields reset (not currently used) */
} profile_validation_status_t;
//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************


//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
uint32_t storage_init(void);
uint32_t storage_has_user_config(void);
uint32_t storage_load_user_config(espresso_user_config_t *ptr_rxData);
uint32_t storage_save_shot_profile(espresso_user_config_t *ptr_sxData);
uint32_t storage_save_controller_config(espresso_user_config_t *ptr_sxData);
uint32_t storage_print_user_config(espresso_user_config_t *ptr_rxData);

#endif // BLESPRESSOSERVICES_H__

