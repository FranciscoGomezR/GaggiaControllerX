/* Host-test stub: nrf_drv_gpiote.h */
#ifndef NRF_DRV_GPIOTE_H_STUB__
#define NRF_DRV_GPIOTE_H_STUB__

#include <stdint.h>

typedef uint32_t nrf_drv_gpiote_pin_t;

typedef enum {
    NRF_GPIOTE_POLARITY_LOTOHI = 1,
    NRF_GPIOTE_POLARITY_HITOLO = 2,
    NRF_GPIOTE_POLARITY_TOGGLE = 3
} nrf_gpiote_polarity_t;

typedef void (*nrf_drv_gpiote_evt_handler_t)(nrf_drv_gpiote_pin_t, nrf_gpiote_polarity_t);

typedef struct { int sense; int pull; int is_watcher; int hi_accuracy; } nrf_drv_gpiote_in_config_t;

#define NRF_DRV_GPIOTE_CONFIG_IN_SENSE_TOGGLE(hi_acc) { .sense = 3, .pull = 0, .is_watcher = 0, .hi_accuracy = (hi_acc) }

static inline uint32_t nrf_drv_gpiote_init(void) { return 0; }
static inline uint32_t nrf_drv_gpiote_in_init(nrf_drv_gpiote_pin_t p,
    const nrf_drv_gpiote_in_config_t *cfg,
    nrf_drv_gpiote_evt_handler_t h) { (void)p; (void)cfg; (void)h; return 0; }
static inline void nrf_drv_gpiote_in_event_enable(nrf_drv_gpiote_pin_t p, int e) { (void)p; (void)e; }
static inline uint32_t nrf_drv_gpiote_in_is_set(nrf_drv_gpiote_pin_t p) { (void)p; return 0; }

#endif /* NRF_DRV_GPIOTE_H_STUB__ */
