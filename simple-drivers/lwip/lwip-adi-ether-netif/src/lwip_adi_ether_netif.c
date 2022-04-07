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

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/etharp.h"

#include "lwip_adi_ether_netif.h"

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
    WORKER_PKT_RX
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
    rxBuff->nChannel = 0;
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

/* This function readies and unlinks an ADI_ETHER_BUFFER for transmission */
static void
adi_ether_netif_reset_tx_buff(ADI_ETHER_BUFFER *txBuff)
{
    txBuff->ElementCount = 0;
    txBuff->ElementWidth = 1;
    txBuff->ProcessedFlag = 0;
    txBuff->nChannel = 0;
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

/* Queue a frame received from lwIP for transmit */
static err_t
adi_ether_netif_lwip_tx_frame(struct netif *netif, struct pbuf *p)
{
    adi_ether_netif *adi_ether = netif->state;
    uint16_t len;
    struct pbuf *q;
    uint8_t *out;
    ADI_ETHER_BUFFER *txBuff;
    ADI_ETHER_RESULT etherResult;

    /*
     * TODO: Track 'txPktTail' here when this event is fixed
     * to check for overruns.
     */

    /* Grab the next TX buffer */
    txBuff = &adi_ether->txBuff[adi_ether->txPktHead];

    /* Put the total length into the first 2 bytes of the frame */
    len = p->tot_len + sizeof(uint16_t);

    out = (uint8_t *)txBuff->Data;
    *((uint16_t *)out) = len;

    out += sizeof(uint16_t);

    /* Insert the lwIP payload into the frame */
    for (q = p; q != NULL; q = q->next) {
        memcpy(out, q->payload, q->len);
        out += q->len;
    }

    /* Prepare the ADI_ETHER_BUFFER for transmission by the driver */
    adi_ether_netif_reset_tx_buff(txBuff);
    txBuff->ElementCount = len;

    /* Send it out if the link is up */
    if (adi_ether->linkUp) {
        etherResult = adi_ether_Write(adi_ether->hEthernet, txBuff);
        if (etherResult == ADI_ETHER_RESULT_SUCCESS) {
            adi_ether->txPktHead++;
            if (adi_ether->txPktHead == ADI_ETHER_NUM_TX_BUFFS) {
                adi_ether->txPktHead = 0;
            }
            LINK_STATS_INC(link.xmit);
        } else {
            LINK_STATS_INC(link.drop);
        }
    } else {
        LINK_STATS_INC(link.drop);
    }

    return(ERR_OK);
}

/* Receive a frame into lwIP */
static void
adi_ether_netif_lwip_rx_frame(struct netif *netif, ADI_ETHER_BUFFER *rxBuff)
{
    uint16_t len;
    struct pbuf *p, *q;
    uint8_t *in;
    err_t err;

    /* Get the total length from the first 2 bytes of the frame */
    in = (uint8_t *)rxBuff->Data;
    len = *((uint16_t *)in);

    in += 2;
    len -= 2;

    /* Allocate an lwIP pbuf to carry the data, copy, and submit it */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

    if (p != NULL) {
        for (q = p; q != NULL; q = q->next) {
            memcpy(q->payload, in, q->len);
            in += q->len;
        }
        err = netif->input(p, netif);
        if (err == ERR_OK) {
            LINK_STATS_INC(link.recv);
        } else {
            pbuf_free(p);
        }
    } else {
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
    }

}

/* Worker thread to process events from the driver */
static void
adi_ether_netif_worker(void *pvParameters)
{
    struct adi_ether_netif *adi_ether = pvParameters;
    struct netif *netif = adi_ether->netif;
    ADI_ETHER_BUFFER *pktReceivedBuffer;
    ADI_ETHER_RESULT etherResult;
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
                while (pktReceivedBuffer) {
                    adi_ether_netif_lwip_rx_frame(netif, pktReceivedBuffer);
                    adi_ether_netif_reset_rx_buff(pktReceivedBuffer);
                    pktReceivedBuffer = pktReceivedBuffer->pNext;
                }
                etherResult = adi_ether_Read(
                    adi_ether->hEthernet, adi_ether->pktReceivedBuffer
                );
                if (etherResult != ADI_ETHER_RESULT_SUCCESS) {
                    asm("nop;");
                }
                adi_ether->pktReceivedBuffer = NULL;
                sys_arch_unprotect(protect);
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
            /* TODO: Track 'txPktTail' here when this event is fixed */
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

    /* Spin up the worker thread */
    adi_ether->worker = sys_thread_new(
        "adi_ether_netif_worker", adi_ether_netif_worker, adi_ether,
        TCPIP_THREAD_STACKSIZE, TCPIP_THREAD_PRIO
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

#if LWIP_IPV4
    netif->output = etharp_output;
#endif
    netif->linkoutput = adi_ether_netif_lwip_tx_frame;

    /* Save a ref to the netif for callback link processing */
    adi_ether->netif = netif;

    /* Initialize the ADI EMAC driver Rx/Tx buffers */
    adi_ether_netif_init_rx(adi_ether);
    adi_ether_netif_init_tx(adi_ether);

    /* Do low-level ADI EMAC hardware init */
    adi_ether_netif_low_level_init(netif);

    return ERR_OK;
}

adi_ether_netif *
adi_ether_netif_new(void)
{
    adi_ether_netif *adi_ether;

    adi_ether = ADI_ETHER_CALLOC(1, sizeof(*adi_ether));

    adi_ether->rxBuff = ADI_ETHER_CALLOC(
        ADI_ETHER_NUM_RX_BUFFS, sizeof(*adi_ether->rxBuff)
    );
    adi_ether->txBuff = ADI_ETHER_CALLOC(
        ADI_ETHER_NUM_TX_BUFFS, sizeof(*adi_ether->txBuff)
    );
    adi_ether->rxPktData = ADI_ETHER_CALLOC(
        ADI_ETHER_NUM_RX_BUFFS, sizeof(*adi_ether->rxPktData)
    );
    adi_ether->txPktData = ADI_ETHER_CALLOC(
        ADI_ETHER_NUM_TX_BUFFS, sizeof(*adi_ether->txPktData)
    );

    adi_ether->etherMemTable.BaseMemLen = ADI_ETHER_BASE_MEM_SIZE;
    adi_ether->etherMemTable.pBaseMem = ADI_ETHER_CALLOC(
        adi_ether->etherMemTable.BaseMemLen, 1
    );
    adi_ether->etherMemTable.RecvMemLen =
        ADI_ETHER_NUM_RECV_DESC * ADI_ETHER_DMA_DESCRIPTOR_SIZE;
    adi_ether->etherMemTable.pRecvMem = ADI_ETHER_CALLOC(
        adi_ether->etherMemTable.RecvMemLen, 1
    );
    adi_ether->etherMemTable.TransmitMemLen =
        ADI_ETHER_NUM_XMIT_DESC * ADI_ETHER_DMA_DESCRIPTOR_SIZE;
    adi_ether->etherMemTable.pTransmitMem = ADI_ETHER_CALLOC(
        adi_ether->etherMemTable.TransmitMemLen, 1
    );

    return(adi_ether);
}

void
adi_ether_netif_delete(adi_ether_netif *adi_ether)
{
    if (adi_ether->hEthernet) {
        adi_ether_Close(adi_ether->hEthernet);
    }

    /* TODO: kill the worker thread here */

    if (adi_ether->etherMemTable.pTransmitMem) {
        ADI_ETHER_FREE(adi_ether->etherMemTable.pTransmitMem);
    }
    if (adi_ether->etherMemTable.pRecvMem) {
        ADI_ETHER_FREE(adi_ether->etherMemTable.pRecvMem);
    }
    if (adi_ether->etherMemTable.pBaseMem) {
        ADI_ETHER_FREE(adi_ether->etherMemTable.pBaseMem);
    }
    if (adi_ether->txPktData) {
        ADI_ETHER_FREE(adi_ether->txPktData);
    }
    if (adi_ether->rxPktData) {
        ADI_ETHER_FREE(adi_ether->rxPktData);
    }
    if (adi_ether->txBuff) {
        ADI_ETHER_FREE(adi_ether->txBuff);
    }
    if ( adi_ether->rxBuff) {
        ADI_ETHER_FREE( adi_ether->rxBuff);
    }
}
