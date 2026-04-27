/* Host-test stub: app_config.h
   Provides pin/timing defines without overriding NRF_LOG_ENABLED.
   app_config.h from ble_espresso_app/pca10040/s132/config/ would redefine
   NRF_LOG_ENABLED=1, conflicting with nrf_log.h stub (NRF_LOG_ENABLED=0).
   This stub is found first via -I tests/stubs and shadows the real file. */
#ifndef APP_CONFIG_H_STUB__
#define APP_CONFIG_H_STUB__

/* GPIO pin assignments (same values as the real app_config.h) */
#define inBREW_PIN          27
#define inSTEAM_PIN         28
#define inZEROCROSS_PIN     26
#define outSSRboiler_PIN    19
#define outSSRpump_PIN      20
#define enSolenoidRelay_PIN 25
#define enDC12Voutput_PIN    3

/* SPI pins */
#define SPI_SCK_PIN   13
#define SPI_MISO_PIN  11
#define SPI_MOSI_PIN  14
#define SPI_SS_PIN    12

/* Keep NRF_LOG_ENABLED at 0 — purposely NOT defined here so the value from
   the nrf_log.h stub (0) wins. */

#endif /* APP_CONFIG_H_STUB__ */
