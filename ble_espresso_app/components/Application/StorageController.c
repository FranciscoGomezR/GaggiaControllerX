
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "StorageController.h"
#include "bluetooth_drv.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

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
  static bool nvmWdataFlag;

  NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
  {
      /* Set a handler for fstorage events. */
      .evt_handler = fstorage_evt_handler,

      /* These below are the boundaries of the flash space assigned to this instance of fstorage.
       * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
       * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
       * last page of flash available to write data. */
      .start_addr = PARAM_NVM_START_ADDR,
      .end_addr   = PARAM_NVM_END_ADDR,
  };

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
  
  void fstorage_Init(void)
  {
      ret_code_t rc;
      nrf_fstorage_api_t * p_fs_api;
      /* Initialize an fstorage instance using the nrf_fstorage_sd backend.
       * nrf_fstorage_sd uses the SoftDevice to write to flash. This implementation can safely be
       * used whenever there is a SoftDevice, regardless of its status (enabled/disabled). */
      p_fs_api = &nrf_fstorage_sd;
      NRF_LOG_DEBUG("FDS INIT: FLASH block init.");
      rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
      APP_ERROR_CHECK(rc);
      fcn_Read_ParameterNVM((BLEspressoVariable_struct *)&int_NvmData);
      if(int_NvmData.nvmKey == PARAM_NVM_MEM_KEY)
      {
        nvmWdataFlag = true;
        memcpy((void *)&BLEspressoVar, (void *)&int_NvmData, sizeof(BLEspressoVariable_struct));
        NRF_LOG_DEBUG("FDS INIT: FLASH block already contains data");
      }else{

        nvmWdataFlag = false;
        NRF_LOG_DEBUG("FDS INIT: FLASH block empty");
      }
      NRF_LOG_FLUSH();
  }

  void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
  {
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }
    switch (p_evt->id)
    {
		case NRF_FSTORAGE_EVT_READ_RESULT:
        {
            NRF_LOG_INFO("--> Event received: read %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
  }

  void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
  {
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        nrf_pwr_mgmt_run();
    }
  }
  void wait_for_flash_ready_noSoftDevice(nrf_fstorage_t const * p_fstorage)
  {
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
    }
  }


  void fcn_WriteParameterNVM(BLEspressoVariable_struct * ptr_writeParam)
  {
    ret_code_t rc;
    /* Let's erease  flash. */
    //uint32_t flashEndAddr = fs_flash_page_end_addr();
    rc = nrf_fstorage_erase(&fstorage, PARAM_NVM_START_ADDR, 1,NULL);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);
    
    ptr_writeParam->nvmKey = PARAM_NVM_MEM_KEY;
    rc = nrf_fstorage_write(&fstorage, PARAM_NVM_START_ADDR, (void const *)ptr_writeParam, sizeof(BLEspressoVariable_struct), NULL);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);
  }

  void fcn_Read_ParameterNVM(BLEspressoVariable_struct * ptr_ReadParam)
  {
    ret_code_t rc;
    //volatile BLEspressoVariable_struct r_data;
    //volatile float rTemp;
    //wait_for_flash_ready(&fstorage);
    rc = nrf_fstorage_read(&fstorage,PARAM_NVM_START_ADDR,(void *)ptr_ReadParam,sizeof(BLEspressoVariable_struct));
    APP_ERROR_CHECK(rc);
    //wait_for_flash_ready(&fstorage);
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