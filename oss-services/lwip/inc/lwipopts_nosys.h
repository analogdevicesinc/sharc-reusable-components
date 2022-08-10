#ifndef _lwipopts_h
#define _lwipopts_h

/* No RTOS support */
#define NO_SYS 1

/* No thread protection needed */
#define SYS_LIGHTWEIGHT_PROT 0

/* No netconn interface */
#define LWIP_NETCONN  0

/* No socket interface */
#define LWIP_SOCKET  0

/* Enable DHCP */
#define LWIP_DHCP 1
#define LWIP_DHCP_CHECK_LINK_UP 1
#define LWIP_NETIF_HOSTNAME 1

#endif
