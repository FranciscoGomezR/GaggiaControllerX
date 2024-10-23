
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "StorageController.h"
#include "bluetooth_drv.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************

//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************

//*****************************************************************************
//
//			PUBLIC VARIABLES
//
//****************************************************************************


//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************

  /**@brief   Helper function to obtain the last address on the last page of the on-chip flash that
  *          can be used to write user data.
  */
  static uint32_t nrf5_flash_end_addr_get()
  {
    uint32_t const bootloader_addr = BOOTLOADER_ADDRESS;
    uint32_t const page_sz         = NRF_FICR->CODEPAGESIZE;
    uint32_t const code_sz         = NRF_FICR->CODESIZE;

    return (bootloader_addr != 0xFFFFFFFF ?
            bootloader_addr : (code_sz * page_sz));
  }

  void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
  {
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        nrf_pwr_mgmt_run();
    }
  }

  void fcn_runtest(void)
  {
    ret_code_t rc;
    nrf_fstorage_api_t * p_fs_api;
    /* Initialize an fstorage instance using the nrf_fstorage_sd backend.
     * nrf_fstorage_sd uses the SoftDevice to write to flash. This implementation can safely be
     * used whenever there is a SoftDevice, regardless of its status (enabled/disabled). */
    p_fs_api = &nrf_fstorage_sd;
    rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);

    rc = nrf_fstorage_erase(&fstorage, 0x3e000, 1,NULL);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);

    

    /* Let's write to flash. */
    
    rc = nrf_fstorage_write(&fstorage, 0x3e000, (void const *)&BLEspressoVar, sizeof(BLEspressoVariable_struct), NULL);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);

    volatile BLEspressoVariable_struct r_data;
    volatile float rTemp;
    rc = nrf_fstorage_read(&fstorage,0x3e000,(void *)&r_data,sizeof(BLEspressoVariable_struct));
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);
    rTemp =  r_data.BrewPreInfussionPwr;
  }

//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	InitClocks
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
 // Public function 2 