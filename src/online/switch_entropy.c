// The entropy source mbedTLS uses on the Nintendo Switch: libnx's csrng service.
#include <stddef.h>
#include <switch.h>

int mbedtls_hardware_poll(void* data, unsigned char* output, size_t len, size_t* olen) {
    (void)data;
    static int ready = 0;
    if (!ready) {
        if (R_FAILED(csrngInitialize())) return -1;  // MBEDTLS_ERR_ENTROPY_SOURCE_FAILED is -0x003C; any failure will do
        ready = 1;
    }
    if (R_FAILED(csrngGetRandomBytes(output, len))) return -1;
    *olen = len;
    return 0;
}
