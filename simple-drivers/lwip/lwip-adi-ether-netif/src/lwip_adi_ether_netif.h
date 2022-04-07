/**
 * Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

#ifndef _lwip_adi_ether_netif_h
#define _lwip_adi_ether_netif_h

#include <stdint.h>
#include <stdbool.h>

#include "adi_ether.h"
#include "adi_ether_gemac.h"

#include "lwip/etharp.h"
#include "lwip/sys.h"

#define ADI_ETHER_CALLOC                calloc
#define ADI_ETHER_MALLOC                malloc
#define ADI_ETHER_FREE                  free
#define ADI_ETHER_MEMSET                memset

#define ADI_ETHER_BASE_MEM_SIZE         (32)
#define ADI_ETHER_DMA_DESCRIPTOR_SIZE   (32)
#define ADI_ETHER_NUM_RECV_DESC         (32)
#define ADI_ETHER_NUM_XMIT_DESC         (64)
#define ADI_ETHER_NUM_TX_BUFFS          (64)
#define ADI_ETHER_NUM_RX_BUFFS          (32)

#define ADI_ETHER_NUM_MBOX_EVENTS       (20)

typedef enum ADI_ETHER_EMAC_PORT {
    EMAC0 = 0,
    EMAC1
} ADI_ETHER_EMAC_PORT;

/***********************************************************************
 * Typedefs
 **********************************************************************/
/*
 * Must add 22 plus a sizeof(uint16_t) to the Ethernet MTU for additional
 * room to carry the Ethernet header and length.
 */
#define ETHERNET_MTU        (1500)
#define ETHERNET_MAX_SIZE   (ETHERNET_MTU + 22 + sizeof(uint16_t))

typedef struct ADI_ETHER_PACKET_DATA {
    uint8_t data[ETHERNET_MAX_SIZE];
} ADI_ETHER_PACKET_DATA;

typedef struct adi_ether_netif {

    /* Application parameters */
    struct eth_addr ethAddr;
    ADI_ETHER_EMAC_PORT port;
    const char *hostName;

    /* Application status */
    bool linkUp;
    bool initOk;

    /**************************************************************
     * Members below are private to the driver
     *************************************************************/

    /* ADI Ethernet handle */
    ADI_ETHER_HANDLE  hEthernet;

    /* Ethernet packet buffers */
    ADI_ETHER_BUFFER *rxBuff;
    ADI_ETHER_BUFFER *txBuff;

    /* Ethernet data buffers */
    ADI_ETHER_PACKET_DATA *rxPktData;
    ADI_ETHER_PACKET_DATA *txPktData;

    /* ADI Ethernet driver memory */
    ADI_ETHER_MEM etherMemTable;

    /* Driver receive buffer state tracking */
    ADI_ETHER_BUFFER *pktReceivedBuffer;

    /* Driver transmit buffer state tracking */
    uint16_t txPktHead;
    uint16_t txPktTail;

    /* Worker thread */
    sys_thread_t worker;
    sys_mbox_t workerToDo;

    /* Netif reference */
    struct netif *netif;

} adi_ether_netif;

/***********************************************************************
 * API
 **********************************************************************/
adi_ether_netif *adi_ether_netif_new(void);
void adi_ether_netif_delete(adi_ether_netif *adi_ether);
err_t adi_ether_netif_init(struct netif *netif);

#endif
