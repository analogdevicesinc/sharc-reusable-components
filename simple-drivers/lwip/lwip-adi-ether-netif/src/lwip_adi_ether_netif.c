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

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <machine/endian.h>

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"

#include "lwip_adi_ether_netif.h"

/* Make sure ETH_PAD_SIZE is set to 2 */
#if !defined(ETH_PAD_SIZE) || (ETH_PAD_SIZE != 2)
#error ETH_PAD_SIZE must be 2
#endif

#ifndef ADI_ETHER_MAX_NETIFS
#define ADI_ETHER_MAX_NETIFS 2
#endif

/* Network interface name */
#define IFNAME0 'a'
#define IFNAME1 'd'

/* This is a continuation of the hack in lwIP sys_arch.c FreeRTOS
 * porting layer.
 */
extern long xInsideISR;

enum WORKER_TODO {
    WORKER_UNKNOWN = 0,
    WORKER_LINK_UP,
    WORKER_LINK_DOWN,
    WORKER_PKT_RX,
    WORKER_PKT_TX
};

/*************************************************************************
 * Private APIs
 ************************************************************************/
/* This function readies an ADI_ETHER_BUFFER for a new reception */
static void
adi_ether_netif_reset_rx_buff(ADI_ETHER_BUFFER *rxBuff)
{
    rxBuff->ElementCount = ETHERNET_MAX_SIZE;
    rxBuff->ElementWidth = 1;
    rxBuff->ProcessedFlag = 0;
    rxBuff->Status = 0;
#ifdef ADI_ETHER_SUPPORT_AV
    rxBuff->nChannel = 0;
#endif
}

/* This function initializes and links the Ethernet driver Rx buffers */
static void
adi_ether_netif_init_rx(adi_ether_netif *adi_ether)
{
    ADI_ETHER_BUFFER *rxBuff;
    ADI_ETHER_PACKET_DATA *rxPktData;
    int i;

    rxBuff = adi_ether->rxBuff;
    rxPktData = adi_ether->rxPktData;

    /* Initialize and link the Rx buffers */
    for (i = 0; i < ADI_ETHER_NUM_RX_BUFFS; i++) {
        adi_ether_netif_reset_rx_buff(&rxBuff[i]);
        rxBuff[i].Data = &rxPktData[i].data;
        rxBuff[i].pNext = NULL;
        if (i > 0) {
            rxBuff[i-1].pNext = &rxBuff[i];
        }
    }
}

/* This function returns a zero copy Rx buffer to the driver */
static void
adi_ether_netif_pbuf_free(struct pbuf *p)
{
    adi_ether_pbuf_t *aep = (adi_ether_pbuf_t *)p;
    adi_ether_netif *adi_ether = aep->adi_ether;
    ADI_ETHER_BUFFER *pktBuffer = aep->pktBuffer;

    aep->busy = false;
    adi_ether_netif_reset_rx_buff(pktBuffer);

    sys_mutex_lock(&adi_ether->readLock);
    adi_ether_Read(adi_ether->hEthernet, pktBuffer);
    sys_mutex_unlock(&adi_ether->readLock);
}

/* This function initializes the zero copy Rx pbufs */
static void
adi_ether_netif_init_aePbuf(adi_ether_netif *adi_ether)
{
    adi_ether_pbuf_t *aep;
    int i;

    for (i = 0; i < ADI_ETHER_NUM_RX_BUFFS; i++) {
        aep = &adi_ether->aePbuf[i];
        aep->adi_ether = adi_ether;
        aep->p.custom_free_function = adi_ether_netif_pbuf_free;
        aep->busy = false;
    }

    adi_ether->aePbufHead = 0;
}

/* This function readies and unlinks an ADI_ETHER_BUFFER for transmission */
static void
adi_ether_netif_reset_tx_buff(ADI_ETHER_BUFFER *txBuff)
{
    txBuff->ElementCount = 0;
    txBuff->ElementWidth = 1;
    txBuff->ProcessedFlag = 0;
    txBuff->CallbackParameter = 0;
    txBuff->Flag = 0;
#ifdef ADI_ETHER_SUPPORT_AV
    txBuff->nChannel = 0;
#endif
    txBuff->pNext = NULL;
}

/* This function initializes the Ethernet driver Tx buffers */
static void
adi_ether_netif_init_tx(adi_ether_netif *adi_ether)
{
    ADI_ETHER_BUFFER *txBuff;
    ADI_ETHER_PACKET_DATA *txPktData;
    int i;

    txBuff = adi_ether->txBuff;
    txPktData = adi_ether->txPktData;

    /* Initialize the Tx buffers */
    for (i = 0; i < ADI_ETHER_NUM_TX_BUFFS; i++) {
        txBuff[i].Data = &txPktData[i].data;
        adi_ether_netif_reset_tx_buff(&txBuff[i]);
    }
}

/*
 * Queue a frame received from ptp or lwIP for transmit.
 * WARNING: This function assumes ETH_PAD_SIZE is 2
 */
static err_t
adi_ether_netif_tx_frame(struct netif *netif, struct pbuf *p, bool ptpCB)
{
    adi_ether_netif *adi_ether = netif->state;
    uint16_t len;
    struct pbuf *q;
    uint8_t *out;
    ADI_ETHER_BUFFER *txBuff;
    ADI_ETHER_RESULT etherResult;
    uint16_t txPktNext;

    /* Make sure to never exceed the TX buffer pool */
    txPktNext = adi_ether->txPktHead + 1;
    if (txPktNext == ADI_ETHER_NUM_TX_BUFFS) {
        txPktNext = 0;
    }
    if (txPktNext == adi_ether->txPktTail) {
        LINK_STATS_INC(link.drop);
        return(ERR_OK);
    }

    /* Grab the next TX buffer */
    txBuff = &adi_ether->txBuff[adi_ether->txPktHead];

    /* Insert the lwIP payload into the frame */
    out = (uint8_t *)txBuff->Data;
    for (q = p; q != NULL; q = q->next) {
        memcpy(out, q->payload, q->len);
        out += q->len;
    }

    /* Put the total length (including 2 bytes of padding which
     * is the same as the size field) into the first 2 bytes of the
     * frame.
     */
    len = p->tot_len;
    *((uint16_t *)txBuff->Data) = len;

    /* Prepare the ADI_ETHER_BUFFER for transmission by the driver */
    adi_ether_netif_reset_tx_buff(txBuff);
    txBuff->ElementCount = len;

    /* Request a timestamp for PTP packets */
    if (ptpCB) {
        txBuff->CallbackParameter = (void *)1;
        txBuff->Flag |= ADI_ETHER_BUFFER_FLAG_TX_TIMESTAMP_EN;
    }

    /* Send it out if the link is up */
    if (adi_ether->linkUp) {
        etherResult = adi_ether_Write(adi_ether->hEthernet, txBuff);
        if (etherResult == ADI_ETHER_RESULT_SUCCESS) {
            adi_ether->txPktHead = txPktNext;
            LINK_STATS_INC(link.xmit);
        } else {
            LINK_STATS_INC(link.drop);
        }
    } else {
        LINK_STATS_INC(link.drop);
    }

    return(ERR_OK);
}

/*
 * Queue a frame received from lwIP for transmit.
 * WARNING: This function assumes ETH_PAD_SIZE is 2
 */
static err_t
adi_ether_netif_lwip_tx_frame(struct netif *netif, struct pbuf *p)
{
    adi_ether_netif *adi_ether = netif->state;
    err_t err;

    sys_mutex_lock(&adi_ether->writeLock);
    err = adi_ether_netif_tx_frame(netif, p, false);
    sys_mutex_unlock(&adi_ether->writeLock);

    return (err);
}

/*
 * Queue a ptp frame for transmit.
 */
err_t
adi_ether_netif_ptp_tx_frame(struct netif *netif, uint8_t *srcMacAddr,
    uint8_t *dstMacAddr, uint16_t etherType, uint8_t *data, uint16_t len,
    bool txCallback)
{
    adi_ether_netif *adi_ether = netif->state;
    struct pbuf p[5];
    uint8_t pad[2];
    uint8_t eType[2];
    err_t err;

    /* 2 byte alignment padding */
    p[0].len = 2;
    p[0].payload = pad;
    p[0].next = &p[1];

    /* Dest MAC */
    p[1].len = 6;
    p[1].payload = dstMacAddr;
    p[1].next = &p[2];

    /* Src MAC */
    p[2].len = 6;
    p[2].payload = srcMacAddr;
    p[2].next = &p[3];

    /* Ethertype */
    p[3].len = 2;
    p[3].payload = eType;
    p[3].next = &p[4];
    eType[0] = (etherType >> 8) & 0xFF;
    eType[1] = (etherType >> 0) & 0xFF;

    /* Data */
    p[4].len = len;
    p[4].payload = data;
    p[4].next = NULL;

    p[0].tot_len = p[0].len + p[1].len +p[2].len + p[3].len + p[4].len;

    sys_mutex_lock(&adi_ether->writeLock);
    err = adi_ether_netif_tx_frame(netif, p, txCallback);
    sys_mutex_unlock(&adi_ether->writeLock);

    return (err);
}

/*
 * Queue a 1722 frame for transmit.
 */
err_t
adi_ether_netif_1722_tx_frame(struct netif *netif, uint8_t *srcMacAddr,
    uint8_t *dstMacAddr, uint16_t etherType, uint8_t *p1722, uint16_t p1722Len,
    uint8_t *audio, uint16_t audioLen)
{
    adi_ether_netif *adi_ether = netif->state;
    struct pbuf p[6];
    uint8_t pad[2];
    uint8_t eType[2];
    err_t err;

    /* 2 byte alignment padding */
    p[0].len = 2;
    p[0].payload = pad;
    p[0].next = &p[1];

    /* Dest MAC */
    p[1].len = 6;
    p[1].payload = dstMacAddr;
    p[1].next = &p[2];

    /* Src MAC */
    p[2].len = 6;
    p[2].payload = srcMacAddr;
    p[2].next = &p[3];

    /* Ethertype */
    p[3].len = 2;
    p[3].payload = eType;
    p[3].next = &p[4];
    eType[0] = (etherType >> 8) & 0xFF;
    eType[1] = (etherType >> 0) & 0xFF;

    /* 1722 Data */
    p[4].len = p1722Len;
    p[4].payload = p1722;
    p[4].next = NULL;

    p[0].tot_len = p[0].len + p[1].len +p[2].len + p[3].len + p[4].len;

    /* Audio Data */
    if (audio) {
        p[4].next = &p[5];
        p[5].len = audioLen;
        p[5].payload = audio;
        p[5].next = NULL;
        p[0].tot_len += p[5].len;
    }

    sys_mutex_lock(&adi_ether->writeLock);
    err = adi_ether_netif_tx_frame(netif, p, NULL);
    sys_mutex_unlock(&adi_ether->writeLock);

    return(err);
}

/*
 * Receive a frame into lwIP.
 * WARNING: This function assumes ETH_PAD_SIZE is 2
 */
static void
adi_ether_netif_lwip_rx_frame(struct adi_ether_netif *adi_ether, ADI_ETHER_BUFFER *rxBuff)
{
    adi_ether_pbuf_t *aep;
    struct pbuf *p;
    uint16_t len;
    uint8_t *in;

    /* Get the total length from the first 2 bytes of the frame */
    in = (uint8_t *)rxBuff->Data;
    len = *((uint16_t *)in);

    /* Scan for a free custom pbuf container.  There will always be
     * at least one or we wouldn't be here.
     */
    do {
        aep = &adi_ether->aePbuf[adi_ether->aePbufHead++];
        if (adi_ether->aePbufHead == ADI_ETHER_NUM_RX_BUFFS) {
            adi_ether->aePbufHead = 0;
        }
    } while (aep->busy);

    /* Attach the receive buffer to it */
    aep->pktBuffer = rxBuff;
    aep->busy = true;

    /* Allocate a pbuf by reference and submit to lwIP */
    p = pbuf_alloced_custom(
        PBUF_RAW, len, PBUF_REF, &aep->p, in, ETHERNET_MAX_SIZE
    );
    if (p) {
        if (adi_ether->netif->input(p, adi_ether->netif) == ERR_OK) {
            LINK_STATS_INC(link.recv);
        } else {
            LINK_STATS_INC(link.drop);
            pbuf_free(p);
        }
    } else {
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
    }
}

/*
 * Receive a ptp rx/tx frame into ptp.
 * WARNING: This function assumes ETH_PAD_SIZE is 2
 */
static void
adi_ether_netif_ptp_frame(struct adi_ether_netif *adi_ether, ADI_ETHER_BUFFER *pktBuffer, uint8_t pktType)
{
    uint16_t len;
    uint8_t *in;

    /* Get the length from the first 2 bytes of the frame */
    in = (uint8_t *)pktBuffer->Data;
    len = *((uint16_t *)in);

    /* Skip the length field */
    in += sizeof(uint16_t);

    /* Call the callback */
    if (adi_ether->ptpPktCb) {
        adi_ether->ptpPktCb(adi_ether, in, len, pktType,
            pktBuffer->TimeStamp.LSecond, pktBuffer->TimeStamp.NanoSecond);
    }

    /* Return receive pktBuffers to the driver */
    if (pktType == ADI_ETHER_PKT_TYPE_RX) {
        adi_ether_netif_reset_rx_buff(pktBuffer);
        sys_mutex_lock(&adi_ether->readLock);
        adi_ether_Read(adi_ether->hEthernet, pktBuffer);
        sys_mutex_unlock(&adi_ether->readLock);
    }
}

/*
 * Check if a frame is a 1722 frame
 */
#define ETHTYPE_AVBTP   0x22f0
static bool isP1722(ADI_ETHER_BUFFER *pktBuffer)
{
    struct eth_hdr *ethhdr;
    struct eth_vlan_hdr *vlan;
    uint8_t *in;
    bool is1722 = false;

    /* Get the length from the first 2 bytes of the frame */
    in = (uint8_t *)pktBuffer->Data;

    /* Skip the length field */
    in += sizeof(uint16_t);

    /* Examine the Ethernet header */
    ethhdr = (struct eth_hdr *)in;

    /* Identify and process 1722 AAF frames */
    if (ethhdr->type == __htons(ETHTYPE_VLAN)) {
        vlan = (struct eth_vlan_hdr *)(in + SIZEOF_ETH_HDR);
        if (vlan->tpid == __htons(ETHTYPE_AVBTP)) {
            is1722 = true;
        }
    }

    return(is1722);
}

/*
 * Receive a p1722 rx frame into 1722 stack.
 * WARNING: This function assumes ETH_PAD_SIZE is 2
 */
static void
adi_ether_netif_p1722_frame(struct adi_ether_netif *adi_ether, ADI_ETHER_BUFFER *pktBuffer)
{
    uint16_t len;
    uint8_t *in;

    /* Get the length from the first 2 bytes of the frame */
    in = (uint8_t *)pktBuffer->Data;
    len = *((uint16_t *)in);

    /* Skip the length field */
    in += sizeof(uint16_t);

    /* Call the callback */
    if (adi_ether->p1722PktCb) {
        adi_ether->p1722PktCb(adi_ether, in, len);
    }

    /* Return receive pktBuffers to the driver */
    adi_ether_netif_reset_rx_buff(pktBuffer);
    sys_mutex_lock(&adi_ether->readLock);
    adi_ether_Read(adi_ether->hEthernet, pktBuffer);
    sys_mutex_unlock(&adi_ether->readLock);
}

/* Worker thread to process events from the driver */
static void
adi_ether_netif_worker(void *pvParameters)
{
    struct adi_ether_netif *adi_ether = pvParameters;
    struct netif *netif = adi_ether->netif;
    ADI_ETHER_BUFFER *pktReceivedBuffer;
    ADI_ETHER_BUFFER *pktTransmittedBuffer;
    ADI_ETHER_BUFFER *pktBuffer;
    ADI_ETHER_BUFFER *pNext;
    sys_prot_t protect;
    void *toDo;

    while (1) {
        sys_arch_mbox_fetch(&adi_ether->workerToDo, &toDo, 0);
        switch ((int)toDo) {
            case WORKER_LINK_UP:
                netif_set_link_up(netif);
                break;
            case WORKER_LINK_DOWN:
                netif_set_link_down(netif);
                break;
            case WORKER_PKT_RX:
                protect = sys_arch_protect();
                pktReceivedBuffer = adi_ether->pktReceivedBuffer;
                adi_ether->pktReceivedBuffer = NULL;
                sys_arch_unprotect(protect);
                pktBuffer = pktReceivedBuffer;
                while (pktBuffer) {
                    pNext = pktBuffer->pNext;
                    pktBuffer->pNext = NULL;
                    if (pktBuffer->Status & ADI_ETHER_BUFFER_STATUS_TIMESTAMP_AVAIL) {
                        adi_ether_netif_ptp_frame(adi_ether, pktBuffer, ADI_ETHER_PKT_TYPE_RX);
                    } else if (isP1722(pktBuffer)) {
                        adi_ether_netif_p1722_frame(adi_ether, pktBuffer);
                    } else {
                        adi_ether_netif_lwip_rx_frame(adi_ether, pktBuffer);
                    }
                    pktBuffer = pNext;
                }
                break;
            case WORKER_PKT_TX:
                protect = sys_arch_protect();
                pktTransmittedBuffer = adi_ether->pktTransmittedBuffer;
                adi_ether->pktTransmittedBuffer = NULL;
                sys_arch_unprotect(protect);
                pktBuffer = pktTransmittedBuffer;
                while (pktBuffer) {
                    if (pktBuffer->CallbackParameter) {
                        adi_ether_netif_ptp_frame(adi_ether, pktBuffer, ADI_ETHER_PKT_TYPE_TX);
                    }
                    adi_ether->txPktTail++;
                    if (adi_ether->txPktTail == ADI_ETHER_NUM_TX_BUFFS) {
                        adi_ether->txPktTail = 0;
                    }
                    pktBuffer = pktBuffer->pNext;
                }
                break;


            default:
                break;
        }
    }
}

/* Driver callback handling tx done, rx done, and phy link status */
static void
adi_ether_netif_callback(void *pDev, uint32_t event, void *param, void *usr)
{
    adi_ether_netif *adi_ether = (adi_ether_netif *)usr;
    ADI_ETHER_BUFFER *pBuffer = (ADI_ETHER_BUFFER *)param;
    ADI_ETHER_BUFFER *append;
    uint32_t status;

    xInsideISR = 1;

    switch(event)
    {
        case ADI_ETHER_EVENT_FRAME_XMIT:
            if (adi_ether->pktTransmittedBuffer) {
                append = adi_ether->pktTransmittedBuffer;
                while (append->pNext) {
                    append = append->pNext;
                }
                append->pNext = pBuffer;
            } else {
                adi_ether->pktTransmittedBuffer = pBuffer;
                sys_mbox_trypost(&adi_ether->workerToDo, (void *)WORKER_PKT_TX);
            }
            break;

        case ADI_ETHER_EVENT_FRAME_RCVD:
            if (adi_ether->pktReceivedBuffer) {
                append = adi_ether->pktReceivedBuffer;
                while (append->pNext) {
                    append = append->pNext;
                }
                append->pNext = pBuffer;
            } else {
                adi_ether->pktReceivedBuffer = pBuffer;
                sys_mbox_trypost(&adi_ether->workerToDo, (void *)WORKER_PKT_RX);
            }
            break;

        case ADI_ETHER_EVENT_PHY_INTERRUPT:
            status = (uint32_t)param;
            if (status & ADI_ETHER_PHY_AN_COMPLETE) {
                adi_ether->linkUp = true;
                sys_mbox_trypost(&adi_ether->workerToDo, (void *)WORKER_LINK_UP);
            } else if (status & ADI_ETHER_PHY_LINK_DOWN) {
                adi_ether->linkUp = false;
                sys_mbox_trypost(&adi_ether->workerToDo, (void *)WORKER_LINK_DOWN);
            }
            break;

        default:
            break;
    }

    xInsideISR = 0;
}

/* This function initializes the ADI EMAC driver */
static void
adi_ether_netif_low_level_init(struct netif *netif)
{
    struct adi_ether_netif *adi_ether = netif->state;
    ADI_ETHER_RESULT etherResult;
    ADI_ETHER_DEV_INIT adiEtherInitData;
    ADI_ETHER_DRIVER_ENTRY *driverEntry;

    /* maximum transfer unit */
    netif->mtu = 1500;

    /* device capabilities */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    /* Set ADI EMAC driver entry point */
    if (adi_ether->port == EMAC0) {
        driverEntry = &GEMAC0DriverEntry;
    } else if (adi_ether->port == EMAC1) {
        driverEntry = &GEMAC1DriverEntry;
    } else {
        return;
    }

    /* Initialize the ADI EMAC driver memory */
    memset(&adiEtherInitData, 0, sizeof(adiEtherInitData));
    adiEtherInitData.Cache = true;
    adiEtherInitData.pEtherMemory = &adi_ether->etherMemTable;
    adiEtherInitData.pCmdArgArray = NULL;

    /* Open the ADI EMAC driver */
    etherResult = adi_ether_Open(driverEntry, &adiEtherInitData,
        adi_ether_netif_callback, &adi_ether->hEthernet, adi_ether);
    if (etherResult != ADI_ETHER_RESULT_SUCCESS) {
        return;
    }

    /* Set the MAC hardware address in the driver */
    etherResult = adi_ether_SetMACAddress(
        adi_ether->hEthernet, adi_ether->ethAddr.addr);
    if (etherResult != ADI_ETHER_RESULT_SUCCESS) {
        return;
    }

    /* Set the MAC hardware address in lwIP */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    etherResult = adi_ether_GetMACAddress(adi_ether->hEthernet, netif->hwaddr);
    if (etherResult != ADI_ETHER_RESULT_SUCCESS) {
        return;
    }

    /* Give the receive rxBuff chain to the driver */
    etherResult = adi_ether_Read(adi_ether->hEthernet, adi_ether->rxBuff);
    if (etherResult != ADI_ETHER_RESULT_SUCCESS) {
        return;
    }

    /* Create a semaphore for the worker thread */
    sys_mbox_new(&adi_ether->workerToDo, ADI_ETHER_NUM_MBOX_EVENTS);

    /* Create read/write locks for lwIP/PTP accesses */
    sys_mutex_new(&adi_ether->readLock);
    sys_mutex_new(&adi_ether->writeLock);

    /* Spin up the worker thread */
    adi_ether->worker = sys_thread_new(
        "adi_ether_netif_worker", adi_ether_netif_worker, adi_ether,
        TCPIP_THREAD_STACKSIZE, ETHER_WORKER_PRIO
    );

    /* Enable the MAC */
    etherResult = adi_ether_EnableMAC(adi_ether->hEthernet);
    if (etherResult != ADI_ETHER_RESULT_SUCCESS) {
        return;
    }

    /* Indicate initialization OK */
    adi_ether->initOk = true;
}

/*************************************************************************
 * Public APIs
 ************************************************************************/
err_t
adi_ether_netif_init(struct netif *netif)
{
    struct adi_ether_netif *adi_ether = netif->state;

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    if (adi_ether->hostName) {
        netif->hostname = adi_ether->hostName;
    }
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    netif->num = adi_ether->idx;

#if LWIP_IPV4
    netif->output = etharp_output;
#endif
    netif->linkoutput = adi_ether_netif_lwip_tx_frame;

    /* Save a ref to the netif for callback link processing */
    adi_ether->netif = netif;

    /* Initialize the ADI EMAC driver Rx/Tx buffers */
    adi_ether_netif_init_rx(adi_ether);
    adi_ether_netif_init_tx(adi_ether);

    /* Initialize the zero copy Rx custom pbufs */
    adi_ether_netif_init_aePbuf(adi_ether);

    /* Do low-level ADI EMAC hardware init */
    adi_ether_netif_low_level_init(netif);

    return ERR_OK;
}

#if 0
static adi_ether_netif netifs[ADI_ETHER_MAX_NETIFS]
    __attribute__ ((section(".l3_uncached_data"))) = { 0 };
#else
static adi_ether_netif netifs[ADI_ETHER_MAX_NETIFS]
    __attribute__((aligned (32)));
#endif

adi_ether_netif *
adi_ether_netif_new(void *usrPtr)
{
    adi_ether_netif *adi_ether;
    int i;

    /* Find an available netif */
    for (i = 0; i < ADI_ETHER_MAX_NETIFS; i++) {
        adi_ether = &netifs[i];
        if (!adi_ether->allocated) {
            break;
        }
    }

    /* Return NULL if no netifs available */
    if (i == ADI_ETHER_MAX_NETIFS) {
        return(NULL);
    }

    /* Mark instance as being allocated */
    adi_ether->allocated = true;
    adi_ether->idx = i;

    /* Reset Rx/Tx packet buffers */
    ADI_ETHER_MEMSET(adi_ether->rxBuff, 0, sizeof(adi_ether->rxBuff));
    ADI_ETHER_MEMSET(adi_ether->txBuff, 0, sizeof(adi_ether->txBuff));
    ADI_ETHER_MEMSET(adi_ether->rxPktData, 0, sizeof(adi_ether->rxPktData));
    ADI_ETHER_MEMSET(adi_ether->txPktData, 0, sizeof(adi_ether->txPktData));

    /* Set up the EMAC driver DMA descriptor memory */
    adi_ether->etherMemTable.BaseMemLen = 0;
    adi_ether->etherMemTable.pBaseMem = NULL;
    adi_ether->etherMemTable.RecvMemLen = sizeof(adi_ether->RecvMem);
    adi_ether->etherMemTable.pRecvMem = adi_ether->RecvMem;
    adi_ether->etherMemTable.TransmitMemLen = sizeof(adi_ether->TransmitMem);
    adi_ether->etherMemTable.pTransmitMem = adi_ether->TransmitMem;

    /* Save the user pointer */
    adi_ether->usrPtr = usrPtr;

    return(adi_ether);
}

void
adi_ether_netif_delete(adi_ether_netif *adi_ether)
{
    if (adi_ether->hEthernet) {
        adi_ether_Close(adi_ether->hEthernet);
    }

    /* TODO: kill the worker thread here */

    ADI_ETHER_MEMSET(adi_ether, 0, sizeof(*adi_ether));
}

err_t
adi_ether_netif_set_src_addr_filt(struct netif *netif,
    const uint32_t ipAddr, const uint8_t ipMaskBits, const bool invert)
{
    struct adi_ether_netif *adi_ether = netif->state;
    adi_ether_SetSrcAddrFilt(adi_ether->hEthernet, ipAddr, ipMaskBits, invert);
    return ERR_OK;
    }

err_t
adi_ether_netif_set_src_addr_filt_enable(struct netif *netif,
    const bool enable)
{
    struct adi_ether_netif *adi_ether = netif->state;
    adi_ether_SetSrcAddrFiltEnable(adi_ether->hEthernet, enable);
    return ERR_OK;
    }

err_t
adi_ether_netif_set_dst_port_filt(struct netif *netif,
    const uint16_t port, const bool udp, const bool invert)
{
    struct adi_ether_netif *adi_ether = netif->state;
    adi_ether_SetDstPortFilt(adi_ether->hEthernet, port, udp, invert);
    return ERR_OK;
    }

err_t
adi_ether_netif_set_dst_port_filt_enable(struct netif *netif,
    const bool enable)
{
    struct adi_ether_netif *adi_ether = netif->state;
    adi_ether_SetDstPortFiltEnable(adi_ether->hEthernet, enable);
    return ERR_OK;
}
