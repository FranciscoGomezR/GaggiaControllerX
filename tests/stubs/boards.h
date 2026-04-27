/* Host-test stub: boards.h
   Provides the pin-ID defines that ac_inputs_drv.h and other peripherals need.
   Values match ble_espresso_app/pca10040/s132/config/app_config.h */
#ifndef BOARDS_H_STUB__
#define BOARDS_H_STUB__

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

#endif /* BOARDS_H_STUB__ */
