
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "spi_sensors.h"
#include "boards.h"
#include "x01_StateMachineControls.h"

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
float rtdTemperature;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */

static uint8_t       m_tx_buf[3];    /**< TX buffer. */
static uint8_t       m_rx_buf[3];                   /**< RX buffer. */
static const uint8_t m_length = 2;                  /**< Transfer length. */
static const uint8_t r_length = 3;
volatile uint8_t hwreg_SensorConfig;
volatile float resistanceRTD;

enum{
      sm_state0=0,
      sm_state1,
      sm_state2
      };

 StateMachineCtrl_Struct smRead_SPI_TempSen;

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************
void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context);


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
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = SPI_SS_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;
    spi_config.sck_pin  = SPI_SCK_PIN;
    spi_config.frequency = NRF_DRV_SPI_FREQ_1M;
    spi_config.mode     = NRF_DRV_SPI_MODE_3;
    spi_config.bit_order= NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
    
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
    smRead_SPI_TempSen.sRunning = sm_state0;
 }

 void spim_initRTDconverter (void)
 {
    m_tx_buf[0] = 0x80;     //Configuration Register
    m_tx_buf[1] = 0xD1; 
    // Reset rx buffer and transfer done flag
    memset(m_rx_buf, 0, m_length);
    spi_xfer_done = false;
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf, m_length, m_rx_buf, m_length));
    while (!spi_xfer_done)
    {   __WFE();    }

    m_tx_buf[0] = 0x00;     //Configuration Register
    m_tx_buf[1] = 0xA0; 
    // Reset rx buffer and transfer done flag
    memset(m_rx_buf, 0, m_length);
    spi_xfer_done = false;
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf, m_length, m_rx_buf, m_length));
    while (!spi_xfer_done)
    {   __WFE();    }
    
    hwreg_SensorConfig = (volatile uint8_t)m_rx_buf[1];
 }

 void spim_ReadRTDconverter(void)
 {
      uint16_t AdcConv;
      static float  sqrtTerm, Numerator, rtdTemp;
      switch(smRead_SPI_TempSen.sRunning)
      {
          case sm_state0:
            m_tx_buf[0] = 0x01;     //Configuration Register
            m_tx_buf[1] = 0xAA; 
            m_tx_buf[2] = 0xAA;
            // Reset rx buffer and transfer done flag
            memset(m_rx_buf, 0, m_length);
            spi_xfer_done = false;
            APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf, r_length, m_rx_buf, r_length));
            smRead_SPI_TempSen.sRunning = sm_state1;
          break;

          case sm_state1:
            if(spi_xfer_done == true)
            {
              AdcConv = (((uint16_t)m_rx_buf[1])<<8 | (uint16_t)m_rx_buf[2]);
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
              smRead_SPI_TempSen.sRunning = sm_state0;
             // smRead_SPI_TempSen.sRunning = sm_state2;
            }else{}
          break;
      }      
 }


 bool spim_operation_done(void)
 {
    return spi_xfer_done;
 }

//***********************************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//***********************************************************************************************

/*****************************************************************************
 * Function: 	spi_event_handler
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    spi_xfer_done = true;
}