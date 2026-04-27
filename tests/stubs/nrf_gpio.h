/* Host-test stub: nrf_gpio.h */
#ifndef NRF_GPIO_H_STUB__
#define NRF_GPIO_H_STUB__

#include <stdint.h>

static inline void nrf_gpio_cfg_output(uint32_t p) { (void)p; }
static inline void nrf_gpio_cfg_input(uint32_t p, int pull) { (void)p; (void)pull; }
static inline void nrf_gpio_pin_set(uint32_t p) { (void)p; }
static inline void nrf_gpio_pin_clear(uint32_t p) { (void)p; }
static inline void nrf_gpio_pin_toggle(uint32_t p) { (void)p; }
static inline uint32_t nrf_gpio_pin_read(uint32_t p) { (void)p; return 0; }

#define NRF_GPIO_PIN_PULLUP   1
#define NRF_GPIO_PIN_PULLDOWN 3
#define NRF_GPIO_PIN_NOPULL   0

#endif /* NRF_GPIO_H_STUB__ */
