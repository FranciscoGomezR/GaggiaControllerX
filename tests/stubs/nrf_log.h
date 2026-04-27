/* Host-test stub: nrf_log.h */
#ifndef NRF_LOG_H_STUB__
#define NRF_LOG_H_STUB__

/* Disable all logging for host tests */
#define NRF_LOG_ENABLED 0

#define NRF_LOG_INFO(...)
#define NRF_LOG_DEBUG(...)
#define NRF_LOG_WARNING(...)
#define NRF_LOG_ERROR(...)
#define NRF_LOG_RAW_INFO(...)
#define NRF_LOG_PROCESS()   (false)

#define NRF_LOG_FLOAT_MARKER    "%f"
#define NRF_LOG_FLOAT(x)        (x)

#endif /* NRF_LOG_H_STUB__ */
