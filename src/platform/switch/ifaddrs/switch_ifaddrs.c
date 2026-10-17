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
