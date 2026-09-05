//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "StorageController.h"
#include "x04_Numbers.h"
/* #include "bluetooth_drv.h"    */
/* #include "nrf_fstorage.h"     */
/* #include "nrf_fstorage_sd.h"  */

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************
/*  DATA STRUCTURE fOR BLE_SPRESSO USER_DATA  */
#define NVM_PARAM_START_ADDR              0x3E000
#define NVM_PARAM_END_ADDR                0x3FFFF
/* Key bumped: 0x38/0x3C slots repurposed (dlpf/gain -> pboost/iboost).
   Already-provisioned units must have the NVM param page erased. */
/* FIXED VALUE - DO NOT MODIFY.
   NVM_PARAM_MEM_KEY is the magic key that marks a valid NVM param page.
   It is immutable: neither a human developer nor an AI agent shall change
   this value. Changing it silently invalidates every already-provisioned
   unit's stored user config. */
#define NVM_PARAM_MEM_KEY                 0x00AA00AA
#define NVM_PARAM_EMPTY_DATA              0xFFFFFFFF
#define NVM_PARAM_PAGE_ADD                0
#define NVM_PARAM_PAGE_OFFSET             0

#define NVM_PARAM_USERDATA_ADD            0
#define NVM_PARAM_USERDATA_SIZE           65

#define NVM_PARAM_WCYCLESECTION_ADD       0
#define NVM_PARAM_WCYCLESECTION_SIZE      4
#define NVM_PARAM_KEYSECTION_ADD          4
#define NVM_PARAM_KEYSECTION_SIZE         4

#define NVM_PARAM_SHOTPROFILE_ADD         8
#define NVM_PARAM_SHOTPROFILE_SIZE        32

#define NVM_PARAM_CONTROLLER_ADD          40
#define NVM_PARAM_CONTROLLER_SIZE         25

/*
  ADDRESS MAP of: espresso_user_config_t 
  -----------------------------------------
  ADDR          Variable 
  -----------------------------------------
  0x00 - 0x03   uint32_t nvmWcycles -> (littleEndia)16bit MSB allocated for ShotProfile - 16b LSB allocated for Controller profile
  0x04 - 0x07   uint32_t nvmKey
  0x08 - 0x0B   float brewTempDegC
  0x0C - 0x0F   float steamTempDegC
  0x10 - 0x13   float profPreInfusePwr
  0x14 - 0x17   float BrewPreInfussionTmr
  0x18 - 0x1B   float profInfusePwr
  0x1C - 0x1F   float profInfuseTmr
  0x20 - 0x23   float profTaperingPwr
  0x24 - 0x27   float profTaperingTmr
  0x28 - 0x2B   float pidPTerm
  0x2C - 0x2F   float pidITerm
  0x30 - 0x33   float pidImaxTerm
  0x34 - 0x37   float pidDTerm
  0x38 - 0x3B   float pidPboostTerm
  0x3C - 0x3F   float pidIboostTerm
  ----------------------------------------
 */
#define USERDATA_NVM_WCYCLE            0x00
#define USERDATA_NVM_FTKEY             0x04
#define USERDATA_BREW_TEMP            0x08
#define USERDATA_STEAM_TEMP           0x0C
#define USERDATA_BREWPREINFUSSION_PWR  0x10
#define USERDATA_BREWPREINFUSSION_TMR  0x14
#define USERDATA_BREWINFUSSION_PWR     0x18
#define USERDATA_BREWINFUSSION_TMR     0x1C
#define USERDATA_BREWDECLINING_PWR     0x20
#define USERDATA_BREWDECLINING_TMR     0x24
#define USERDATA_PID_PTERM             0x28
#define USERDATA_PID_ITERM             0x2C
#define USERDATA_PID_IMAXTERM          0x30
#define USERDATA_PID_DTERM             0x34
#define USERDATA_PID_PBOOSTTERM       0x38
#define USERDATA_PID_IBOOSTTERM       0x3C

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
/* Validate and clamp every float field in the profile struct.
 * Returns PROFILE_VALID if all fields were within safe ranges.
 * Returns PROFILE_CLAMPED if any field had to be corrected.
 * Used by StorageController after NVM read and by bluetooth_drv after BLE writes. */
static profile_validation_status_t validate_clamp_data(
    espresso_user_config_t *profile);

static void unpack_float_from_strg_bytes(uint8_t* ptr_Fbytes, float* ptr_Fnumber);
static void unpack_u32_from_strg_bytes(uint8_t* ptr_Fbytes, uint32_t* ptr_number);
static void pack_float_to_strg_bytes(float fnumber, uint8_t* ptr_Fbytes);

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
* Function: 	storage_init
* Description:  Inits the external spi memory by resetting it and reading its ID
* Return:       STORAGE_INIT_OK = ID match memory manufacturer 
*               STORAGE_INIT_ERROR = ID do NOT match memory manufacturer 
*****************************************************************************/
uint32_t storage_init(void)
{
  return spim_initNVmemory();
}

/*****************************************************************************
* Function: 	storage_has_user_config
* Description:  Reads data from the external memory and look for USER data already
*               stored in it by checking = NVM_PARAM_MEM_KEY
* Return:       
*****************************************************************************/
uint32_t storage_has_user_config(void)
{
  uint8_t rx_key_data_arr[NVM_PARAM_KEYSECTION_SIZE];
  uint32_t nvm_key;
  uint32_t data_status = 0U;

  memset(rx_key_data_arr, 0x00U, NVM_PARAM_KEYSECTION_SIZE);
  spi_NVMemoryRead(NVM_PARAM_PAGE_ADD,
                  NVM_PARAM_KEYSECTION_ADD,
                  NVM_PARAM_KEYSECTION_SIZE,
                  &rx_key_data_arr[0]);

  unpack_u32_from_strg_bytes((uint8_t *)&rx_key_data_arr[0], &nvm_key);

  if(nvm_key == NVM_PARAM_MEM_KEY)
  {
    data_status = STORAGE_USERDATA_LOADED;
  }else if(nvm_key == NVM_PARAM_EMPTY_DATA)
  {
    data_status = STORAGE_USERDATA_EMPTY;
  }
  return data_status;
}

/*****************************************************************************
* Function: 	storage_erase_user_config
* Description:  Erases the NVM param sector (NVM_PARAM_MEM_KEY + the rest of
*               espresso_user_config_t). Developer/debug tool only, triggered
*               via the ESPRESSO_CFG_ERASE_NVM_KEY compile-time one-shot in
*               espressoMachineServices.h.
* Return:       STORAGE_USERDATA_ERASED
*****************************************************************************/
uint32_t storage_erase_user_config(void)
{
  spi_NVMemoryErasePage(NVM_PARAM_PAGE_ADD);
  return STORAGE_USERDATA_ERASED;
}

/*****************************************************************************
* Function: 	storage_load_user_config
* Description:  Reads data from the external memory and store it the UserData Pointer
* Return:       STORAGE_USERDATA_LOADED = memory read success
                STORAGE_USERDATA_ERROR = Could not read nvm or 0xff
*****************************************************************************/
uint32_t storage_load_user_config(espresso_user_config_t* ptr_rxData)
{
  uint8_t rx_user_data_arr[NVM_PARAM_USERDATA_SIZE];
  uint32_t nvm_key, nvm_w_cycle;
  volatile uint16_t w_cycle_shot_profile = 0U;
  volatile uint16_t w_cycle_ctrl_profile = 0U;
  uint32_t data_status = 0xFFFFFFFFU;
  float temp_float_val;

  memset(rx_user_data_arr, 0x00U, NVM_PARAM_USERDATA_SIZE);
  spi_NVMemoryRead( NVM_PARAM_PAGE_ADD,
                    NVM_PARAM_PAGE_OFFSET,
                    NVM_PARAM_USERDATA_SIZE,
                    &rx_user_data_arr[NVM_PARAM_USERDATA_ADD]);
  unpack_u32_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_NVM_FTKEY], &nvm_key);
  ptr_rxData->nvmKey = nvm_key;

  if(nvm_key == NVM_PARAM_MEM_KEY)
  {
    unpack_u32_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_NVM_WCYCLE], &nvm_w_cycle);
    ptr_rxData->nvmWcycles = nvm_w_cycle;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_BREW_TEMP],
                                 (float*)&temp_float_val);
    ptr_rxData->brewTempDegC = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_STEAM_TEMP],
                                 (float*)&temp_float_val);
    ptr_rxData->steamTempDegC = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_BREWPREINFUSSION_PWR],
                                 (float*)&temp_float_val);
    ptr_rxData->profPreInfusePwr = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_BREWPREINFUSSION_TMR],
                                 (float*)&temp_float_val);
    ptr_rxData->profPreInfuseTmr = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_BREWINFUSSION_PWR],
                                 (float*)&temp_float_val);
    ptr_rxData->profInfusePwr = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_BREWINFUSSION_TMR],
                                 (float*)&temp_float_val);
    ptr_rxData->profInfuseTmr = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_BREWDECLINING_PWR],
                                 (float*)&temp_float_val);
    ptr_rxData->profTaperingPwr = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_BREWDECLINING_TMR],
                                 (float*)&temp_float_val);
    ptr_rxData->profTaperingTmr = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_PID_PTERM],
                                 (float*)&temp_float_val);
    ptr_rxData->pidPTerm = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_PID_ITERM],
                                 (float*)&temp_float_val);
    ptr_rxData->pidITerm = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_PID_IMAXTERM],
                                 (float*)&temp_float_val);
    ptr_rxData->pidImaxTerm = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_PID_DTERM],
                                 (float*)&temp_float_val);
    ptr_rxData->pidDTerm = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_PID_PBOOSTTERM],
                                 (float*)&temp_float_val);
    ptr_rxData->pidPboostTerm = temp_float_val;

    unpack_float_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_PID_IBOOSTTERM],
                                 (float*)&temp_float_val);
    ptr_rxData->pidIboostTerm = temp_float_val;

    data_status = STORAGE_USERDATA_LOADED;
    /* M3 fix: validate all deserialized float fields and clamp out-of-range
     * values to safe defaults.  Protects against corrupted flash data. */
    (void)validate_clamp_data(ptr_rxData);
  }else{
    data_status = STORAGE_USERDATA_EMPTY;
  }
  (void)w_cycle_shot_profile;
  (void)w_cycle_ctrl_profile;
  return data_status;
}

/*****************************************************************************
* Function: 	storage_save_shot_profile
* Description:  wrtie NEW Espresso profile into NVM  
* Return:       STORAGE_PROFILEDATA_STORED
*****************************************************************************/
uint32_t storage_save_shot_profile(espresso_user_config_t* ptr_sxData)
{
  uint8_t tx_user_data_arr[NVM_PARAM_USERDATA_SIZE];
  uint8_t rx_user_data_arr[NVM_PARAM_USERDATA_SIZE];
  uint32_t nvm_key, nvm_w_cycle;
  volatile uint16_t w_cycle_shot_profile = 0U;
  volatile uint16_t w_cycle_ctrl_profile = 0U;
  uint32_t data_status = 0xFFFFFFFFU;

  memset(tx_user_data_arr, 0x00U, NVM_PARAM_USERDATA_SIZE);
  memset(rx_user_data_arr, 0x00U, NVM_PARAM_USERDATA_SIZE);
  spi_NVMemoryRead( NVM_PARAM_PAGE_ADD,
                    NVM_PARAM_PAGE_OFFSET,
                    NVM_PARAM_USERDATA_SIZE,
                    &rx_user_data_arr[NVM_PARAM_USERDATA_ADD]);
  unpack_u32_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_NVM_FTKEY], &nvm_key);
  if(nvm_key == NVM_PARAM_MEM_KEY)
  {
    data_status = STORAGE_USERDATA_STORED;
    (void)mempcpy(&tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD],
                  &rx_user_data_arr[NVM_PARAM_KEYSECTION_ADD],
                  NVM_PARAM_KEYSECTION_SIZE);
  }else if(nvm_key == NVM_PARAM_EMPTY_DATA){
    data_status = STORAGE_USERDATA_FIRSTW;
    tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD+0U] = 0xAAU;
    tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD+1U] = 0x00U;
    tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD+2U] = 0xAAU;
    tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD+3U] = 0x00U;
  }else{
    data_status = STORAGE_USERDATA_ERROR;
  }

  if(data_status != STORAGE_USERDATA_ERROR)
  {
    unpack_u32_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_NVM_WCYCLE], &nvm_w_cycle);
    w_cycle_shot_profile = (uint16_t)((nvm_w_cycle) >> 16U);
    w_cycle_ctrl_profile = (uint16_t)((nvm_w_cycle) & 0x00FFU);
    w_cycle_shot_profile++;
    tx_user_data_arr[USERDATA_NVM_WCYCLE+3U] = (uint8_t)(w_cycle_shot_profile >> 8U);
    tx_user_data_arr[USERDATA_NVM_WCYCLE+2U] = (uint8_t)(w_cycle_shot_profile & 0x00FFU);
    tx_user_data_arr[USERDATA_NVM_WCYCLE+1U] = (uint8_t)(w_cycle_ctrl_profile >> 8U);
    tx_user_data_arr[USERDATA_NVM_WCYCLE+0U] = (uint8_t)(w_cycle_ctrl_profile & 0x00FFU);
    (void)mempcpy(&tx_user_data_arr[NVM_PARAM_CONTROLLER_ADD],
                  &rx_user_data_arr[NVM_PARAM_CONTROLLER_ADD],
                  NVM_PARAM_CONTROLLER_SIZE);
    pack_float_to_strg_bytes(ptr_sxData->brewTempDegC,
                             (uint8_t *)&tx_user_data_arr[USERDATA_BREW_TEMP]);
    pack_float_to_strg_bytes(ptr_sxData->steamTempDegC,
                             (uint8_t *)&tx_user_data_arr[USERDATA_STEAM_TEMP]);
    pack_float_to_strg_bytes((float)ptr_sxData->profPreInfusePwr,
                             (uint8_t *)&tx_user_data_arr[USERDATA_BREWPREINFUSSION_PWR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profPreInfuseTmr,
                             (uint8_t *)&tx_user_data_arr[USERDATA_BREWPREINFUSSION_TMR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profInfusePwr,
                             (uint8_t *)&tx_user_data_arr[USERDATA_BREWINFUSSION_PWR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profInfuseTmr,
                             (uint8_t *)&tx_user_data_arr[USERDATA_BREWINFUSSION_TMR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profTaperingPwr,
                             (uint8_t *)&tx_user_data_arr[USERDATA_BREWDECLINING_PWR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profTaperingTmr,
                             (uint8_t *)&tx_user_data_arr[USERDATA_BREWDECLINING_TMR]);
    spi_NVMemoryWritePage(NVM_PARAM_PAGE_ADD,
                          NVM_PARAM_PAGE_OFFSET,
                          NVM_PARAM_USERDATA_SIZE,
                          &tx_user_data_arr[NVM_PARAM_USERDATA_ADD]);
    data_status = STORAGE_PROFILEDATA_STORED;
  }
  return data_status;
}

/*****************************************************************************
* Function: 	storage_save_controller_config
* Description:  wrtie NEW controller profile into NVM
* Return:       STORAGE_CONTROLLERDATA_STORED
*****************************************************************************/
uint32_t storage_save_controller_config(espresso_user_config_t* ptr_sxData)
{
  uint8_t tx_user_data_arr[NVM_PARAM_USERDATA_SIZE];
  uint8_t rx_user_data_arr[NVM_PARAM_USERDATA_SIZE];
  uint32_t nvm_key, nvm_w_cycle;
  volatile uint16_t w_cycle_shot_profile = 0U;
  volatile uint16_t w_cycle_ctrl_profile = 0U;
  uint32_t data_status = 0xFFFFFFFFU;

  memset(tx_user_data_arr, 0x00U, NVM_PARAM_USERDATA_SIZE);
  memset(rx_user_data_arr, 0x00U, NVM_PARAM_USERDATA_SIZE);
  spi_NVMemoryRead( NVM_PARAM_PAGE_ADD,
                    NVM_PARAM_PAGE_OFFSET,
                    NVM_PARAM_USERDATA_SIZE,
                    &rx_user_data_arr[NVM_PARAM_USERDATA_ADD]);
  unpack_u32_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_NVM_FTKEY], &nvm_key);
  if(nvm_key == NVM_PARAM_MEM_KEY)
  {
    data_status = STORAGE_USERDATA_STORED;
    (void)mempcpy(&tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD],
                  &rx_user_data_arr[NVM_PARAM_KEYSECTION_ADD],
                  NVM_PARAM_KEYSECTION_SIZE);
  }else if(nvm_key == NVM_PARAM_EMPTY_DATA){
    data_status = STORAGE_USERDATA_FIRSTW;
    tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD+0U] = 0xAAU;
    tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD+1U] = 0x00U;
    tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD+2U] = 0xAAU;
    tx_user_data_arr[NVM_PARAM_KEYSECTION_ADD+3U] = 0x00U;
  }else{
    data_status = STORAGE_USERDATA_ERROR;
  }

  if(data_status != STORAGE_USERDATA_ERROR)
  {
    unpack_u32_from_strg_bytes((uint8_t *)&rx_user_data_arr[USERDATA_NVM_WCYCLE], &nvm_w_cycle);
    w_cycle_shot_profile = (uint16_t)((nvm_w_cycle) >> 16U);
    w_cycle_ctrl_profile = (uint16_t)((nvm_w_cycle) & 0x00FFU);
    w_cycle_ctrl_profile++;
    tx_user_data_arr[USERDATA_NVM_WCYCLE+3U] = (uint8_t)(w_cycle_shot_profile >> 8U);
    tx_user_data_arr[USERDATA_NVM_WCYCLE+2U] = (uint8_t)(w_cycle_shot_profile & 0x00FFU);
    tx_user_data_arr[USERDATA_NVM_WCYCLE+1U] = (uint8_t)(w_cycle_ctrl_profile >> 8U);
    tx_user_data_arr[USERDATA_NVM_WCYCLE+0U] = (uint8_t)(w_cycle_ctrl_profile & 0x00FFU);
    (void)mempcpy(&tx_user_data_arr[NVM_PARAM_SHOTPROFILE_ADD],
                  &rx_user_data_arr[NVM_PARAM_SHOTPROFILE_ADD],
                  NVM_PARAM_SHOTPROFILE_SIZE);
    pack_float_to_strg_bytes(ptr_sxData->pidPTerm,
                             (uint8_t *)&tx_user_data_arr[USERDATA_PID_PTERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidITerm,
                             (uint8_t *)&tx_user_data_arr[USERDATA_PID_ITERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidImaxTerm,
                             (uint8_t *)&tx_user_data_arr[USERDATA_PID_IMAXTERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidDTerm,
                             (uint8_t *)&tx_user_data_arr[USERDATA_PID_DTERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidPboostTerm,
                             (uint8_t *)&tx_user_data_arr[USERDATA_PID_PBOOSTTERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidIboostTerm,
                             (uint8_t *)&tx_user_data_arr[USERDATA_PID_IBOOSTTERM]);
    spi_NVMemoryWritePage(NVM_PARAM_PAGE_ADD,
                          NVM_PARAM_PAGE_OFFSET,
                          NVM_PARAM_USERDATA_SIZE,
                          &tx_user_data_arr[NVM_PARAM_USERDATA_ADD]);
    data_status = STORAGE_CONTROLLERDATA_STORED;
  }
  return data_status;

}

/*****************************************************************************
* Function: 	storage_print_user_config
* Description:  Print into UART the data contain is *ptr_rxData Pointer
* Return:       STORAGE_USERDATA_PRINTED
*****************************************************************************/
uint32_t storage_print_user_config(espresso_user_config_t* ptr_rxData)
{
  uint16_t w_cycle_shot_profile;
  uint16_t w_cycle_ctrl_profile;

  w_cycle_shot_profile = (uint16_t)((ptr_rxData->nvmWcycles) >> 16U);
  w_cycle_ctrl_profile = (uint16_t)((ptr_rxData->nvmWcycles) & 0x00FFU);

  /* Print: uint32_t nvmWcycles; */
  NRF_LOG_DEBUG("NVM Profile- # of Write cycles:  %d\r\n",
                (w_cycle_shot_profile));
  NRF_LOG_DEBUG("NVM CTRL-    # of Write cycles:  %d\r\n",
                (w_cycle_ctrl_profile));
  /* Print: uint32_t nvmKey; (not printed - fixed magic value) */

  /* Print: float brewTempDegC; */
  NRF_LOG_DEBUG("Boiler-      Brew Temp:          " NRF_LOG_FLOAT_MARKER "  degC\r\n",
                NRF_LOG_FLOAT(ptr_rxData->brewTempDegC));
  /* Print: float steamTempDegC; */
  NRF_LOG_DEBUG("Boiler-      Steam Temp:         " NRF_LOG_FLOAT_MARKER "  degC\r\n",
                NRF_LOG_FLOAT(ptr_rxData->steamTempDegC));
  /* Print: float profPreInfusePwr; */
  NRF_LOG_DEBUG("Profile-     PreInfuse Power:    " NRF_LOG_FLOAT_MARKER "  %%\r\n",
                NRF_LOG_FLOAT(ptr_rxData->profPreInfusePwr));
  /* Print: float profPreInfuseTmr; */
  NRF_LOG_DEBUG("Profile-     PreInfuse Timer:    " NRF_LOG_FLOAT_MARKER "  seconds\r\n",
                NRF_LOG_FLOAT(ptr_rxData->profPreInfuseTmr));
  /* Print: float profInfusePwr; */
  NRF_LOG_DEBUG("Profile-     Infuse Power:       " NRF_LOG_FLOAT_MARKER "  %%\r\n",
                NRF_LOG_FLOAT(ptr_rxData->profInfusePwr));
  /* Print: float profInfuseTmr; */
  NRF_LOG_DEBUG("Profile-     Infuse Timer:       " NRF_LOG_FLOAT_MARKER "  seconds\r\n",
                NRF_LOG_FLOAT(ptr_rxData->profInfuseTmr));
  /* Print: float profTaperingPwr; */
  NRF_LOG_DEBUG("Profile-     Declining Power:    " NRF_LOG_FLOAT_MARKER "  %%\r\n",
                NRF_LOG_FLOAT(ptr_rxData->profTaperingPwr));
  /* Print: float profTaperingTmr; */
  NRF_LOG_DEBUG("Profile-     Declining Timer:    " NRF_LOG_FLOAT_MARKER "  seconds\r\n",
                NRF_LOG_FLOAT(ptr_rxData->profTaperingTmr));

  /* Print: float pidPTerm; */
  NRF_LOG_DEBUG("Controller-  Proportial Gain:    " NRF_LOG_FLOAT_MARKER "\r\n",
                NRF_LOG_FLOAT(ptr_rxData->pidPTerm));
  /* Print: float pidITerm; */
  NRF_LOG_DEBUG("Controller-  Integral Gain:      " NRF_LOG_FLOAT_MARKER "\r\n",
                NRF_LOG_FLOAT(ptr_rxData->pidITerm));
  /* Print: float pidImaxTerm; */
  NRF_LOG_DEBUG("Controller-  Max Integral Value: " NRF_LOG_FLOAT_MARKER "\r\n",
                NRF_LOG_FLOAT(ptr_rxData->pidImaxTerm));
  /* Print: float pidDTerm; */
  NRF_LOG_DEBUG("Controller-  Derivative Gain:    " NRF_LOG_FLOAT_MARKER "\r\n",
                NRF_LOG_FLOAT(ptr_rxData->pidDTerm));
  /* Print: float pidPboostTerm; */
  NRF_LOG_DEBUG("Controller-  P boost:            " NRF_LOG_FLOAT_MARKER "\r\n",
                NRF_LOG_FLOAT(ptr_rxData->pidPboostTerm));
  /* Print: float pidIboostTerm; */
  NRF_LOG_DEBUG("Controller-  I boost:            " NRF_LOG_FLOAT_MARKER "\r\n",
                NRF_LOG_FLOAT(ptr_rxData->pidIboostTerm));

  return STORAGE_USERDATA_PRINTED;
}

 
//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
* Function: 	unpack_float_from_strg_bytes
* Hint:         Parsing	= Convert from string/binary to data structures
* Description:  Provide the add of the bytes array & pointer to the float where to store the result of parsing
* Return:       
*****************************************************************************/
static void unpack_float_from_strg_bytes(uint8_t* ptr_Fbytes, float* ptr_Fnumber)
{
  volatile uint32_t hex_temp = 0x00000000U;
  hex_temp = ((uint32_t)(*(ptr_Fbytes+3U)<<24U) & 0xFF000000U) |
             ((uint32_t)(*(ptr_Fbytes+2U)<<16U) & 0x00FF0000U) |
             ((uint32_t)(*(ptr_Fbytes+1U)<<8U)  & 0x0000FF00U) |
             ((uint32_t)  *ptr_Fbytes            & 0x000000FFU);
  *ptr_Fnumber = *((float *)&hex_temp);
}

/*****************************************************************************
* Function: 	unpack_u32_from_strg_bytes
* Hint:         Parsing	= Convert from string/binary to data structures
* Description:  Provide the add of the bytes array & pointer to the float where to store the result of parsing
* Return:       
*****************************************************************************/
static void unpack_u32_from_strg_bytes(uint8_t* ptr_Fbytes, uint32_t* ptr_number)
{
  *ptr_number = ((uint32_t)(*(ptr_Fbytes+3U)<<24U) & 0xFF000000U) |
                ((uint32_t)(*(ptr_Fbytes+2U)<<16U) & 0x00FF0000U) |
                ((uint32_t)(*(ptr_Fbytes+1U)<<8U)  & 0x0000FF00U) |
                ((uint32_t)  *ptr_Fbytes            & 0x000000FFU);
}

/*****************************************************************************
* Function: 	pack_float_to_strg_bytes
* Hint:         Serialization/Encoding = Convert from data structures to string/binary
* Description:  Provide the Float number and the pointer to array where the hex value of the float is going to be stored.
* Return:       
*****************************************************************************/
static void pack_float_to_strg_bytes(float fnumber, uint8_t* ptr_Fbytes)
{
  static uint32_t hex_temp = 0x00000000U;
  hex_temp = *((uint32_t *)&fnumber);

  *ptr_Fbytes++ = (uint8_t)(hex_temp & 0xFFU);
  *ptr_Fbytes++ = (uint8_t)((hex_temp >> 8U) & 0xFFU);
  *ptr_Fbytes++ = (uint8_t)((hex_temp >> 16U) & 0xFFU);
  *ptr_Fbytes   = (uint8_t)((hex_temp >> 24U) & 0xFFU);
}


/* ---------------------------------------------------------------------------
 * validate_clamp_data
 *
 * Validates every user-configurable float field in the profile struct.
 * Out-of-range or non-finite values are replaced with safe defaults.
 * Returns PROFILE_VALID if no corrections were needed, PROFILE_CLAMPED if
 * at least one field was corrected.
 * --------------------------------------------------------------------------- */
static profile_validation_status_t validate_clamp_data(
    espresso_user_config_t *profile)
{
    bool all_valid = true;

    /* ---- Temperature setpoints ---- */
    all_valid &= validate_float_in_range(&profile->boilerTempSetpointDegC,
        BOILER_SETPOINT_TEMP_MIN_DEGC, BOILER_SETPOINT_TEMP_MAX_DEGC, BOILER_SETPOINT_TEMP_DEFAULT_DEGC);
    all_valid &= validate_float_in_range(&profile->brewTempDegC,
        BREW_TEMP_MIN_DEGC, BREW_TEMP_MAX_DEGC, BREW_TEMP_DEFAULT_DEGC);
    all_valid &= validate_float_in_range(&profile->steamTempDegC,
        STEAM_TEMP_MIN_DEGC, STEAM_TEMP_MAX_DEGC, STEAM_TEMP_DEFAULT_DEGC);

    /* ---- Brew profile — power (0–100 %) ---- */
    all_valid &= validate_float_in_range(&profile->profPreInfusePwr,
        PROF_PREINFUSE_PWR_MIN_PWR, PROF_PREINFUSE_PWR_MAX_PWR, PROF_PREINFUSE_PWR_DEFAULT_PWR);
    all_valid &= validate_float_in_range(&profile->profInfusePwr,
        PROF_INFUSE_PWR_MIN_PWR, PROF_INFUSE_PWR_MAX_PWR, PROF_INFUSE_PWR_DEFAULT_PWR);
    all_valid &= validate_float_in_range(&profile->profTaperingPwr,
        PROF_TAPERING_PWR_MIN_PWR, PROF_TAPERING_PWR_MAX_PWR, PROF_TAPERING_PWR_DEFAULT_PWR);

    /* ---- Brew profile — timers (seconds) ---- */
    all_valid &= validate_float_in_range(&profile->profPreInfuseTmr,
        PROF_PREINFUSE_TMR_MIN_SECS, PROF_PREINFUSE_TMR_MAX_SECS, PROF_PREINFUSE_TMR_DEFAULT_SECS);
    all_valid &= validate_float_in_range(&profile->profInfuseTmr,
        PROF_INFUSE_TMR_MIN_SECS, PROF_INFUSE_TMR_MAX_SECS, PROF_INFUSE_TMR_DEFAULT_SECS);
    all_valid &= validate_float_in_range(&profile->profTaperingTmr,
        PROF_TAPERING_TMR_MIN_SECS, PROF_TAPERING_TMR_MAX_SECS, PROF_TAPERING_TMR_DEFAULT_SECS);

    /* ---- PID gains ---- */
    all_valid &= validate_float_in_range(&profile->pidPTerm,
        PID_P_TERM_MIN, PID_P_TERM_MAX, PID_P_TERM_DEFAULT);
    all_valid &= validate_float_in_range(&profile->pidITerm,
        PID_I_TERM_MIN, PID_I_TERM_MAX, PID_I_TERM_DEFAULT);
    all_valid &= validate_float_in_range(&profile->pidImaxTerm,
        PID_I_MAX_TERM_MIN, PID_I_MAX_TERM_MAX, PID_I_MAX_TERM_DEFAULT);
    all_valid &= validate_float_in_range(&profile->pidDTerm,
        PID_D_TERM_MIN, PID_D_TERM_MAX, PID_D_TERM_DEFAULT);
    all_valid &= validate_float_in_range(&profile->pidPboostTerm,
        PID_P_BOOST_TERM_MIN, PID_P_BOOST_TERM_MAX, PID_P_BOOST_TERM_DEFAULT);
    all_valid &= validate_float_in_range(&profile->pidIboostTerm,
        PID_I_BOOST_TERM_MIN, PID_I_BOOST_TERM_MAX, PID_I_BOOST_TERM_DEFAULT);

    return all_valid ? PROFILE_VALID : PROFILE_CLAMPED;
}