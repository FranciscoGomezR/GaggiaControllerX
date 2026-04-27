/* Host-test stub: nordic_common.h */
#ifndef NORDIC_COMMON_H_STUB__
#define NORDIC_COMMON_H_STUB__

#include <stdint.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

/* Compile-time assertion stub — silently ignored on host */
#define STATIC_ASSERT(expr) typedef char static_assert_check_##__LINE__[(expr) ? 1 : -1]

#endif /* NORDIC_COMMON_H_STUB__ */
