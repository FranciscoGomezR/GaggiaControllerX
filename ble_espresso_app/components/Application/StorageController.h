
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
#include "BLEspressoServices.h"
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
} storageCtrl_status_t;
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
uint32_t stgCtrl_Init(void);
uint32_t stgCtrl_ChkForUserData(void);
uint32_t stgCtrl_ReadUserData(bleSpressoUserdata_struct* ptr_rxData);
uint32_t stgCtrl_StoreShotProfileData(bleSpressoUserdata_struct* ptr_sxData);
uint32_t stgCtrl_StoreControllerData(bleSpressoUserdata_struct* ptr_sxData);
uint32_t stgCtrl_PrintUserData(bleSpressoUserdata_struct* ptr_rxData);

#endif // BLESPRESSOSERVICES_H__

