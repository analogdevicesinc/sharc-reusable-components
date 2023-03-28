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

#define ADI_ETHER_DMA_DESCRIPTOR_SIZE   (32)
#define ADI_ETHER_NUM_RECV_DESC         (128)
#define ADI_ETHER_NUM_XMIT_DESC         (64)
#define ADI_ETHER_NUM_TX_BUFFS          (64)
#define ADI_ETHER_NUM_RX_BUFFS          (128)

#define ADI_ETHER_NUM_MBOX_EVENTS       (20)

typedef enum ADI_ETHER_EMAC_PORT {
    EMAC_UNKNOWN = 0,
    EMAC0,
    EMAC1
} ADI_ETHER_EMAC_PORT;

/***********************************************************************
 * Typedefs
 **********************************************************************/
/*
 * Must add 22 plus a sizeof(uint16_t) to the Ethernet MTU for additional
 * room to carry the Ethernet header and length.  Add an additional 12 bytes
 * to keep the buffers cache aligned for better performance.
 */
#define ETHERNET_MTU        (1500)
#define ETHERNET_MAX_SIZE   (ETHERNET_MTU + 22 + sizeof(uint16_t) + 12)

typedef struct ADI_ETHER_PACKET_DATA {
    uint8_t data[ETHERNET_MAX_SIZE];
} ADI_ETHER_PACKET_DATA;

typedef struct adi_ether_pbuf {
    struct pbuf_custom p;
    ADI_ETHER_BUFFER *pktBuffer;
    struct adi_ether_netif *adi_ether;
    bool busy;
} adi_ether_pbuf_t;


enum PKT_TYPES {
    ADI_ETHER_PKT_TYPE_UNKNOWN = 0,
    ADI_ETHER_PKT_TYPE_TX,
    ADI_ETHER_PKT_TYPE_RX
};

typedef struct adi_ether_netif adi_ether_netif;

typedef void (*ADI_ETHER_PTP_CB)(adi_ether_netif *netif,
    uint8_t *pktData, uint16_t pktSize, uint8_t pktType,
    uint32_t second, uint32_t nanoSecond);

typedef void (*ADI_ETHER_P1722_CB)(adi_ether_netif *netif,
    uint8_t *pktData, uint16_t pktSize, void *pkt);

void adi_ether_netif_p1722_free(adi_ether_netif *netif, 
    void *pkt);

/*
 * The rxPktData and txPktData buffers are at the top to facilitate
 * cache alignment when instantiated.  Same for the DMA descriptors.
 */
typedef struct adi_ether_netif {

    /* Ethernet data buffers */
    ADI_ETHER_PACKET_DATA rxPktData[ADI_ETHER_NUM_RX_BUFFS];
    ADI_ETHER_PACKET_DATA txPktData[ADI_ETHER_NUM_TX_BUFFS];

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
    ADI_ETHER_BUFFER rxBuff[ADI_ETHER_NUM_RX_BUFFS];
    ADI_ETHER_BUFFER txBuff[ADI_ETHER_NUM_TX_BUFFS];

    /*
     * ADI Ethernet DMA descriptor memory
     *
     * NOTE: Allocating this memory from the heap appears to trigger an
     *       issue with the TX descriptors in the EMAC driver.  This
     *       issue is still under investigation.
     */
    ADI_ETHER_MEM etherMemTable;
    uint8_t RecvMem[ADI_ETHER_NUM_RECV_DESC * ADI_ETHER_DMA_DESCRIPTOR_SIZE];
    uint8_t TransmitMem[ADI_ETHER_NUM_XMIT_DESC * ADI_ETHER_DMA_DESCRIPTOR_SIZE];

    /* Driver receive buffer state tracking */
    ADI_ETHER_BUFFER *pktReceivedBuffer;
    ADI_ETHER_BUFFER *pktTransmittedBuffer;
    adi_ether_pbuf_t aePbuf[ADI_ETHER_NUM_RX_BUFFS];
    uint16_t aePbufHead;

    /* Driver transmit buffer state tracking */
    uint16_t txPktHead;
    uint16_t txPktTail;

    /* Worker thread */
    sys_thread_t worker;
    sys_mbox_t workerToDo;

    /* PTP/lwIP Read/Write locks */
    sys_mutex_t readLock;
    sys_mutex_t writeLock;

    /* Netif reference */
    struct netif *netif;

    /* Allocated indication */
    bool allocated;
    int idx;

    /* PTP packet time callback */
    ADI_ETHER_PTP_CB ptpPktCb;

    /* 1722 packet callback */
    ADI_ETHER_P1722_CB p1722PktCb;

    /* User pointer */
    void *usrPtr;

} adi_ether_netif;

/***********************************************************************
 * API
 **********************************************************************/
adi_ether_netif *adi_ether_netif_new(void *usrPtr);
void adi_ether_netif_delete(adi_ether_netif *adi_ether);
err_t adi_ether_netif_init(struct netif *netif);

err_t adi_ether_netif_set_src_addr_filt(struct netif *netif,
    const uint32_t ipAddr, const uint8_t ipAMaskBits, const bool invert);
err_t adi_ether_netif_set_src_addr_filt_enable(struct netif *netif,
    const bool enable);
err_t adi_ether_netif_set_dst_port_filt(struct netif *netif,
    const uint16_t port, const bool udp, const bool invert);
err_t adi_ether_netif_set_dst_port_filt_enable(struct netif *netif,
    const bool enable);

err_t
adi_ether_netif_ptp_tx_frame(struct netif *netif, uint8_t *srcMacAddr,
    uint8_t *dstMacAddr, uint16_t etherType, uint8_t *data, uint16_t len,
    bool txCallback);

err_t
adi_ether_netif_1722_tx_frame(struct netif *netif, uint8_t *srcMacAddr,
    uint8_t *dstMacAddr, uint16_t etherType, uint8_t *p1722, uint16_t p1722Len,
    uint8_t *audio, uint16_t audioLen);

#endif
