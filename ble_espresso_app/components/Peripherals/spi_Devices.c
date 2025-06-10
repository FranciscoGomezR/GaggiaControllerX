/*
    SPI DEVICES:
    1 - MAX31865 -  RTD-to-Digital Converter    
    2 - W25Q64FV -  64M-BIT SERIAL FLASH MEMORY
*/
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "spi_Devices.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_drv_timer.h"
#include "app_error.h"
#include "x01_StateMachineControls.h"

//*****************************************************************************
//
//			PRIVATE DEFINES SECTION - OWN BY THIS MODULE ONLY
//
//*****************************************************************************
#define SPI_INSTANCE  1 /**< SPI instance index. */

/* Quadratic formula's coefficients */
/* Reference links:
 1- https://www.mouser.com/pdfDocs/AN7186.pdf
 2- https://www.ti.com/lit/an/sbaa275a/sbaa275a.pdf?ts=1688395022605&ref_url=https%253A%252F%252Fwww.google.com%252F
 */
#define rtdAcoeff             +0.0039083f
#define rtdBcoeff             -0.000000577500f
#define rtdCcoeff             -0.00000000000418301f

#define rtdAxAcoeff           (float)(rtdAcoeff * rtdAcoeff)
#define rtd4xBcoeff           (float)(4.0f * rtdBcoeff)
#define rtd2xBcoeff           (float)(2.0f * rtdBcoeff)


#define SPI_NVM_CS_PIN        15
#define SPI_NVM_WP_PIN        17     
#define SPI_NVM_HD_PIN        18

#define TMP_CONFIG_REG_VALUE  0xD1


/* 
WP = 0
When /WP pin is low the Status Register locked and can not
be written to.
WP = 1 
When /WP pin is high the Status register is unlocked and can
be written to after a Write Enable instruction, WEL=1.*/

/* /HOLD pin
The /HOLD pin allows the device to be paused while it is actively selected.
HOLD = 0
When /HOLD is brought low,while /CS is low, the DO pin will be at high impedance and signals on the DI and CLK pins will be ignored(don’t care). 
HOLD = 1
When /HOLD is brought high, device operation can resume
*/

/* NVM DEVICE SECTION  */
#define NVM_MFR_FLASH_ID        0xEF    //Winbond Serial Flash  
#define NVM_W25Q16JV_IQ_ID      0x4015  //W25Q16JV-IQ/JQ  ID
#define NVM_W25Q16JV_IM_ID      0x7015  //W25Q16JV-IM*/JM*  ID
#define NVM_W25Q64_SPI_ID       0x4017  //W25Q64FV (SPI)  ID
#define NVM_W25Q64_QSPI_ID      0x6017  //W25Q64FV (QPI)*  ID

#define NVM_CMD_RESET_EN        0x66
#define NVM_CMD_RESET           0x99
#define NVM_CMD_JEDEC_ID        0x9F    //Read JEDEC ID
#define NVM_CMD_WR_ENABLE       0x06    //Write Enable
#define NVM_CMD_WR_DISABLE      0x04    //Write Disable
#define NVM_CMD_READ            0x03    //Read data 
#define NVM_CMD_FAST_READ       0x0B    //Fast Read
#define NVM_CMD_PAGE_WR         0x02    //Page Program
#define NVM_CMD_SECTOR_ER       0x20    //Sector Erase

//*****************************************************************************
//
//			PRIVATE STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
static float rtdTemperature; 

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */

static uint8_t       nvm_tx_buf[256];    /**< TX buffe. */
static uint8_t       nvm_rx_buf[256+4];   /**< RX buffer maximum 1 page; 4byte for CMD+ADD */

static const uint8_t tmp_buf_len = 3;   /**< Transfer length. */
static uint8_t       tmp_tx_buf[3];     /**< TX buffer for Temp sensor */
static uint8_t       tmp_rx_buf[3];     /**< RX buffer for Temp sensor*/


volatile uint8_t     hwreg_SensorConfig;
volatile float       resistanceRTD;

enum{
      sm_state0=0,
      sm_state1,
      sm_state2
      };

/* STATE MACHINE SECTION  */
StateMachineCtrl_Struct sm_SPIdevices;

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************
/* SPI DRIVER SECTION  */
void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context);
/* TEMP DEVICE SECTION  */
void spi_initTempDevCtrlPins(void);

/* NVM DEVICE SECTION  */
void spi_initNvmDevCtrlPins(void);
void spi_NVMemoryCMDreset(void);
uint32_t spi_NVMemoryReadJEDECID(void);
void spi_NVMemoryWriteEnable(void);
void spi_NVMemoryWriteDisable(void);

void spi_NVMemoryEraseSector(uint16_t noSector);
uint32_t NVMpageNoByteToW(uint32_t NoByte, uint16_t offset);

/* TEMP DEVICE SECTION  */
__STATIC_INLINE void spi_TempSensorCSenable(void);
__STATIC_INLINE void spi_TempSensorCSdisable(void);
/* NVM DEVICE SECTION  */
__STATIC_INLINE void spi_NVMemoryCSenable(void);
__STATIC_INLINE void spi_NVMemoryCSdisable(void);

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	spim_init
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void spim_init (void)
{
    /* SPI DRIVER SECTION  */
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = NRF_DRV_SPI_PIN_NOT_USED; //SPI_SS_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;
    spi_config.sck_pin  = SPI_SCK_PIN;
    spi_config.frequency = NRF_DRV_SPI_FREQ_1M;
    spi_config.mode     = NRF_DRV_SPI_MODE_3;
    spi_config.bit_order= NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));

    /* TEMP DEVICE SECTION  */
    spi_initTempDevCtrlPins();
    /* NVM DEVICE SECTION  */
    spi_initNvmDevCtrlPins();
    /* STATE MACHINE SECTION  */
    sm_SPIdevices.sRunning = sm_state0;
 }


/* TEMP DEVICE SECTION  */
/*****************************************************************************
* Function: 	spim_initRTDconverter
* Description:  configures the Temp sensor  by writing to reg: 80h(W) 
*               setting: VBIAS = ON; 
*               CONVERSION MODE = AUTO; 
*               SENSOR = 3-wire RTD; 
*               FILTER = 50 HZ
* Return:       if MCU reads back the same config value = 0xD1, init is ok
*****************************************************************************/
spi_Tmp_status_t spim_initRTDconverter(void)
{
  tmp_tx_buf[0] = 0x80;     //Configuration Register
  tmp_tx_buf[1] = 0xD1;     //Send configuration
  // Reset rx buffer and transfer done flag
  memset(tmp_rx_buf, 0, tmp_buf_len);
  spi_xfer_done = false;
  nrf_drv_gpiote_out_clear(SPI_SS_PIN);   //SPI-CTRL: Selecting Temp Device
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, tmp_tx_buf, 2, tmp_rx_buf, 0));
  while (!spi_xfer_done)
  {   __WFE();    }
  nrf_drv_gpiote_out_set(SPI_SS_PIN);     //SPI-CTRL: Unselecting Temp Device

  tmp_tx_buf[0] = 0x00;     //Read Configuration Register
  tmp_tx_buf[1] = 0xA0;     //Dummy register
  // Reset rx buffer and transfer done flag
  memset(tmp_rx_buf, 0, tmp_buf_len);
  spi_xfer_done = false;
  nrf_drv_gpiote_out_clear(SPI_SS_PIN);   //SPI-CTRL: Selecting Temp Device
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, tmp_tx_buf, 2, tmp_rx_buf, 2));
  while (!spi_xfer_done)
  {   __WFE();    }
  nrf_drv_gpiote_out_set(SPI_SS_PIN);     //SPI-CTRL: Unselecting Temp Device

  if( tmp_rx_buf[1] == TMP_CONFIG_REG_VALUE )
  { 
    return TMP_INIT_OK;
  }else{
    return TMP_INIT_ERROR;
  }
}

/* NVM DEVICE SECTION  */

/*****************************************************************************
* Function: 	spim_initNVmemory
* Description:  Inits the memory by resetting it and reading its ID
* Return:      
*****************************************************************************/
spi_nvm_status_t spim_initNVmemory(void)
{
  static uint32_t nvmMfrId;
  static uint32_t nvmJedecId;
  static uint32_t nvmMemTypeId;

  spi_NVMemoryCMDreset();
  nvmJedecId = spi_NVMemoryReadJEDECID();

  nvmMfrId = (nvmJedecId>>16) & 0x0000FF;
  nvmMemTypeId = nvmJedecId & 0x00FFFF;

  if(nvmMfrId == NVM_MFR_FLASH_ID)
  {
    return NVM_INIT_OK;
  }else{
    return NVM_INIT_NVM_ID_ERROR;
  }
}

 void spim_ReadRTDconverter(void)
 {
      uint16_t AdcConv;
      static float  sqrtTerm, Numerator, rtdTemp;
      switch(sm_SPIdevices.sRunning)
      {
          case sm_state0:
            tmp_tx_buf[0] = 0x01;     //Register's Address for: RTD MSBs
            tmp_tx_buf[1] = 0xAA; 
            tmp_tx_buf[2] = 0xAA;
            // Reset rx buffer and transfer done flag
            memset(tmp_rx_buf, 0, tmp_buf_len);
            spi_xfer_done = false;
            nrf_drv_gpiote_out_clear(SPI_SS_PIN);   //SPI-CTRL: Selecting Temp Device
            APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, tmp_tx_buf, tmp_buf_len, tmp_rx_buf, tmp_buf_len));
            sm_SPIdevices.sRunning = sm_state1;
          break;

          case sm_state1:
            if(spi_xfer_done == true)
            {
              nrf_drv_gpiote_out_set(SPI_SS_PIN);     //SPI-CTRL: Unselecting Temp Device
              AdcConv = (((uint16_t)tmp_rx_buf[1])<<8 | (uint16_t)tmp_rx_buf[2]);
              AdcConv = AdcConv >>1;
              resistanceRTD = (AdcConv * 430.0f)/32768.0f;
              //Numerator =  (float)1.0 - ((float)resistanceRTD / (float)100.0); 
              Numerator =  1.0f - ((float)resistanceRTD / 100.0f); 
              Numerator = (rtd4xBcoeff * Numerator);
              Numerator = (rtdAxAcoeff - Numerator);
              sqrtTerm = sqrtf(Numerator);
              Numerator =  (sqrtTerm - rtdAcoeff);
              rtdTemp = Numerator / rtd2xBcoeff;
              rtdTemperature = rtdTemp;
              sm_SPIdevices.sRunning = sm_state0;
             // sm_SPIdevices.sRunning = sm_state2;
            }else{}
          break;
      }      
 }


 bool spim_operation_done(void)
 {
    return spi_xfer_done;
 }

 /*****************************************************************************
 * Function: 	f_getBoilerTemperature
 * Description: Encapsulates public variable into "spi_sensors.c" module
 * Return:      temperature in float format
 *****************************************************************************/
 float f_getBoilerTemperature(void)
 {
    return rtdTemperature;
 }

//***********************************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//***********************************************************************************************

/*****************************************************************************
 * Function: 	spi_event_handler
 * Description: 
 *****************************************************************************/
void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    spi_xfer_done = true;
}

/*****************************************************************************
 * Function: 	spi_initTempDevCtrlPins
 * Description: Configure CS for the Temp sensor
 *****************************************************************************/
void spi_initTempDevCtrlPins(void)
{
   ret_code_t err_code_gpio;
   nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);

   /* TEMP CS pin */
   err_code_gpio = nrf_drv_gpiote_out_init(SPI_SS_PIN, &out_config);
   APP_ERROR_CHECK(err_code_gpio);

   /* Initail status*/
   nrf_drv_gpiote_out_set(SPI_SS_PIN);      //SPi memory is not selected
}

/*****************************************************************************
 * Function: 	spi_initNvmDevCtrlPins
 * Description: Configure CS, WP and HOLD pins for the NVM
 *****************************************************************************/
void spi_initNvmDevCtrlPins(void)
{
   ret_code_t err_code_gpio;
   nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);

   /* NVM CS pin */
   err_code_gpio = nrf_drv_gpiote_out_init(SPI_NVM_CS_PIN, &out_config);
   APP_ERROR_CHECK(err_code_gpio);

   /* Write Protection Pin*/
   err_code_gpio = nrf_drv_gpiote_out_init(SPI_NVM_WP_PIN, &out_config);
   APP_ERROR_CHECK(err_code_gpio);

   /* HOLD pin */
   err_code_gpio = nrf_drv_gpiote_out_init(SPI_NVM_HD_PIN, &out_config);
   APP_ERROR_CHECK(err_code_gpio);

   /* Initail status*/
   nrf_drv_gpiote_out_set(SPI_NVM_CS_PIN);      //SPi memory is not selected
   nrf_drv_gpiote_out_set(SPI_NVM_WP_PIN);    //Write protection is activated by HW
   nrf_drv_gpiote_out_set(SPI_NVM_HD_PIN);    //NVM is in hold (SPI is ignored)
}

/* TEMP DEVICE SECTION  */
__STATIC_INLINE void spi_TempSensorCSenable(void)
{
  nrf_drv_gpiote_out_clear(SPI_SS_PIN);   //SPI-CTRL: Selecting Temp Device
}
__STATIC_INLINE void spi_TempSensorCSdisable(void)
{
  nrf_drv_gpiote_out_set(SPI_SS_PIN);     //SPI-CTRL: Unselecting Temp Device
}
/* NVM DEVICE SECTION  */
__STATIC_INLINE void spi_NVMemoryCSenable(void)
{
  nrf_drv_gpiote_out_clear(SPI_NVM_CS_PIN);   //SPI-CTRL: Selecting NVM Device
}
__STATIC_INLINE void spi_NVMemoryCSdisable(void)
{
  nrf_drv_gpiote_out_set(SPI_NVM_CS_PIN);     //SPI-CTRL: Unselecting NVM Device
}

void spi_NVMemoryCMDreset(void)
{
  //Send RESET ENABLE COMMAND
  nvm_tx_buf[0] = NVM_CMD_RESET_EN;
  //memset(nvm_rx_buf, 0, 13);
  spi_xfer_done = false;
  nrf_drv_gpiote_out_clear(SPI_NVM_CS_PIN);   //SPI-CTRL: Selecting NVM Device
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, nvm_tx_buf, 1, nvm_rx_buf, 0));
  while (!spi_xfer_done)
  {   __WFE();    }
  nrf_drv_gpiote_out_set(SPI_NVM_CS_PIN);     //SPI-CTRL: Unselecting NVM Device
  nrf_delay_us(50);
  //Send RESET COMMAND
  nvm_tx_buf[0] = NVM_CMD_RESET;
  //memset(nvm_rx_buf, 0, 13);
  spi_xfer_done = false;
  nrf_drv_gpiote_out_clear(SPI_NVM_CS_PIN);   //SPI-CTRL: Selecting NVM Device
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, nvm_tx_buf, 1, nvm_rx_buf, 0));
  while (!spi_xfer_done)
  {   __WFE();    }
  nrf_drv_gpiote_out_set(SPI_NVM_CS_PIN);     //SPI-CTRL: Unselecting NVM Device
}

/*****************************************************************************
 * Function: 	spi_NVMemoryReadJEDECID
 * Description: Read JEDEC ID
 * return:
 *          - uint32_t value that contains MFR and MEM ID
 *****************************************************************************/
uint32_t spi_NVMemoryReadJEDECID(void)
{
  nvm_tx_buf[0] = NVM_CMD_JEDEC_ID;     //Read JEDEC ID COMMAND
  nvm_tx_buf[1] = 0x0A;     //3 Dummies
  nvm_tx_buf[2] = 0x0A; 
  nvm_tx_buf[3] = 0x0A; 

  // Reset rx buffer and transfer done flag
  memset(nvm_rx_buf, 0, 4);
  spi_xfer_done = false;
  nrf_drv_gpiote_out_clear(SPI_NVM_CS_PIN);   //SPI-CTRL: Selecting NVM Device
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, nvm_tx_buf, 4, nvm_rx_buf, 4));
  while (!spi_xfer_done)
  {   __WFE();    }
  nrf_drv_gpiote_out_set(SPI_NVM_CS_PIN);     //SPI-CTRL: Unselecting NVM Device

  return (uint32_t) ((nvm_rx_buf[1]<<16) & 0xFF0000) |
                   ((nvm_rx_buf[2]<<8) & 0x00FF00) |
                   ((nvm_rx_buf[1]) & 0x0000FF);
/*
  if( nvm_rx_buf[1] == NVM_MFR_FLASH_ID && 
      nvm_rx_buf[2] == (uint8_t)((NVM_W25Q64_SPI_ID>>8)&0xFF) &&
      nvm_rx_buf[3] == (uint8_t)(NVM_W25Q64_SPI_ID & 0xFF)  )
  { 
    spi_xfer_done = false;
    return true;
  }else{
    return false;
  }*/
}

/*****************************************************************************
 * Function: 	spi_NVMemoryWriteEnable
 * Description: The Write Enable instruction (Figure 5) sets the Write Enable Latch (WEL) bit 
 *              in the Status Register to a 1. 
 *              The WEL bit must be set prior to every Page Program
 *****************************************************************************/
void spi_NVMemoryWriteEnable(void)
{
  nvm_tx_buf[0] = NVM_CMD_WR_ENABLE;     //Write Enable (06h) COMMAND
  spi_xfer_done = false;
  nrf_drv_gpiote_out_clear(SPI_NVM_CS_PIN);   //SPI-CTRL: Selecting NVM Device
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, nvm_tx_buf, 1, nvm_rx_buf, 0));
  while (!spi_xfer_done)
  {   __WFE();    }
  nrf_drv_gpiote_out_set(SPI_NVM_CS_PIN);     //SPI-CTRL: Unselecting NVM Device
}

/*****************************************************************************
 * Function: 	spi_NVMemoryWriteDisable
 * Description: The Write Disable instruction (Figure 7) resets the Write Enable Latch (WEL) bit 
 *              in the Status Register to a 0.
 *****************************************************************************/
void spi_NVMemoryWriteDisable(void)
{
  nvm_tx_buf[0] = NVM_CMD_WR_DISABLE;     //Write Disable (04h) COMMAND
  spi_xfer_done = false;
  nrf_drv_gpiote_out_clear(SPI_NVM_CS_PIN);   //SPI-CTRL: Selecting NVM Device
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, nvm_tx_buf, 1, nvm_rx_buf, 0));
  while (!spi_xfer_done)
  {   __WFE();    }
  nrf_drv_gpiote_out_set(SPI_NVM_CS_PIN);     //SPI-CTRL: Unselecting NVM Device
}

/*****************************************************************************
 * Function: 	spi_NVMemoryRead
 * Description: CMD 03h: Read data from NVM
 * Parameter:
 *             startPage 
 *             addrOffset offset from the start page and has a range from: 0 - 255
 *             noByte
 *             *rData
 * Caveats: this function can only address up to 512 block or addr: 0xFF FFFF
 *****************************************************************************/
void spi_NVMemoryRead(uint32_t page, uint8_t offset, uint32_t noByte, uint8_t * rData)
{ 
  uint32_t memAddr24bit = (uint32_t)((page *256) + offset); //24bit / 3byte address
  nvm_tx_buf[0] = NVM_CMD_READ;     //Read data COMMAND
  nvm_tx_buf[1] = (uint8_t)((memAddr24bit>>16) & 0xFF);   //MSB of the 24bit address
  nvm_tx_buf[2] = (uint8_t)((memAddr24bit>>8) & 0xFF); 
  nvm_tx_buf[3] = (uint8_t)((memAddr24bit) & 0xFF);       //LSB of the address
  spi_xfer_done = false;

  // Reset rx buffer and transfer done flag
  memset(nvm_rx_buf, 0, 256+4);
  nrf_drv_gpiote_out_clear(SPI_NVM_CS_PIN);   //SPI-CTRL: Selecting NVM Device
  //APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, nvm_tx_buf, noByte+4, rData, noByte+5));
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, nvm_tx_buf, 4, nvm_rx_buf, noByte+4));
  while (!spi_xfer_done)
  {   __WFE();    }
  nrf_drv_gpiote_out_set(SPI_NVM_CS_PIN);     //SPI-CTRL: Unselecting NVM Device

   //Now lets copy data from nvm_rx_buf[4] to rData[0]
   for(uint8_t cnt=0; cnt<noByte; cnt++)
   {
      *rData = nvm_rx_buf[cnt+4];
      rData++;
   }
 }

/*****************************************************************************
 * Function: 	spi_NVMemoryWritePage
 * Description: CMD 02h: PAge Program from NVM
 *              allow to write data from 1 to 256 bytes (page)
 *              BUT it also erease the sector first
 * Parameter:
 *             page 
 *             offset offset from the start page and has a range from: 0 - 255
 *             noByte
 *             *wData
 * Caveats:   this function can only address up to 512 block or addr: 0xFF FFFF
 *****************************************************************************/
void spi_NVMemoryWritePage(uint32_t page, uint8_t offset, uint32_t noByte, uint8_t * wData)
{
  uint32_t startPage = page;
  uint32_t endPage = startPage + ((offset+noByte-1)/256);
  uint32_t noPages = ( startPage - endPage )+1;

  //Now, we need to erase the entire sector: 4KB
  uint16_t startSector = startPage/16;
  uint16_t endSector = endPage/16;
  uint16_t noSectors = endSector-startSector+1;
  for (uint16_t cnt=0; cnt<noSectors; cnt++)
  {
    spi_NVMemoryEraseSector(startSector++);
  }
  uint16_t dataPosition = 0;
  uint16_t indx;
  //write data to each page 256bytes
  for (uint32_t cnt=0; cnt<noPages; cnt++)
  {
    uint32_t memAddr24bit = (uint32_t)((page *256) + offset); //24bit / 3byte address
    uint32_t byteStillToW = NVMpageNoByteToW(noByte,offset);
    
    spi_NVMemoryWriteEnable();    //Write Enable instruction must be executed

    nvm_tx_buf[0] = 0x02;     //Page Program COMMAND
    nvm_tx_buf[1] = (uint8_t)((memAddr24bit>>16) & 0xFF);   //MSB of the 24bit address
    nvm_tx_buf[2] = (uint8_t)((memAddr24bit>>8) & 0xFF); 
    nvm_tx_buf[3] = (uint8_t)((memAddr24bit) & 0xFF);       //LSB of the address
    indx=4;
    uint32_t byteToSend = byteStillToW + indx;
    uint16_t nByte;
    for (nByte=0; nByte<byteStillToW; nByte++)
    {
      nvm_tx_buf[indx++] = (uint8_t)wData[nByte+dataPosition];
    }
    spi_NVMemoryCSenable();   //SPI-CTRL: Selecting NVM Device
    spi_xfer_done = false;
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, nvm_tx_buf, byteToSend, nvm_rx_buf,byteToSend));
    while (!spi_xfer_done)
    {   __WFE();    }
    spi_NVMemoryCSdisable();     //SPI-CTRL: Unselecting NVM Device
    
    startPage++;
    offset = 0;
    noByte -= byteStillToW;
    dataPosition+= byteStillToW;
    nrf_delay_ms(6);
    spi_NVMemoryWriteDisable();   //Write Enable Latch (WEL) bit in the Status Register is cleared to 0.
  }
}

/*****************************************************************************
 * Function: 	NVMpageNoByteToW
 * Description: caculate how many byte I can still write to a page of 256bytes
 * return:  # of bytes available in one page
 *****************************************************************************/
uint32_t NVMpageNoByteToW(uint32_t NoByte, uint16_t offset)
{
  if( ((NoByte + offset)<256) )
  {
    return NoByte;
  }else{
    return 256-offset;
  }
}

/*****************************************************************************
 * Function: 	spi_NVMemoryEraseSector
 * Description: CMD 20h: Sector erase
 * Parameter:
 *             noSector 
 *****************************************************************************/
void spi_NVMemoryEraseSector(uint16_t noSector)
{
  uint32_t memAddr24bit = (uint32_t)(noSector*16*256); //24bit / 3byte address
  spi_NVMemoryWriteEnable();    //Write Enable instruction must be executed
  nvm_tx_buf[0] = 0x20;           //Erease Sector COMMAND
  nvm_tx_buf[1] = (uint8_t)((memAddr24bit>>16) & 0xFF);   //MSB of the 24bit address
  nvm_tx_buf[2] = (uint8_t)((memAddr24bit>>8) & 0xFF); 
  nvm_tx_buf[3] = (uint8_t)((memAddr24bit) & 0xFF);       //LSB of the address
  spi_xfer_done = false;
  spi_NVMemoryCSenable();       //SPI-CTRL: Selecting NVM Device
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, nvm_tx_buf, 4, nvm_rx_buf,0));
  while (!spi_xfer_done)
  {   __WFE();    }
  spi_NVMemoryCSdisable();      //SPI-CTRL: Unselecting NVM Device
  nrf_delay_ms(450);
  spi_NVMemoryWriteDisable();   //Write Enable Latch (WEL) bit in the Status Register is cleared to 0.
}
/*
NVM W25Q64FV (64M-bit)
has 128 blocks (64KB)
each block has 16 sector (4KB)
each sector has 16 pages 
each paghe has 256 bytes
Therefore 2048 sectors and 32,768 pages

One Page contains: 256 bytes
one group/sector contains: 16 pages or 4KB
one 32KB block contains: 128 pages or 32KB
one 64KB block contains: 256 pages or 64KB

Page Program (02h)
Sector Erase (20h)
Chip Erase (C7h / 60h)

Page has 256byte
Sector has 4K-bytes

Chip Erase (C7h / 60h) - CMD + address (A23-A0) of 000000h.
Read Unique ID Number (4Bh) - CMD + 4 byte dummy + 64bit ID

Erase Security Registers (44h)
Program Security Registers (42h)
Read Security Registers (48h)
*/


/*  EXAMPLE TO READ AND WRITE TO EXTERNAL FLASH  */
/*

uint8_t TxData[20];
volatile uint32_t NVMwriteDataFlg = 0;
static uint8_t rxData[20];
if(NVMwriteDataFlg)
{
spi_NVMemoryRead(3, 0, 14, &rxData[0]);
//spi_NVMemoryEraseSector(0);
sprintf((char *)TxData, "%s", "hello buffer  ");
 TxData[12] = rxData[12] + 1;
spi_NVMemoryWritePage(3, 0, 14, &TxData[0]);
}else{}
spi_NVMemoryRead(3, 0, 14, &rxData[0]);
*/
