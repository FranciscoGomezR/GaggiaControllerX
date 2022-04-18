//*****************************************************************************
//
//			INCLUDE FILE SECTION FOR THIS MODULE
//
//*****************************************************************************
#include "i2c_sensors.h"

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
float eTMP006_Temp;

//*****************************************************************************
//
//			PRIVATE VARIABLES
//
//*****************************************************************************
/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false;

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

/* Buffer for raw samples read from temperature sensor. */
static uint8_t m_sample[2];

static float pTemp;

//*****************************************************************************
//
//			PRIVATE FUNCTIONS PROTOYPES
//
//*****************************************************************************
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context);

//*****************************************************************************
//
//			PUBLIC FUNCTIONS SECTION
//
//*****************************************************************************
/*****************************************************************************
 * Function: 	LM75B_set_mode
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void LM75B_set_mode(void)
{
    ret_code_t err_code;

    /* Writing to LM75B_REG_CONF "0" set temperature sensor in NORMAL mode. */
    uint8_t reg[2] = {TMP006_REG_CONF, NORMAL_MODE};
    err_code = nrf_drv_twi_tx(&m_twi, TMP006_ADDR, reg, sizeof(reg), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);

    /* Writing to pointer byte. */
    reg[0] = TMP006_REG_TEMP;
    m_xfer_done = false;
    err_code = nrf_drv_twi_tx(&m_twi, TMP006_ADDR, reg, 1, false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done == false);
}

/*****************************************************************************
 * Function: 	twi_init
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void twi_init (void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_lm75b_config = {
       .scl                = ARDUINO_SCL_PIN,
       .sda                = ARDUINO_SDA_PIN,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_lm75b_config, twi_handler, NULL);
    APP_ERROR_CHECK(err_code);
    nrf_drv_twi_enable(&m_twi);
}

/*****************************************************************************
 * Function: 	read_sensor_data
 * Description: 
 * Caveats:
 * Parameters:	
 * Return:
 *****************************************************************************/
void read_sensor_data(void)
{
    uint16_t vTemp;
    if(twi_operation_done() == true)
    {
        m_xfer_done = false;
        /* Read 2 byte from the specified address - skip 3 bits dedicated for fractional part of temperature. */
        ret_code_t err_code = nrf_drv_twi_rx(&m_twi, TMP006_ADDR, &m_sample[0], 2);
        APP_ERROR_CHECK(err_code);
        vTemp = ((((uint16_t)m_sample[0])<<8)|((uint16_t)m_sample[1]));
        vTemp = vTemp>>2;
        pTemp = ((float)vTemp/(float)32);
    }else{}
}


bool twi_operation_done(void)
{
  return m_xfer_done;
}
//*****************************************************************************
//
//			PRIVATE FUNCTIONS SECTION
//
//*****************************************************************************

/*****************************************************************************
 * Function: 	twi_handler
 * Description: Function for handling data from temperature sensor
 * Caveats:
 * Parameters:	temp - Temperature in Celsius degrees read from sensor.
 * Return:
 *****************************************************************************/
void twi_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX)
            {
                eTMP006_Temp = pTemp;
            }
            m_xfer_done = true;
            break;
        default:
            break;
    }
}