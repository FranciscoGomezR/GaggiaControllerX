#ifndef SPI_DEVICES_H__
#define SPI_DEVICES_H__

#ifdef __cplusplus
extern  "C" {
#endif

//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "nrf_drv_spi.h"
#include "x01_StateMachineControls.h"
//#include "nrf_log.h"
//#include "nrf_log_ctrl.h"
//#include "nrf_log_default_backends.h"

//*****************************************************************************
//
//			PUBLIC DEFINES SECTION
//
//*****************************************************************************
typedef enum {
    NVM_INIT_OK = 0,
    NVM_INIT_ERROR,
    NVM_INIT_NVM_ID_ERROR,
    NVM_INIT_NVM_TYPE_ERROR
} spi_nvm_status_t;

typedef enum {
    TMP_INIT_OK = 0,
    TMP_INIT_ERROR,
} spi_Tmp_status_t;

//*****************************************************************************
//
//			PUBLIC STRUCTs, UNIONs ADN ENUMs SECTION
//
//*****************************************************************************

//*****************************************************************************
//
//			PUBLIC VARIABLES PROTOTYPE
//
//*****************************************************************************
//extern volatile float rtdTemperature; -> encapsulated

//*****************************************************************************
//
//			PUBLIC FUNCTIONS PROTOYPES
//
//*****************************************************************************
void spim_init (void);

/* NVM DEVICE SECTION  */
spi_nvm_status_t spim_initNVmemory(void);

/* TEMP DEVICE SECTION  */
spi_Tmp_status_t spim_initRTDconverter(void);
void spim_ReadRTDconverter(void);
bool spim_operation_done(void);
float f_getBoilerTemperature(void);

void spim_DevCommMng(void);

/* NVM SECTION  */
void spi_NVMemoryRead(uint32_t page, uint8_t offset, uint32_t noByte, uint8_t * rData);
void spi_NVMemoryWritePage(uint32_t page, uint8_t offset, uint32_t noByte, uint8_t * wData);

#ifdef __cplusplus
}
#endif
#endif // SPI_SENSORS_H__