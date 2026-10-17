#include "ifaddrs.h"

#include <stdlib.h>
#include <string.h>
#include <switch.h>

int getifaddrs(struct ifaddrs** list) {
    *list = NULL;
    u32 ip = 0;
    if (R_FAILED(nifmInitialize(NifmServiceType_User))) return 0;  // no address: libjuice then has no local candidates
    const Result rc = nifmGetCurrentIpAddress(&ip);
    nifmExit();
    if (R_FAILED(rc) || ip == 0) return 0;
    struct ifaddrs* ifa = calloc(1, sizeof *ifa);
    struct sockaddr_in* sin = calloc(1, sizeof *sin);
    if (ifa == NULL || sin == NULL) {
        free(ifa);
        free(sin);
        return -1;
    }
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = ip;  // libnx gives it as the bytes of the address in order, which is network order
    ifa->ifa_name = strdup("wlan0");
    ifa->ifa_flags = IFF_UP;
    ifa->ifa_addr = (struct sockaddr*)sin;
    *list = ifa;
    return 0;
}

void freeifaddrs(struct ifaddrs* list) {
    while (list != NULL) {
        struct ifaddrs* next = list->ifa_next;
        free(list->ifa_name);
        free(list->ifa_addr);
        free(list);
        list = next;
    }
}

// libnx has no pipe(); libjuice uses one to wake up its polling thread. Two UDP sockets on the
// loopback, each connected to the other, do the same job: what is written to fds[1] is read at fds[0].
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

__attribute__((weak)) int pipe(int fds[2]) {
    int a = socket(AF_INET, SOCK_DGRAM, 0);
    int b = socket(AF_INET, SOCK_DGRAM, 0);
    if (a < 0 || b < 0) goto fail;
    {
        struct sockaddr_in sa, sb;
        socklen_t la = sizeof sa, lb = sizeof sb;
        memset(&sa, 0, sizeof sa);
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        sb = sa;
        if (bind(a, (struct sockaddr*)&sa, sizeof sa) != 0 || bind(b, (struct sockaddr*)&sb, sizeof sb) != 0) goto fail;
        if (getsockname(a, (struct sockaddr*)&sa, &la) != 0 || getsockname(b, (struct sockaddr*)&sb, &lb) != 0) goto fail;
        if (connect(a, (struct sockaddr*)&sb, sizeof sb) != 0 || connect(b, (struct sockaddr*)&sa, sizeof sa) != 0) goto fail;
    }
    fds[0] = a;
    fds[1] = b;
    return 0;
fail:
    if (a >= 0) close(a);
    if (b >= 0) close(b);
    return -1;
}
