// mbedTLS settings for the Nintendo Switch (devkitA64): there is no /dev/urandom, so the random
// numbers come from the console's own generator (switch_entropy.c).
#pragma once
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_PLATFORM_MS_TIME_ALT
// Modules that need a Unix or Windows system underneath; the game uses its own sockets and timers.
#undef MBEDTLS_TIMING_C
#undef MBEDTLS_NET_C
