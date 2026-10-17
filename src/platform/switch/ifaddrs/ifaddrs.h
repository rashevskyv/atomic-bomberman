// <ifaddrs.h> for the Nintendo Switch, which has none: the one address that matters, the
// console's own IPv4 address on the network it is connected to (libjuice's local candidates).
#pragma once

#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#ifndef IFF_UP
#define IFF_UP 0x1
#endif
#ifndef IFF_LOOPBACK
#define IFF_LOOPBACK 0x8
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ifaddrs {
    struct ifaddrs* ifa_next;
    char* ifa_name;
    unsigned int ifa_flags;
    struct sockaddr* ifa_addr;
    struct sockaddr* ifa_netmask;
};

int getifaddrs(struct ifaddrs** list);
void freeifaddrs(struct ifaddrs* list);

#ifdef __cplusplus
}
#endif
