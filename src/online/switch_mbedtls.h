// mbedTLS settings for the Nintendo Switch (devkitA64): there is no /dev/urandom, so the random
// numbers come from the console's own generator (switch_entropy.c).
#pragma once
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
