# simple-drivers/lwip/lwip-adi-ether-netif

## Overview

The `lwip-adi-ether-netif` driver provides a bridge between the Analog Devices Device EMAC driver and lwIP.

## Required components

- lwip
- FreeRTOS
- lwip-arch-FreeRTOS
- adi-ethernet

## Recommended components

- None

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project

## Configure

- See the top of `lwip_adi_ether_netif.h` for configurable options.

## Run

```C
/* LwIP includes */
#include "lwip/inet.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "netif/etharp.h"

/* ADI Ethernet lwIP netif includes */
#include "lwip_adi_ether_netif.h"

static const char *IP_ADDR_STR = "192.168.20.2";
static const char *NETMASK_STR = "255.255.255.0";
static const char *GATEWAY_STR = "192.168.20.1";
static const uint8_t macaddr[6] = {0x00, 0x12, 0x34, 0x56, 0x78, 0x9A };

struct netif eth0_netif;
adi_ether_netif *adi_ether;

/* This function converts a string to a network address struct */
static int net_aton(char *str_addr, ip_addr_t *net_addr)
{
    int i = inet_aton(str_addr, &net_addr->addr);
    if (!i)
        return -1;
    return 0;
}

/* This function prints a network address */
static void log_addr(char *name, ip_addr_t *net_addr)
{
    syslog_printf("%s: %d.%d.%d.%d\n", name,
        ip4_addr1(net_addr), ip4_addr2(net_addr),
        ip4_addr3(net_addr), ip4_addr4(net_addr));
}

#if LWIP_NETIF_STATUS_CALLBACK
static void netif_status_callback(struct netif *netif)
{
    /* Make sure there's a sane address */
    if (ip_addr_get_ip4_u32(&netif->ip_addr) == 0x00000000) {
        return;
    }
    log_addr("IP Address", &netif->ip_addr);
    log_addr("Gateway", &netif->gw);
    log_addr("Netmask", &netif->netmask);
}
#endif

#if LWIP_NETIF_LINK_CALLBACK
static void netif_link_status_callback(struct netif *netif)
{
    if (netif->flags & NETIF_FLAG_LINK_UP) {
        syslog_print("Ethernet link up\n");
    } else {
        syslog_print("Ethernet link down\n");
    }
}
#endif

void ethernet_init(void)
{
    ip_addr_t sam_ip_addr, gw_addr, netmask;
    err_t err;

    /* Set IP addresses */
    net_aton((char *)IP_ADDR_STR, &sam_ip_addr);
    net_aton((char *)GATEWAY_STR, &gw_addr);
    net_aton((char *)NETMASK_STR, &netmask);

    /* Initialize lwIP and the TCP/IP subsystem */
    tcpip_init(NULL, NULL);

    /* Allocate a new ADI Ethernet network interface */
    adi_ether = adi_ether_netif_new();

    /* Configure the user parameters (hostname, EMAC port, MAC address) */
    adi_ether->hostName = "ez-kit";
    adi_ether->port = EMAC0;
    memcpy(adi_ether->ethAddr.addr, macaddr, sizeof(adi_ether->ethAddr.addr));

    /* Add the network interface */
    netif_add(&eth0_netif,
        &sam_ip_addr, &netmask, &gw_addr,
        adi_ether, adi_ether_netif_init, tcpip_input);

    /* Set this interface as the default interface */
    netif_set_default(&eth0_netif);

#if LWIP_NETIF_STATUS_CALLBACK
    /* Register a callback for DHCP address complete */
    netif_set_status_callback(&eth0_netif, netif_status_callback);
#endif

#if LWIP_NETIF_LINK_CALLBACK
    /* Register a callback for link status */
    netif_set_link_callback(&eth0_netif, netif_link_status_callback);
#endif

    /* Set interface to up */
    netif_set_up(&eth0_netif);

#ifdef ENABLE_DHCP
    /* Enable DHCP */
    dhcp_start(&eth0_netif);
#endif

}

```

## Info

- None
