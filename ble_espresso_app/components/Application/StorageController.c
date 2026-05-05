//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "StorageController.h"
#include "x04_Numbers.h"
//#include "bluetooth_drv.h"
//#include "nrf_fstorage.h"
//#include "nrf_fstorage_sd.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************
/*  DATA STRUCTURE fOR BLE_SPRESSO USER_DATA  */
#define PARAM_NVM_START_ADDR              0x3E000
#define PARAM_NVM_END_ADDR                0x3FFFF
#define PARAM_NVM_MEM_KEY                 0x00AA00AA
#define PARAM_NVM_EMPTY_DATA              0xFFFFFFFF
#define PARAM_NVM_PAGE_ADD                0
#define PARAM_NVM_PAGE_OFFSET             0

#define PARAM_NVM_USERDATA_ADD            0
#define PARAM_NVM_USERDATA_SIZE           65

#define PARAM_NVM_WCYCLESECTION_ADD       0
#define PARAM_NVM_WCYCLESECTION_SIZE      4
#define PARAM_NVM_KEYSECTION_ADD          4
#define PARAM_NVM_KEYSECTION_SIZE         4

#define PARAM_NVM_SHOTPROFILE_ADD         8
#define PARAM_NVM_SHOTPROFILE_SIZE        32

#define PARAM_NVM_CONTROLLER_ADD          40
#define PARAM_NVM_CONTROLLER_SIZE         25

/*
  ADDRESS MAP of: espresso_user_config_t 
  -----------------------------------------
  ADDR          Variable 
  -----------------------------------------
  0x00 - 0x03   uint32_t nvmWcycles -> (littleEndia)16bit MSB allocated for ShotProfile - 16b LSB allocated for Controller profile
  0x04 - 0x07   uint32_t nvmKey
  0x08 - 0x0B   float boilerTempSetpointDegC
  0x0C - 0x0F   float RSVD
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
  0x38 - 0x3B   float pidDlpfTerm
  0x3C - 0x3F   float pidGainTerm
  0x40          bool  pidIwindupTerm
  ----------------------------------------
 */
#define BE_USERDATA_NVM_WCYCLE            0x00
#define BE_USERDATA_NVM_FTKEY             0x04
#define BE_USERDATA_TARGETBOILER_TMP      0x08
#define BE_USERDATA_RSVD                  0x0C
#define BE_USERDATA_BREWPREINFUSSION_PWR  0x10
#define BE_USERDATA_BREWPREINFUSSION_TMR  0x14
#define BE_USERDATA_BREWINFUSSION_PWR     0x18
#define BE_USERDATA_BREWINFUSSION_TMR     0x1C
#define BE_USERDATA_BREWDECLINING_PWR     0x20
#define BE_USERDATA_BREWDECLINING_TMR     0x24
#define BE_USERDATA_PID_PTERM             0x28
#define BE_USERDATA_PID_ITERM             0x2C
#define BE_USERDATA_PID_IMAXTERM          0x30
#define BE_USERDATA_PID_DTERM             0x34
#define BE_USERDATA_PID_DLPFTERM          0x38
#define BE_USERDATA_PID_GAINTERM          0x3C
#define BE_USERDATA_PID_IWINDUPTERM       0x40

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
profile_validation_status_t validate_clamp_data(
    espresso_user_config_t *profile);

void unpack_float_from_strg_bytes(uint8_t* ptr_Fbytes, float* ptr_Fnumber);
void unpack_u32_from_strg_bytes(uint8_t* ptr_Fbytes, uint32_t* ptr_number);
void pack_float_to_strg_bytes(float fnumber, uint8_t* ptr_Fbytes);

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
*               stored in it by checking = PARAM_NVM_MEM_KEY
* Return:       
*****************************************************************************/
uint32_t storage_has_user_config(void)
{
  uint8_t rxKeyData[PARAM_NVM_KEYSECTION_SIZE];
  uint32_t nvm_Key;
  uint32_t dataStatus=0; 
  
  //Clear rxUserData buffer
  memset(rxKeyData, 0x00, PARAM_NVM_KEYSECTION_SIZE);
  //Let's read g_Espresso_user_config_s (65bytes) from the NVM
  spi_NVMemoryRead(PARAM_NVM_PAGE_ADD, 
                  PARAM_NVM_KEYSECTION_ADD, 
                  PARAM_NVM_KEYSECTION_SIZE, 
                  &rxKeyData[0]);

  //extract NVM KEY to determine if memory has already data or not 
  unpack_u32_from_strg_bytes((uint8_t *)&rxKeyData[0],&nvm_Key);
  
  if(nvm_Key == PARAM_NVM_MEM_KEY)
  {
    dataStatus = STORAGE_USERDATA_LOADED;
  }else if(nvm_Key == PARAM_NVM_EMPTY_DATA) 
  {
    dataStatus = STORAGE_USERDATA_EMPTY;
  }
  return dataStatus;
}

/*****************************************************************************
* Function: 	storage_load_user_config
* Description:  Reads data from the external memory and store it the UserData Pointer
* Return:       STORAGE_USERDATA_LOADED = memory read success
                STORAGE_USERDATA_ERROR = Could not read nvm or 0xff
*****************************************************************************/
uint32_t storage_load_user_config(espresso_user_config_t* ptr_rxData)
{
  uint8_t rxUserData[PARAM_NVM_USERDATA_SIZE];
  uint32_t nvm_Key, nvm_wCycle;
  volatile uint16_t wCycleShotprofile=0;
  volatile uint16_t wCycleCtrlProfile=0;
  uint32_t dataStatus=0xFFFFFFFF;
  float tempfvar;

  //Clear rxUserData buffer
  memset(rxUserData, 0x00, PARAM_NVM_USERDATA_SIZE);
  //Read entire user data section
  spi_NVMemoryRead( PARAM_NVM_PAGE_ADD, 
                    PARAM_NVM_PAGE_OFFSET, 
                    PARAM_NVM_USERDATA_SIZE, 
                    &rxUserData[PARAM_NVM_USERDATA_ADD]);
  //extract NVM KEY to determine if memory has already data or not 
  unpack_u32_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_NVM_FTKEY],&nvm_Key);
  ptr_rxData->nvmKey = nvm_Key;
  
  if(nvm_Key == PARAM_NVM_MEM_KEY)
  {
    //Key is already stored in nvm, proceed to store new data
    unpack_u32_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_NVM_WCYCLE],&nvm_wCycle);
    ptr_rxData->nvmWcycles = nvm_wCycle;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_TARGETBOILER_TMP],(float*)&tempfvar);
    ptr_rxData->boilerTempSetpointDegC = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_BREWPREINFUSSION_PWR],(float*)&tempfvar);
    ptr_rxData->profPreInfusePwr = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_BREWPREINFUSSION_TMR],(float*)&tempfvar);
    ptr_rxData->profPreInfuseTmr = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_BREWINFUSSION_PWR],(float*)&tempfvar);
    ptr_rxData->profInfusePwr = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_BREWINFUSSION_TMR],(float*)&tempfvar);
    ptr_rxData->profInfuseTmr = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_BREWDECLINING_PWR],(float*)&tempfvar);
    ptr_rxData->profTaperingPwr = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_BREWDECLINING_TMR],(float*)&tempfvar);
    ptr_rxData->profTaperingTmr = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_PID_PTERM],(float*)&tempfvar);
    ptr_rxData->pidPTerm = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_PID_ITERM],(float*)&tempfvar);
    ptr_rxData->pidITerm = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_PID_IMAXTERM],(float*)&tempfvar);
    ptr_rxData->pidImaxTerm = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_PID_DTERM],(float*)&tempfvar);
    ptr_rxData->pidDTerm = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_PID_DLPFTERM],(float*)&tempfvar);
    ptr_rxData->pidDlpfTerm = tempfvar;

    unpack_float_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_PID_GAINTERM],(float*)&tempfvar);
    ptr_rxData->pidGainTerm = tempfvar;

    ptr_rxData->pidIwindupTerm = rxUserData[BE_USERDATA_PID_IWINDUPTERM];
    dataStatus=STORAGE_USERDATA_LOADED;
    /* M3 fix: validate all deserialized float fields and clamp out-of-range
     * values to safe defaults.  Protects against corrupted flash data. */
    validate_clamp_data(ptr_rxData);
  }else{
    dataStatus=STORAGE_USERDATA_EMPTY;
  }
  return dataStatus;
}

/*****************************************************************************
* Function: 	storage_save_shot_profile
* Description:  wrtie NEW Espresso profile into NVM  
* Return:       STORAGE_PROFILEDATA_STORED
*****************************************************************************/
uint32_t storage_save_shot_profile(espresso_user_config_t* ptr_sxData)
{
  uint8_t txUserData[PARAM_NVM_USERDATA_SIZE];
  uint8_t rxUserData[PARAM_NVM_USERDATA_SIZE];
  uint32_t nvm_Key, nvm_wCycle;
  volatile uint16_t wCycleShotprofile=0;
  volatile uint16_t wCycleCtrlProfile=0;
  uint32_t dataStatus=0xFFFFFFFF;
  float tempfvar;
  
  //Clear txUserData buffer
  memset(txUserData, 0x00, PARAM_NVM_USERDATA_SIZE);
  //Clear rxUserData buffer
  memset(rxUserData, 0x00, PARAM_NVM_USERDATA_SIZE);
  //Read entire user data section
  spi_NVMemoryRead( PARAM_NVM_PAGE_ADD, 
                    PARAM_NVM_PAGE_OFFSET, 
                    PARAM_NVM_USERDATA_SIZE, 
                    &rxUserData[PARAM_NVM_USERDATA_ADD]);
  //extract NVM KEY to determine if memory has already data or not 
  unpack_u32_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_NVM_FTKEY],&nvm_Key);
  if(nvm_Key == PARAM_NVM_MEM_KEY)
  {
    //Key is already stored in nvm, proceed to store new data
    dataStatus=STORAGE_USERDATA_STORED;
    //Copy the read-keyd into the TX string
    mempcpy(&txUserData[PARAM_NVM_KEYSECTION_ADD],
            &rxUserData[PARAM_NVM_KEYSECTION_ADD],
            PARAM_NVM_KEYSECTION_SIZE);
  }else if(nvm_Key == PARAM_NVM_EMPTY_DATA){
    //Firt data to be stored in the nvm, let's write the key into it as well
    dataStatus=STORAGE_USERDATA_FIRSTW;
    txUserData[PARAM_NVM_KEYSECTION_ADD+0] = 0xAA; 
    txUserData[PARAM_NVM_KEYSECTION_ADD+1] = 0x00; 
    txUserData[PARAM_NVM_KEYSECTION_ADD+2] = 0xAA; 
    txUserData[PARAM_NVM_KEYSECTION_ADD+3] = 0x00;
  }else{
    //something went wrong
    dataStatus=STORAGE_USERDATA_ERROR;
  }

  if(dataStatus!=STORAGE_USERDATA_ERROR)
  {
    //read writen values from 32bit variable
    unpack_u32_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_NVM_WCYCLE],&nvm_wCycle);
    //extract writen cycles from 32bit register into two 16bits variables
    wCycleShotprofile = (uint16_t)((nvm_wCycle)>>16);
    wCycleCtrlProfile = (uint16_t)((nvm_wCycle) & 0x00FF);
    //increase the Write Cycles var for the profile shot by 1
    wCycleShotprofile++;
    //add new value into TX string
    txUserData[BE_USERDATA_NVM_WCYCLE+3] = (uint8_t)(wCycleShotprofile>>8); 
    txUserData[BE_USERDATA_NVM_WCYCLE+2] = (uint8_t)(wCycleShotprofile & 0x00FF); 
    txUserData[BE_USERDATA_NVM_WCYCLE+1] = (uint8_t)(wCycleCtrlProfile>>8); 
    txUserData[BE_USERDATA_NVM_WCYCLE+0] = (uint8_t)(wCycleCtrlProfile & 0x00FF);
    //Copy the block section that will not be updated into the TX string (Controller profile)
    mempcpy(&txUserData[PARAM_NVM_CONTROLLER_ADD],
            &rxUserData[PARAM_NVM_CONTROLLER_ADD],
            PARAM_NVM_CONTROLLER_SIZE);
    //converting and pasting the new shot profule into TX string
    pack_float_to_strg_bytes(ptr_sxData->boilerTempSetpointDegC,    (uint8_t *)&txUserData[BE_USERDATA_TARGETBOILER_TMP]);
    txUserData[BE_USERDATA_RSVD+0] = 0x00; 
    txUserData[BE_USERDATA_RSVD+1] = 0x00; 
    txUserData[BE_USERDATA_RSVD+2] = 0x00; 
    txUserData[BE_USERDATA_RSVD+3] = 0x00; 
    pack_float_to_strg_bytes((float)ptr_sxData->profPreInfusePwr, (uint8_t *)&txUserData[BE_USERDATA_BREWPREINFUSSION_PWR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profPreInfuseTmr, (uint8_t *)&txUserData[BE_USERDATA_BREWPREINFUSSION_TMR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profInfusePwr,    (uint8_t *)&txUserData[BE_USERDATA_BREWINFUSSION_PWR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profInfuseTmr,    (uint8_t *)&txUserData[BE_USERDATA_BREWINFUSSION_TMR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profTaperingPwr,    (uint8_t *)&txUserData[BE_USERDATA_BREWDECLINING_PWR]);
    pack_float_to_strg_bytes((float)ptr_sxData->profTaperingTmr,    (uint8_t *)&txUserData[BE_USERDATA_BREWDECLINING_TMR]);
    //Write entire user data block into nvm
    spi_NVMemoryWritePage(PARAM_NVM_PAGE_ADD, 
                          PARAM_NVM_PAGE_OFFSET, 
                          PARAM_NVM_USERDATA_SIZE, 
                          &txUserData[PARAM_NVM_USERDATA_ADD]);
    dataStatus=STORAGE_PROFILEDATA_STORED;
  }
  return dataStatus;
}

/*****************************************************************************
* Function: 	storage_save_controller_config
* Description:  wrtie NEW controller profile into NVM
* Return:       STORAGE_CONTROLLERDATA_STORED
*****************************************************************************/
uint32_t storage_save_controller_config(espresso_user_config_t* ptr_sxData)
{
  uint8_t txUserData[PARAM_NVM_USERDATA_SIZE];
  uint8_t rxUserData[PARAM_NVM_USERDATA_SIZE];
  uint32_t nvm_Key, nvm_wCycle;
  volatile uint16_t wCycleShotprofile=0;
  volatile uint16_t wCycleCtrlProfile=0;
  uint32_t dataStatus=0xFFFFFFFF;
  float tempfvar;
  
  //Clear txUserData buffer
  memset(txUserData, 0x00, PARAM_NVM_USERDATA_SIZE);
  //Clear rxUserData buffer
  memset(rxUserData, 0x00, PARAM_NVM_USERDATA_SIZE);
  //Read entire user data section
  spi_NVMemoryRead( PARAM_NVM_PAGE_ADD, 
                    PARAM_NVM_PAGE_OFFSET, 
                    PARAM_NVM_USERDATA_SIZE, 
                    &rxUserData[PARAM_NVM_USERDATA_ADD]);
  //extract NVM KEY to determine if memory has already data or not 
  unpack_u32_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_NVM_FTKEY],&nvm_Key);
  if(nvm_Key == PARAM_NVM_MEM_KEY)
  {
    //Key is already stored in nvm, proceed to store new data
    dataStatus=STORAGE_USERDATA_STORED;
    //Copy the read-keyd into the TX string
    mempcpy(&txUserData[PARAM_NVM_KEYSECTION_ADD],
            &rxUserData[PARAM_NVM_KEYSECTION_ADD],
            PARAM_NVM_KEYSECTION_SIZE);
  }else if(nvm_Key == PARAM_NVM_EMPTY_DATA){
    //Firt data to be stored in the nvm, let's write the key into it as well
    dataStatus=STORAGE_USERDATA_FIRSTW;
    txUserData[PARAM_NVM_KEYSECTION_ADD+0] = 0xAA; 
    txUserData[PARAM_NVM_KEYSECTION_ADD+1] = 0x00; 
    txUserData[PARAM_NVM_KEYSECTION_ADD+2] = 0xAA; 
    txUserData[PARAM_NVM_KEYSECTION_ADD+3] = 0x00;
  }else{
    //something went wrong
    dataStatus=STORAGE_USERDATA_ERROR;
  }

  if(dataStatus!=STORAGE_USERDATA_ERROR)
  {
    //read writen values from 32bit variable
    unpack_u32_from_strg_bytes((uint8_t *)&rxUserData[BE_USERDATA_NVM_WCYCLE],&nvm_wCycle);
    //extract writen cycles from 32bit register into two 16bits variables
    wCycleShotprofile = (uint16_t)((nvm_wCycle)>>16);
    wCycleCtrlProfile = (uint16_t)((nvm_wCycle) & 0x00FF);
    //increase the Write Cycles var for the controller profile by 1
    wCycleCtrlProfile++;
    //add new value into TX string
    txUserData[BE_USERDATA_NVM_WCYCLE+3] = (uint8_t)(wCycleShotprofile>>8); 
    txUserData[BE_USERDATA_NVM_WCYCLE+2] = (uint8_t)(wCycleShotprofile & 0x00FF); 
    txUserData[BE_USERDATA_NVM_WCYCLE+1] = (uint8_t)(wCycleCtrlProfile>>8); 
    txUserData[BE_USERDATA_NVM_WCYCLE+0] = (uint8_t)(wCycleCtrlProfile & 0x00FF);
    //Copy the block section that will not be updated into the TX string (shot profile)
    mempcpy(&txUserData[PARAM_NVM_SHOTPROFILE_ADD],
            &rxUserData[PARAM_NVM_SHOTPROFILE_ADD],
            PARAM_NVM_SHOTPROFILE_SIZE);
    //converting and pasting the new controller profule into TX string
    pack_float_to_strg_bytes(ptr_sxData->pidPTerm,          (uint8_t *)&txUserData[BE_USERDATA_PID_PTERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidITerm,          (uint8_t *)&txUserData[BE_USERDATA_PID_ITERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidImaxTerm,       (uint8_t *)&txUserData[BE_USERDATA_PID_IMAXTERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidDTerm,          (uint8_t *)&txUserData[BE_USERDATA_PID_DTERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidDlpfTerm,       (uint8_t *)&txUserData[BE_USERDATA_PID_DLPFTERM]);
    pack_float_to_strg_bytes(ptr_sxData->pidGainTerm,       (uint8_t *)&txUserData[BE_USERDATA_PID_GAINTERM]);
    txUserData[BE_USERDATA_PID_IWINDUPTERM] = (ptr_sxData->pidIwindupTerm);
    //Write entire user data block into nvm
    spi_NVMemoryWritePage(PARAM_NVM_PAGE_ADD, 
                          PARAM_NVM_PAGE_OFFSET, 
                          PARAM_NVM_USERDATA_SIZE, 
                          &txUserData[PARAM_NVM_USERDATA_ADD]);
    dataStatus=STORAGE_CONTROLLERDATA_STORED;
  }
  return dataStatus;

}

/*****************************************************************************
* Function: 	storage_print_user_config
* Description:  Print into UART the data contain is *ptr_rxData Pointer
* Return:       STORAGE_USERDATA_PRINTED
*****************************************************************************/
uint32_t storage_print_user_config(espresso_user_config_t* ptr_rxData)
{
  uint16_t wCycleShotprofile;
  uint16_t wCycleCtrlProfile;

  wCycleShotprofile = (uint16_t)((ptr_rxData->nvmWcycles)>>16);
  wCycleCtrlProfile = (uint16_t)((ptr_rxData->nvmWcycles) & 0x00FF);

  //Print: uint32_t nvmWcycles;
  NRF_LOG_DEBUG("NVM Profile- # of Write cycles:  %d\r\n", 
                (wCycleShotprofile));
  NRF_LOG_DEBUG("NVM CTRL-    # of Write cycles:  %d\r\n", 
                (wCycleCtrlProfile));
  //Print: uint32_t nvmKey;

  
  //Print: float boilerTempSetpointDegC;
  NRF_LOG_DEBUG("Boiler-      Target Temp:        " NRF_LOG_FLOAT_MARKER "  C\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->boilerTempSetpointDegC));
  //Print: float steamTempDegC;
  NRF_LOG_DEBUG("Boiler-      Temp:               " NRF_LOG_FLOAT_MARKER "  C\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->steamTempDegC));
  //Print: float profPreInfusePwr;
  NRF_LOG_DEBUG("Profile-     PreInfuse Power:    " NRF_LOG_FLOAT_MARKER "  %%\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->profPreInfusePwr));
  //Print: float profPreInfuseTmr;
  NRF_LOG_DEBUG("Profile-     PreInfuse Timer:    " NRF_LOG_FLOAT_MARKER "  s\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->profPreInfuseTmr));
  //Print: float profInfusePwr;
  NRF_LOG_DEBUG("Profile-     Infuse Power:       " NRF_LOG_FLOAT_MARKER "  %%\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->profInfusePwr));
  //Print: float profInfuseTmr;
  NRF_LOG_DEBUG("Profile-     Infuse Timer:       " NRF_LOG_FLOAT_MARKER "  s\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->profInfuseTmr));
  //Print: float profTaperingPwr;
  NRF_LOG_DEBUG("Profile-     Declining Power:    " NRF_LOG_FLOAT_MARKER "  %\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->profTaperingPwr));
  //Print: float profTaperingTmr;
  NRF_LOG_DEBUG("Profile-     Declining Timer:    " NRF_LOG_FLOAT_MARKER "  s\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->profTaperingTmr));

  //Print: float pidPTerm;
  NRF_LOG_DEBUG("Controller-  Proportial Gain:    " NRF_LOG_FLOAT_MARKER "\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->pidPTerm));
  //Print: float pidITerm;
  NRF_LOG_DEBUG("Controller-  Integral Gain:      " NRF_LOG_FLOAT_MARKER "\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->pidITerm));
  //Print: float pidImaxTerm;
  NRF_LOG_DEBUG("Controller-  Max Integral Value: " NRF_LOG_FLOAT_MARKER "\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->pidImaxTerm));
  //Print: bool  pidIwindupTerm;
  if(ptr_rxData->pidIwindupTerm)
  {
    NRF_LOG_DEBUG("Controller-  Integral Windup:    ENABLE\r\n");
  }else{
    NRF_LOG_DEBUG("Controller-  Integral Windup:    DISABLE\r\n");
  }
  //Print: float pidDTerm;
  NRF_LOG_DEBUG("Controller-  Derivative Gain:    " NRF_LOG_FLOAT_MARKER "\r\n", 
                NRF_LOG_FLOAT(ptr_rxData->pidDTerm));
  //Print: float pidDlpfTerm;
  NRF_LOG_DEBUG("Controller-  Derivate Filter:    " NRF_LOG_FLOAT_MARKER " Hz \r\n", 
                NRF_LOG_FLOAT(ptr_rxData->pidDlpfTerm));
  //Print: float pidGainTerm;
  NRF_LOG_DEBUG("Controller-  PID Gain:           NOT USED!");
                
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
void unpack_float_from_strg_bytes(uint8_t* ptr_Fbytes, float* ptr_Fnumber)
{
  volatile uint32_t hexTemp = 0x00000000;;
  hexTemp = ((uint32_t)(*(ptr_Fbytes+3)<<24) &0xFF000000) |
            ((uint32_t)(*(ptr_Fbytes+2)<<16) &0x00FF0000) |
            ((uint32_t)(*(ptr_Fbytes+1)<<8)  &0x0000FF00) |
            ((uint32_t)  *ptr_Fbytes         &0x000000FF) ;
  *ptr_Fnumber = *((float *)&hexTemp); 
}

/*****************************************************************************
* Function: 	unpack_u32_from_strg_bytes
* Hint:         Parsing	= Convert from string/binary to data structures
* Description:  Provide the add of the bytes array & pointer to the float where to store the result of parsing
* Return:       
*****************************************************************************/
void unpack_u32_from_strg_bytes(uint8_t* ptr_Fbytes, uint32_t* ptr_number)
{
  * ptr_number = ((uint32_t)(*(ptr_Fbytes+3)<<24)  &0xFF000000) |
                  ((uint32_t)(*(ptr_Fbytes+2)<<16) &0x00FF0000) |
                  ((uint32_t)(*(ptr_Fbytes+1)<<8)  &0x0000FF00) |
                  ((uint32_t)  *ptr_Fbytes         &0x000000FF) ;
}

/*****************************************************************************
* Function: 	pack_float_to_strg_bytes
* Hint:         Serialization/Encoding = Convert from data structures to string/binary
* Description:  Provide the Float number and the pointer to array where the hex value of the float is going to be stored.
* Return:       
*****************************************************************************/
void pack_float_to_strg_bytes(float fnumber, uint8_t* ptr_Fbytes)
{
  static uint32_t hexTemp = 0x00000000;
  //  pointer cast to get raw bits of float
  hexTemp = *((uint32_t *)&fnumber);

  *ptr_Fbytes++ = (uint8_t)(hexTemp & 0xFF);         // LSB
  *ptr_Fbytes++ = (uint8_t)((hexTemp >> 8) & 0xFF);
  *ptr_Fbytes++ = (uint8_t)((hexTemp >> 16) & 0xFF);
  *ptr_Fbytes =   (uint8_t)((hexTemp >> 24) & 0xFF);  // MSB
}


/* ---------------------------------------------------------------------------
 * validate_clamp_data
 *
 * Validates every user-configurable float field in the profile struct.
 * Out-of-range or non-finite values are replaced with safe defaults.
 * Returns PROFILE_VALID if no corrections were needed, PROFILE_CLAMPED if
 * at least one field was corrected.
 * --------------------------------------------------------------------------- */
profile_validation_status_t validate_clamp_data(
    espresso_user_config_t *profile)
{
    bool all_valid = true;

    /* ---- Temperature setpoints ---- */
    all_valid &= fcn_ValidateFloat_InRange(&profile->boilerTempSetpointDegC,       20.0f, 110.0f,  95.5f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->brewTempDegC,       20.0f, 110.0f,  95.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->steamTempDegC,      100.0f, 160.0f, 110.0f);

    /* ---- Brew profile — power (0–100 %) ---- */
    all_valid &= fcn_ValidateFloat_InRange(&profile->profPreInfusePwr,  0.0f, 100.0f,  75.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->profInfusePwr,     0.0f, 100.0f, 100.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->profTaperingPwr,    0.0f, 100.0f,  85.0f);

    /* ---- Brew profile — timers (seconds) ---- */
    all_valid &= fcn_ValidateFloat_InRange(&profile->profPreInfuseTmr,  0.0f,  15.0f,  8.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->profInfuseTmr,     0.0f,  60.0f,  8.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->profTaperingTmr,    0.0f,  30.0f,  8.0f);

    /* ---- PID gains ---- */
    all_valid &= fcn_ValidateFloat_InRange(&profile->pidPTerm,         0.0f, 100.0f,   9.5f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->pidITerm,         0.0f,  10.0f,   0.3f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->pidIboostTerm,    0.0f,  20.0f,   6.5f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->pidImaxTerm,      0.0f, 500.0f, 100.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->pidDTerm,         0.0f,  50.0f,   0.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->pidDlpfTerm,      0.0f,   1.0f,   0.0f);
    all_valid &= fcn_ValidateFloat_InRange(&profile->pidGainTerm,     0.01f,  10.0f,   0.0f);

    return all_valid ? PROFILE_VALID : PROFILE_CLAMPED;
}