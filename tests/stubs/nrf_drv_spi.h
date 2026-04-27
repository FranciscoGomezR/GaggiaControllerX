/* Host-test stub: nrf_drv_spi.h
   Provides minimal SPI driver types required by spi_Devices.h */
#ifndef NRF_DRV_SPI_H_STUB__
#define NRF_DRV_SPI_H_STUB__

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t instance_id;
} nrf_drv_spi_t;

typedef struct {
    uint8_t sck_pin;
    uint8_t miso_pin;
    uint8_t mosi_pin;
    uint8_t ss_pin;
    uint8_t irq_priority;
    uint8_t orc;
    uint8_t frequency;
    uint8_t mode;
    uint8_t bit_order;
} nrf_drv_spi_config_t;

typedef struct {
    const uint8_t *p_tx_buffer;
    uint8_t        tx_length;
    uint8_t       *p_rx_buffer;
    uint8_t        rx_length;
} nrf_drv_spi_xfer_desc_t;

#define NRF_DRV_SPI_INSTANCE(id) { .instance_id = (uint8_t)(id) }
#define NRF_DRV_SPI_DEFAULT_CONFIG { 0xFF, 0xFF, 0xFF, 0xFF, 7, 0xFF, 2, 0, 0 }

static inline uint32_t nrf_drv_spi_init(const nrf_drv_spi_t *p, const nrf_drv_spi_config_t *c, void *h, void *ctx)
    { (void)p; (void)c; (void)h; (void)ctx; return 0; }
static inline uint32_t nrf_drv_spi_transfer(const nrf_drv_spi_t *p, const uint8_t *tx, uint8_t txl, uint8_t *rx, uint8_t rxl)
    { (void)p; (void)tx; (void)txl; (void)rx; (void)rxl; return 0; }
static inline void nrf_drv_spi_uninit(const nrf_drv_spi_t *p) { (void)p; }

#endif /* NRF_DRV_SPI_H_STUB__ */
