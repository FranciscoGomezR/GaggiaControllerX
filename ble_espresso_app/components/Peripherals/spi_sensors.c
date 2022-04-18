
//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
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
float eThermocoupleTemp;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */

static uint8_t       m_tx_buf[2] = {0xAA,0xAA};     /**< TX buffer. */
static uint8_t       m_rx_buf[2+1];                   /**< RX buffer. */
static const uint8_t m_length = 2;                  /**< Transfer length. */

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
    
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
    smRead_SPI_TempSen.sRunning = sm_state0;
 }

 void spim_Read_TempSensor(void)
 {
      volatile uint16_t CoupleVolt;
      switch(smRead_SPI_TempSen.sRunning)
      {
          case sm_state0:
            // Reset rx buffer and transfer done flag
            memset(m_rx_buf, 0, m_length);
            spi_xfer_done = false;
            APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf, m_length, m_rx_buf, m_length));
            smRead_SPI_TempSen.sRunning = sm_state1;
          break;

          case sm_state1:
            if(spi_xfer_done == true)
            {
              CoupleVolt = (((uint16_t)m_rx_buf[0])<<8 | (uint16_t)m_rx_buf[1]);
              CoupleVolt = (uint16_t)(CoupleVolt>>3);
              eThermocoupleTemp = (float)CoupleVolt * (float)0.25;
              //NRF_LOG_INFO("Tsen:   " NRF_LOG_FLOAT_MARKER "\r\n", NRF_LOG_FLOAT(Temp));
              //NRF_LOG_FLUSH();
              smRead_SPI_TempSen.sRunning = sm_state0;
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