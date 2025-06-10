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
  ADDRESS MAP of: bleSpressoUserdata_struct 
  -----------------------------------------
  ADDR          Variable 
  -----------------------------------------
  0x00 - 0x03   uint32_t nvmWcycles -> (littleEndia)16bit MSB allocated for ShotProfile - 16b LSB allocated for Controller profile
  0x04 - 0x07   uint32_t nvmKey
  0x08 - 0x0B   float TargetBoilerTemp
  0x0C - 0x0F   float RSVD
  0x10 - 0x13   float BrewPreInfussionPwr
  0x14 - 0x17   float BrewPreInfussionTmr
  0x18 - 0x1B   float BrewInfussionPwr
  0x1C - 0x1F   float BrewInfussionTmr
  0x20 - 0x23   float BrewDecliningPwr
  0x24 - 0x27   float BrewDecliningTmr
  0x28 - 0x2B   float Pid_P_term
  0x2C - 0x2F   float Pid_I_term
  0x30 - 0x33   float Pid_Imax_term
  0x34 - 0x37   float Pid_D_term
  0x38 - 0x3B   float Pid_Dlpf_term
  0x3C - 0x3F   float Pid_Gain_term
  0x40          bool  Pid_Iwindup_term
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
void parsingBytesToFloat(uint8_t* ptr_Fbytes, float* ptr_Fnumber);
void parsingBytesTo32bitVar(uint8_t* ptr_Fbytes, uint32_t* ptr_number);
void encodeFloatToBytes(float fnumber, uint8_t* ptr_Fbytes);

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
* Function: 	stgCtrl_Init
* Description:  Inits the external spi memory by resetting it and reading its ID
* Return:       STORAGE_INIT_OK = ID match memory manufacturer 
*               STORAGE_INIT_ERROR = ID do NOT match memory manufacturer 
*****************************************************************************/
uint32_t stgCtrl_Init(void)
{
  return spim_initNVmemory();
}

/*****************************************************************************
* Function: 	stgCtrl_ChkForUserData
* Description:  Reads data from the external memory and look for USER data already
*               stored in it by checking = PARAM_NVM_MEM_KEY
* Return:       
*****************************************************************************/
uint32_t stgCtrl_ChkForUserData(void)
{
  uint8_t rxKeyData[PARAM_NVM_KEYSECTION_SIZE];
  uint32_t nvm_Key;
  uint32_t dataStatus=0; 
  
  //Clear rxUserData buffer
  memset(rxKeyData, 0x00, PARAM_NVM_KEYSECTION_SIZE);
  //Let's read BLEspressoVar (65bytes) from the NVM
  spi_NVMemoryRead(PARAM_NVM_PAGE_ADD, 
                  PARAM_NVM_KEYSECTION_ADD, 
                  PARAM_NVM_KEYSECTION_SIZE, 
                  &rxKeyData[0]);

  //extract NVM KEY to determine if memory has already data or not 
  parsingBytesTo32bitVar((uint8_t *)&rxKeyData[0],&nvm_Key);
  
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
* Function: 	stgCtrl_ReadUserData
* Description:  Reads data from the external memory and store it the UserData Pointer
* Return:       STORAGE_USERDATA_LOADED = memory read success
                STORAGE_USERDATA_ERROR = Could not read nvm or 0xff
*****************************************************************************/
uint32_t stgCtrl_ReadUserData(bleSpressoUserdata_struct* ptr_rxData)
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
  parsingBytesTo32bitVar((uint8_t *)&rxUserData[BE_USERDATA_NVM_FTKEY],&nvm_Key);
  ptr_rxData->nvmKey = nvm_Key;
  
  if(nvm_Key == PARAM_NVM_MEM_KEY)
  {
    //Key is already stored in nvm, proceed to store new data
    parsingBytesTo32bitVar((uint8_t *)&rxUserData[BE_USERDATA_NVM_WCYCLE],&nvm_wCycle);
    ptr_rxData->nvmWcycles = nvm_wCycle;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_TARGETBOILER_TMP],(float*)&tempfvar);
    ptr_rxData->TargetBoilerTemp = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_BREWPREINFUSSION_PWR],(float*)&tempfvar);
    ptr_rxData->BrewPreInfussionPwr = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_BREWPREINFUSSION_TMR],(float*)&tempfvar);
    ptr_rxData->BrewPreInfussionTmr = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_BREWINFUSSION_PWR],(float*)&tempfvar);
    ptr_rxData->BrewInfussionPwr = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_BREWINFUSSION_TMR],(float*)&tempfvar);
    ptr_rxData->BrewInfussionTmr = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_BREWDECLINING_PWR],(float*)&tempfvar);
    ptr_rxData->BrewDecliningPwr = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_BREWDECLINING_TMR],(float*)&tempfvar);
    ptr_rxData->BrewDecliningTmr = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_PID_PTERM],(float*)&tempfvar);
    ptr_rxData->Pid_P_term = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_PID_ITERM],(float*)&tempfvar);
    ptr_rxData->Pid_I_term = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_PID_IMAXTERM],(float*)&tempfvar);
    ptr_rxData->Pid_Imax_term = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_PID_DTERM],(float*)&tempfvar);
    ptr_rxData->Pid_D_term = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_PID_DLPFTERM],(float*)&tempfvar);
    ptr_rxData->Pid_Dlpf_term = tempfvar;

    parsingBytesToFloat((uint8_t *)&rxUserData[BE_USERDATA_PID_GAINTERM],(float*)&tempfvar);
    ptr_rxData->Pid_Gain_term = tempfvar;

    ptr_rxData->Pid_Iwindup_term = rxUserData[BE_USERDATA_PID_IWINDUPTERM];
    dataStatus=STORAGE_USERDATA_LOADED;
  }else{
    dataStatus=STORAGE_USERDATA_EMPTY;
  }
  return dataStatus;
}

/*****************************************************************************
* Function: 	stgCtrl_StoreShotProfileData
* Description:  wrtie NEW Espresso profile into NVM  
* Return:       STORAGE_PROFILEDATA_STORED
*****************************************************************************/
uint32_t stgCtrl_StoreShotProfileData(bleSpressoUserdata_struct* ptr_sxData)
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
  parsingBytesTo32bitVar((uint8_t *)&rxUserData[BE_USERDATA_NVM_FTKEY],&nvm_Key);
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
    parsingBytesTo32bitVar((uint8_t *)&rxUserData[BE_USERDATA_NVM_WCYCLE],&nvm_wCycle);
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
    encodeFloatToBytes(ptr_sxData->TargetBoilerTemp,    (uint8_t *)&txUserData[BE_USERDATA_TARGETBOILER_TMP]);
    txUserData[BE_USERDATA_RSVD+0] = 0x00; 
    txUserData[BE_USERDATA_RSVD+1] = 0x00; 
    txUserData[BE_USERDATA_RSVD+2] = 0x00; 
    txUserData[BE_USERDATA_RSVD+3] = 0x00; 
    encodeFloatToBytes((float)ptr_sxData->BrewPreInfussionPwr, (uint8_t *)&txUserData[BE_USERDATA_BREWPREINFUSSION_PWR]);
    encodeFloatToBytes((float)ptr_sxData->BrewPreInfussionTmr, (uint8_t *)&txUserData[BE_USERDATA_BREWPREINFUSSION_TMR]);
    encodeFloatToBytes((float)ptr_sxData->BrewInfussionPwr,    (uint8_t *)&txUserData[BE_USERDATA_BREWINFUSSION_PWR]);
    encodeFloatToBytes((float)ptr_sxData->BrewInfussionTmr,    (uint8_t *)&txUserData[BE_USERDATA_BREWINFUSSION_TMR]);
    encodeFloatToBytes((float)ptr_sxData->BrewDecliningPwr,    (uint8_t *)&txUserData[BE_USERDATA_BREWDECLINING_PWR]);
    encodeFloatToBytes((float)ptr_sxData->BrewDecliningTmr,    (uint8_t *)&txUserData[BE_USERDATA_BREWDECLINING_TMR]);
    //Write entire user data block into nvm
    spi_NVMemoryWritePage(PARAM_NVM_PAGE_ADD, 
                          PARAM_NVM_PAGE_OFFSET, 
                          PARAM_NVM_USERDATA_SIZE, 
                          &txUserData[PARAM_NVM_USERDATA_ADD]);
  }
  return dataStatus;
}

/*****************************************************************************
* Function: 	stgCtrl_StoreControllerData
* Description:  wrtie NEW controller profile into NVM
* Return:       STORAGE_CONTROLLERDATA_STORED
*****************************************************************************/
uint32_t stgCtrl_StoreControllerData(bleSpressoUserdata_struct* ptr_sxData)
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
  parsingBytesTo32bitVar((uint8_t *)&rxUserData[BE_USERDATA_NVM_FTKEY],&nvm_Key);
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
    parsingBytesTo32bitVar((uint8_t *)&rxUserData[BE_USERDATA_NVM_WCYCLE],&nvm_wCycle);
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
    encodeFloatToBytes(ptr_sxData->Pid_P_term,          (uint8_t *)&txUserData[BE_USERDATA_PID_PTERM]);
    encodeFloatToBytes(ptr_sxData->Pid_I_term,          (uint8_t *)&txUserData[BE_USERDATA_PID_ITERM]);
    encodeFloatToBytes(ptr_sxData->Pid_Imax_term,       (uint8_t *)&txUserData[BE_USERDATA_PID_IMAXTERM]);
    encodeFloatToBytes(ptr_sxData->Pid_D_term,          (uint8_t *)&txUserData[BE_USERDATA_PID_DTERM]);
    encodeFloatToBytes(ptr_sxData->Pid_Dlpf_term,       (uint8_t *)&txUserData[BE_USERDATA_PID_DLPFTERM]);
    encodeFloatToBytes(ptr_sxData->Pid_Gain_term,       (uint8_t *)&txUserData[BE_USERDATA_PID_GAINTERM]);
    txUserData[BE_USERDATA_PID_IWINDUPTERM] = (ptr_sxData->Pid_Iwindup_term);
    //Write entire user data block into nvm
    spi_NVMemoryWritePage(PARAM_NVM_PAGE_ADD, 
                          PARAM_NVM_PAGE_OFFSET, 
                          PARAM_NVM_USERDATA_SIZE, 
                          &txUserData[PARAM_NVM_USERDATA_ADD]);
  }
  return dataStatus;

}

 
//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
* Function: 	parsingBytesToFloat
* Hint:         Parsing	= Convert from string/binary to data structures
* Description:  Provide the add of the bytes array & pointer to the float where to store the result of parsing
* Return:       
*****************************************************************************/
void parsingBytesToFloat(uint8_t* ptr_Fbytes, float* ptr_Fnumber)
{
  volatile uint32_t hexTemp = 0x00000000;;
  hexTemp = ((uint32_t)(*(ptr_Fbytes+3)<<24) &0xFF000000) |
            ((uint32_t)(*(ptr_Fbytes+2)<<16) &0x00FF0000) |
            ((uint32_t)(*(ptr_Fbytes+1)<<8)  &0x0000FF00) |
            ((uint32_t)  *ptr_Fbytes         &0x000000FF) ;
  *ptr_Fnumber = *((float *)&hexTemp); 
}

/*****************************************************************************
* Function: 	parsingBytesTo32bitVar
* Hint:         Parsing	= Convert from string/binary to data structures
* Description:  Provide the add of the bytes array & pointer to the float where to store the result of parsing
* Return:       
*****************************************************************************/
void parsingBytesTo32bitVar(uint8_t* ptr_Fbytes, uint32_t* ptr_number)
{
  * ptr_number = ((uint32_t)(*(ptr_Fbytes+3)<<24)  &0xFF000000) |
                  ((uint32_t)(*(ptr_Fbytes+2)<<16) &0x00FF0000) |
                  ((uint32_t)(*(ptr_Fbytes+1)<<8)  &0x0000FF00) |
                  ((uint32_t)  *ptr_Fbytes         &0x000000FF) ;
}

/*****************************************************************************
* Function: 	encodeFloatToBytes
* Hint:         Serialization/Encoding = Convert from data structures to string/binary
* Description:  Provide the Float number and the pointer to array where the hex value of the float is going to be stored.
* Return:       
*****************************************************************************/
void encodeFloatToBytes(float fnumber, uint8_t* ptr_Fbytes)
{
  static uint32_t hexTemp = 0x00000000;
  //  pointer cast to get raw bits of float
  hexTemp = *((uint32_t *)&fnumber);

  *ptr_Fbytes++ = (uint8_t)(hexTemp & 0xFF);         // LSB
  *ptr_Fbytes++ = (uint8_t)((hexTemp >> 8) & 0xFF);
  *ptr_Fbytes++ = (uint8_t)((hexTemp >> 16) & 0xFF);
  *ptr_Fbytes =   (uint8_t)((hexTemp >> 24) & 0xFF);  // MSB
}