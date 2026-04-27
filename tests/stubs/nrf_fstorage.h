/* Host-test stub: nrf_fstorage.h
   StorageController.c includes this but accesses flash entirely through
   spi_Devices API — no fstorage API is actually called. */
#ifndef NRF_FSTORAGE_H_STUB__
#define NRF_FSTORAGE_H_STUB__

#include <stdint.h>

/* Provide the NRF_FSTORAGE_DEF macro stub so any module-level macro usage
   compiles without error. */
#define NRF_FSTORAGE_DEF(x)

#endif /* NRF_FSTORAGE_H_STUB__ */
