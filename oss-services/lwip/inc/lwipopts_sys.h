#ifndef _lwipopts_h
#define _lwipopts_h

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Thread defines */
#define TCPIP_THREAD_STACKSIZE          (configMINIMAL_STACK_SIZE + 1024)
#define TCPIP_THREAD_PRIO               (tskIDLE_PRIORITY + 1)
#define TCPIP_MBOX_SIZE                 10
#define DEFAULT_THREAD_STACKSIZE        (configMINIMAL_STACK_SIZE + 256)
#define DEFAULT_THREAD_PRIO             (tskIDLE_PRIORITY + 1)
#define DEFAULT_RAW_RECVMBOX_SIZE       10
#define DEFAULT_UDP_RECVMBOX_SIZE       10
#define DEFAULT_TCP_RECVMBOX_SIZE       10
#define DEFAULT_ACCEPTMBOX_SIZE         10

/* Enable APIs */
#define LWIP_NETIF_API                  1
#define LWIP_RAW                        0

/* Enable DHCP */
#define LWIP_DHCP                       1
#define LWIP_DHCP_CHECK_LINK_UP         1
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_NETIF_STATUS_CALLBACK      1

/* Enable link up/dn callbacks */
#define LWIP_NETIF_LINK_CALLBACK        1

/* API configs */
#define MEMP_NUM_NETCONN                10
#define LWIP_SO_RCVTIMEO                1

/* Socket options */
#define SO_REUSE                        1

/* Heap configs */
#define MEM_LIBC_MALLOC                 1
#define mem_clib_malloc                 pvPortMalloc
#define mem_clib_free                   vPortFree
#define MEMP_MEM_MALLOC                 1

// if this is left to default 60s, we will need many TCP PCBs
// this ensures they die quickly
// http://savannah.nongnu.org/bugs/?49372
#define TCP_MSL 100UL /* The maximum segment lifetime in milliseconds */

#endif
