/* Host-test stub: nrf_drv_timer.h */
#ifndef NRF_DRV_TIMER_H_STUB__
#define NRF_DRV_TIMER_H_STUB__

#include <stdint.h>
#include <stddef.h>

typedef enum {
    NRF_TIMER_BIT_WIDTH_8  = 0,
    NRF_TIMER_BIT_WIDTH_16 = 1,
    NRF_TIMER_BIT_WIDTH_24 = 2,
    NRF_TIMER_BIT_WIDTH_32 = 3
} nrf_timer_bit_width_t;

typedef enum {
    NRF_TIMER_FREQ_16MHz = 0,
    NRF_TIMER_FREQ_8MHz  = 1,
    NRF_TIMER_FREQ_4MHz  = 2,
    NRF_TIMER_FREQ_2MHz  = 3,
    NRF_TIMER_FREQ_1MHz  = 4
} nrf_timer_frequency_t;

typedef enum {
    NRF_TIMER_CC_CHANNEL0 = 0,
    NRF_TIMER_CC_CHANNEL1 = 1,
    NRF_TIMER_CC_CHANNEL2 = 2,
    NRF_TIMER_CC_CHANNEL3 = 3
} nrf_timer_cc_channel_t;

typedef enum {
    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK = 1,
    NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK = 2
} nrf_timer_short_mask_t;

typedef enum {
    NRF_TIMER_EVENT_COMPARE0 = 0,
    NRF_TIMER_EVENT_COMPARE1 = 1,
    NRF_TIMER_EVENT_COMPARE2 = 2,
    NRF_TIMER_EVENT_COMPARE3 = 3
} nrf_timer_event_t;

typedef struct {
    void    *p_reg;
    uint8_t  instance_id;
} nrfx_timer_t;

typedef nrfx_timer_t nrf_drv_timer_t;

typedef void (*nrfx_timer_event_handler_t)(nrf_timer_event_t event_type, void *p_context);

typedef struct {
    nrf_timer_frequency_t frequency;
    nrf_timer_bit_width_t bit_width;
    uint8_t               interrupt_priority;
    void                 *p_context;
} nrf_drv_timer_config_t;

/* Compound-literal macro matching the real SDK pattern */
#define NRF_DRV_TIMER_INSTANCE(id) \
    { .p_reg = (void *)(uintptr_t)((id) + 1), .instance_id = (uint8_t)(id) }

#define NRF_DRV_TIMER_DEFAULT_CONFIG \
    { NRF_TIMER_FREQ_1MHz, NRF_TIMER_BIT_WIDTH_32, 6, NULL }

static inline uint32_t nrf_drv_timer_init(
    const nrf_drv_timer_t         *t,
    const nrf_drv_timer_config_t  *cfg,
    nrfx_timer_event_handler_t     handler)
{ (void)t; (void)cfg; (void)handler; return 0; }

static inline void nrf_drv_timer_enable(const nrf_drv_timer_t *t)  { (void)t; }
static inline void nrf_drv_timer_disable(const nrf_drv_timer_t *t) { (void)t; }

static inline uint32_t nrf_drv_timer_us_to_ticks(
    const nrf_drv_timer_t *t, uint32_t us)
{ (void)t; return us; }

static inline void nrf_drv_timer_extended_compare(
    const nrf_drv_timer_t  *t,
    nrf_timer_cc_channel_t  channel,
    uint32_t                cc_value,
    nrf_timer_short_mask_t  short_mask,
    int                     enable_int)
{ (void)t; (void)channel; (void)cc_value; (void)short_mask; (void)enable_int; }

#endif /* NRF_DRV_TIMER_H_STUB__ */
