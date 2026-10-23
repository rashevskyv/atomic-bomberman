// mbedTLS settings for the Nintendo Switch (devkitA64): there is no /dev/urandom, so the random
// numbers come from the console's own generator (switch_entropy.c).
#pragma once
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_PLATFORM_MS_TIME_ALT
// libnx has the BSD sockets mbedTLS's net module wants (getaddrinfo, select), but the module
// only accepts a Unix-like system by name. Its timing module (not used here) has no clock to use.
#define MBEDTLS_PLATFORM_IS_UNIXLIKE
#undef MBEDTLS_TIMING_C
