/*!
*********************************************************************************
 *
 * @file:    adi_gemac_int.h
 *
 * @brief:   Ethernet GEMAC driver internal header file
 *
 * @version: $Revision: 27441 $
 *
 * @date:    $Date: 2016-08-26 05:18:22 -0400 (Fri, 26 Aug 2016) $
 * ------------------------------------------------------------------------------
 *
 * Copyright (c) 2011-2016 Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Modified versions of the software must be conspicuously marked as such.
 * - This software is licensed solely and exclusively for use with processors
 *   manufactured by or for Analog Devices, Inc.
 * - This software may not be combined or merged with other code in any manner
 *   that would cause the software to become subject to terms and conditions
 *   which differ from those listed here.
 * - Neither the name of Analog Devices, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 * - The use of this software may or may not infringe the patent rights of one
 *   or more patent holders.  This license does not release you from the
 *   requirement that you obtain separate licenses from these patent holders
 *   to use this software.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF CLAIMS OF INTELLECTUAL
 * PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
#ifndef _ADI_GEMAC_INT_H_
#define _ADI_GEMAC_INT_H_
#include <stdlib.h>
#include <drivers/ethernet/adi_ether.h>
#include <drivers/ethernet/adi_ether_gemac.h>
#include <drivers/ethernet/adi_ether_misra.h>
#include "adi_gemac_proc_int.h"
#include "adi_phy_int.h"
#include <string.h>


/*! Statistical collection is enabled if ADI_ETHER_DEBUG macro is defined */
#if defined(ADI_ETHER_DEBUG)
#include <stdio.h>
	#define ETHER_PRINT(str)      (printf((str)))       /*!< Enable debug prints */
        #define ETHER_DEBUG(str,val)  (printf((str),(val))) /*!< Enable debug prints */
        #define ADI_ETHER_STATISTICS            /*!< enable statistics             */
        #define STATS_INC(x) (x++)              /*!< increment statistical counters */
#else
	#define ETHER_PRINT(str)                /*!< disable debug prints */
        #define ETHER_DEBUG(str,val)            /*!< disable debug prints */
        #define STATS_INC(x)                    /*!< disable statistic collection */
#endif  /* ADI_ETHER_DEBUG */

#define ADI_EMAC_MIN_DMEM_SZ       (4 *32)  /*!< minimum number of dma descriptors per channel */
#define ADI_EMAC_DMAOWN            (1UL << 31) /*!< Set when DMA owns the descriptor */
#define ADI_EMAC_DMA_DESC_TX_TM_EN (1UL <<)
#define ADI_EMAC_NUM_MCAST_BINS    (64)   /*!< number of multicast bins supported by controller */

#define MAC_LOOPCOUNT    1000000000          /*!< counter used as timeout for MAC operations */

#if defined(ADI_ETHER_STATISTICS)
/*! EMAC statistics */
typedef struct ADI_EMAC_STATS
{
    int32_t TxIntCnt;
    int32_t TxProcessStopCnt;
    int32_t TxBufferUnAvailCnt;
    int32_t TxJabberTimeOutCnt;
    int32_t RxOvfCnt;
    int32_t TxUnfCnt;
    int32_t RxIntCnt;
    int32_t RxBufferUnAvailCnt;
    int32_t RxProcessStopCnt;
    int32_t RxWDTimeoutCnt;
    int32_t EarlyTxIntCnt;
    int32_t BusErrorCnt;
    int32_t EarlyRxIntCnt;
    int32_t MMCIntCnt;

} ADI_EMAC_STATS;
#endif  /* ADI_ETHER_STATISTICS */

/*! EMAC Register map including the padding */
typedef volatile struct ADI_EMAC_REGISTERS
{
     uint32_t EMAC_MACCFG;                 /*!< mac configuration register */
     uint32_t EMAC_MACFRMFILT;             /*!< mac frame filter register  */
     uint32_t EMAC_HASHTBL_HI;             /*!< hash table register low    */
     uint32_t EMAC_HASHTBL_LO;             /*!< has table register high    */
     uint32_t EMAC_SMI_ADDR;               /*!< MII address register       */
     uint32_t EMAC_SMI_DATA;               /*!< MII data register          */
     uint32_t EMAC_FLOWCTL;                /*!< flow control register      */
     uint32_t EMAC_VLANTAG;                /*!< vlan tag register          */
     uint8_t  PAD_1[4];                    /*!< pad 1 */
     uint32_t EMAC_DBG;                    /*!< debug register             */
     uint8_t  PAD_2[8];                    /*!< pad 2 */
#if defined (GEMAC_SUPPORT_LPI)
     uint32_t EMAC_LPI_CTLSTAT;            /*!< LPI Control and Status Register */
     uint32_t EMAC_LPI_TMRSCTL;            /*!< LPI Timeout Register */
#else
     uint8_t  PAD_LPI[8];                  /*!< pad LPI */
#endif
     uint32_t EMAC_ISTAT;                  /*!< mac interrupt status       */
     uint32_t EMAC_IMSK;                   /*!< mac imask                  */
     uint32_t EMAC_ADDR0_HI;               /*!< MAC 0 address high            */
     uint32_t EMAC_ADDR0_LO;               /*!< MAC 0 address low           */

#if defined (GEMAC_SUPPORT_MAC_ADDR1)
     uint32_t EMAC_ADDR1_HI;               /*!< MAC 1 address high            */
     uint32_t EMAC_ADDR1_LO;               /*!< MAC 1 address low           */
#else
     uint8_t  PAD_ADDR1[8];                /*!< pad ADDR1 */
#endif

     uint8_t  PAD_3[136];                  /*!< pad 3 */

#if defined (GEMAC_SUPPORT_RGMII)
     uint32_t EMAC_GIGE_CTLSTAT;
#else
     uint8_t  PAD_RGMII[4];
#endif

#if defined (GEMAC_SUPPORT_WDOG)
     uint32_t EMAC_WDOG_TIMOUT;
#else
     uint8_t  PAD_WDOG[4];
#endif

     uint8_t  PAD_4[32];                   /*!< pad 4 */

     uint32_t EMAC_MMC_CTL;                /*!< mmc control register       */
     uint32_t EMAC_MMC_RXINT;              /*!< mmc rx interrupt           */
     uint32_t EMAC_MMC_TXINT;              /*!< mmc tx interrupt           */
     uint32_t EMAC_MMC_RXIMSK;             /*!< rx imask register          */
     uint32_t EMAC_MMC_TXIMSK;             /*!< tx imask register          */
     uint32_t EMAC_TXOCTCNT_GB;            /*!< no. tx bytes good and bad frames */
     uint32_t EMAC_TXFRMCNT_GB;            /*!< no. tx frames transmitted  */
     uint32_t EMAC_TXBCASTFRM_G;           /*!< no. of tx broadcast frames */
     uint32_t EMAC_TXMCASTFRM_G;           /*!< no. of tx multicast frames */
     uint32_t EMAC_TX64_GB;                /*!< no. of tx frames with length = 64 */
     uint32_t EMAC_TX65TO127_GB;           /*!< no. of tx frames with length between 65 and 127 */
     uint32_t EMAC_TX128TO255_GB;          /*!< no. of tx frames with length between 128 and 255 */
     uint32_t EMAC_TX256TO511_GB;          /*!< no. of tx frames with length between 256 and 512 */
     uint32_t EMAC_TX512TO1023_GB;         /*!< no. of tx frames with length between 512 and 1023 */
     uint32_t EMAC_TX1024TOMAX_GB;         /*!< no. of tx frames with length between 1024 and max */
     uint32_t EMAC_TXUCASTFRM_GB;          /*!< no. of tx unicast frames */
     uint32_t EMAC_TXMCASTFRM_GB;          /*!< no. of tx multicast frames */
     uint32_t EMAC_TXBCASTFRM_GB;          /*!< no. of tx broadcast frames */
     uint32_t EMAC_TXUNDR_ERR;             /*!< no. of tx frames with under runs  */
     uint32_t EMAC_TXSNGCOL_G;             /*!< no. of tx frames with single collision */
     uint32_t EMAC_TXMULTCOL_G;            /*!< no. of tx frames with multiple collisions */
     uint32_t EMAC_TXDEFERRED;             /*!< no. of deferred tx frames */
     uint32_t EMAC_TXLATECOL;              /*!< no. of tx frames with late collision */
     uint32_t EMAC_TXEXCESSCOL;            /*!< no. of tx frames with excessive collision */
     uint32_t EMAC_TXCARR_ERR;             /*!< no. of tx frames with carrier error */
     uint32_t EMAC_TXOCTCNT_G;             /*!< tx octet count - good frames only */
     uint32_t EMAC_TXFRMCNT_G;             /*!< tx frame count - good frames only */
     uint32_t EMAC_TXEXCESSDEF;            /*!< no. of tx frames aborted due to excess collision */
     uint32_t EMAC_TXPAUSEFRM;             /*!< no. of good pause frames */
     uint32_t EMAC_TXVLANFRM_G;            /*!< no. of tx virtual lan frames */

#if defined (GEMAC_SUPPORT_STAT_TX_OVERSIZE_G)
     uint32_t EMAC_TXOVRSIZE_G;
#else
     uint8_t  PAD_STAT_TX_OVERSIZE_G[4];
#endif

     uint8_t  PAD_5[4];                    /*!< pad 5 */

     uint32_t EMAC_RXFRMCNT_GB;            /*!< no. of rx frames */
     uint32_t EMAC_RXOCTCNT_GB;            /*!< no. of rx octets good and bad frames */
     uint32_t EMAC_RXOCTCNT_G;             /*!< no. of rx octets only good frames */
     uint32_t EMAC_RXBCASTFRM_G;           /*!< no. of rx good broadcast frames */
     uint32_t EMAC_RXMCASTFRM_G;           /*!< no. of rx good multicast frames */
     uint32_t EMAC_RXCRC_ERR;              /*!< no. of rx frames with crc errors */
     uint32_t EMAC_RXALIGN_ERR;            /*!< no. of rx frames with alignment errors */
     uint32_t EMAC_RXRUNT_ERR;             /*!< no. of rx frame with runt(<64bytes & crc) error */
     uint32_t EMAC_RXJAB_ERR;              /*!< no. of rx frames with jabber error */
     uint32_t EMAC_RXUSIZE_G;              /*!< no. of rx frames with length < 64 & no errors */
     uint32_t EMAC_RXOSIZE_G;              /*!< no. of rx frames with > 1518 bytes without errors */
     uint32_t EMAC_RX64_GB;                /*!< no. of rx frames with 64 byte length */
     uint32_t EMAC_RX65TO127_GB;           /*!< rx frames with length between 65 and 127 */
     uint32_t EMAC_RX128TO255_GB;          /*!< rx frames with length between 128 and 255 */
     uint32_t EMAC_RX256TO511_GB;          /*!< rx frames with length between 256 and 511 */
     uint32_t EMAC_RX512TO1023_GB;         /*!< rx frames with length between 512 and 1023 */
     uint32_t EMAC_RX1024TOMAX_GB;         /*!< rx frames with length between 1024 and max */
     uint32_t EMAC_RXUCASTFRM_G;           /*!< no. of rx unicast frames */
     uint32_t EMAC_RXLEN_ERR;              /*!< no. of rx frames with length errors */
     uint32_t EMAC_RXOORTYPE;              /*!< no. of rx frames with invalid frame size */
     uint32_t EMAC_RXPAUSEFRM;             /*!< no. of received pause frames */
     uint32_t EMAC_RXFIFO_OVF;             /*!< no. of missed received frames due to FIFO overflow*/
     uint32_t EMAC_RXVLANFRM_GB;           /*!< no. of good and bad vlan frames */
     uint32_t EMAC_RXWDOG_ERR;             /*!< no. of frames received with watchdog timeout */

#if defined (GEMAC_SUPPORT_STAT_REG_RX_RCV_ERR)
     uint32_t EMAC_RXRCV_ERR;
#else
     uint8_t PAD_STAT_REG_RX_RCV_ERR[4];
#endif

#if defined (GEMAC_SUPPORT_STAT_REG_RX_CTL_FRM_G)
     uint32_t EMAC_RXCTLFRM_G;
#else
     uint8_t PAD_STAT_REG_RX_CTL_FRM_G[4];
#endif

     uint8_t  PAD_6[24];                   /*!< pad 6 */

     uint32_t EMAC_IPC_RXIMSK;             /*!< IPC receive interrupt mask */
     uint8_t  PAD_7[4];                    /*!< pad 7 */
     uint32_t EMAC_IPC_RXINT;              /*!< IPC receive interrupt */
     uint8_t  PAD_8[4];                    /*!< pad 8 */
     uint32_t EMAC_RXIPV4_GD_FRM;          /*!< good ipv4 frames */
     uint32_t EMAC_RXIPV4_HDR_ERR_FRM;     /*!< ipv4 frames with header or version errors */
     uint32_t EMAC_RXIPV4_NOPAY_FRM;       /*!< ipv4 frames with no payload */
     uint32_t EMAC_RXIPV4_FRAG_FRM;        /*!< ipv4 frames with fragmentation */
     uint32_t EMAC_RXIPV4_UDSBL_FRM;       /*!< good ipv4 frames with disabled checksum for udp */
     uint32_t EMAC_RXIPV6_GD_FRM;          /*!< no. of ipv6 frames */
     uint32_t EMAC_RXIPV6_HDR_ERR_FRM;     /*!< ipv6 frames with header errors */
     uint32_t EMAC_RXIPV6_NOPAY_FRM;       /*!< ipv6 frames with no payload */
     uint32_t EMAC_RXUDP_GD_FRM;           /*!< ip datagrams with good udp payload */
     uint32_t EMAC_RXUDP_ERR_FRM;          /*!< good ip datagrams with udp checksum errors */
     uint32_t EMAC_RXTCP_GD_FRM;           /*!< ip datagrams with tcp payload */
     uint32_t EMAC_RXTCP_ERR_FRM;          /*!< ip datagrames with tcp payload checksum errors */
     uint32_t EMAC_RXICMP_GD_FRM;          /*!< ip datagrams with  icmp payload */
     uint32_t EMAC_RXICMP_ERR_FRM;         /*!< ip datagrams with icmp checksum errors */
     uint8_t  PAD_9[8];                    /*!< pad 9 */
     uint32_t EMAC_RXIPV4_GD_OCT;          /*!< ipv4 octets */
     uint32_t EMAC_RXIPV4_HDR_ERR_OCT;     /*!< ipv4 frames with header errors */
     uint32_t EMAC_RXIPV4_NOPAY_OCT;       /*!< ipv4 frames with no payload */
     uint32_t EMAC_RXIPV4_FRAG_OCT;        /*!< ipv4 frames with fragmentation */
     uint32_t EMAC_RXIPV4_UDSBL_OCT;       /*!< ipv4 udp segments with checksum disabled */
     uint32_t EMAC_RXIPV6_GD_OCT;          /*!< ipv6 good frames */
     uint32_t EMAC_RXIPV6_HDR_ERR_OCT;     /*!< ipv6 with header errors */
     uint32_t EMAC_RXIPV6_NOPAY_OCT;       /*!< ipv6 frames with no payload */
     uint32_t EMAC_RXUDP_GD_OCT;           /*!< good udp frames */
     uint32_t EMAC_RXUDP_ERR_OCT;          /*!< udp frames with errors */
     uint32_t EMAC_RXTCP_GD_OCT;           /*!< good tcp frames */
     uint32_t EMAC_RXTCP_ERR_OCT;          /*!< tcp frames with errors */
     uint32_t EMAC_RXICMP_GD_OCT;          /*!< good icmp frames */
     uint32_t EMAC_RXICMP_ERR_OCT;         /*!< icmp frames with errors */

#if defined (GEMAC_SUPPORT_LAYER3_LAYER4)
     uint8_t  PAD_10[376];                 /*!< pad 10 */
     uint32_t EMAC_L3L4_CTL;
     uint32_t EMAC_L4_ADDR;
     uint32_t PAD_L3L4[2];
     uint32_t EMAC_L3_ADDR0;
     uint32_t EMAC_L3_ADDR1;
     uint32_t EMAC_L3_ADDR2;
     uint32_t EMAC_L3_ADDR3;
     uint32_t EMAC_VLAN_INCL;
     uint32_t EMAC_VLAN_HSHTBL;
     uint8_t  PAD_11[728];                 /*!< pad 11 */
#else
     uint8_t  PAD_LAYER3_LAYER4[1144];     /*!< padding */
#endif
     uint32_t EMAC_TM_CTL;                 /*!< timestamp control register */
     uint32_t EMAC_TM_SUBSEC;              /*!< timestamp subsecond register */
     uint32_t EMAC_TM_SEC;                 /*!< timestamp second register */
     uint32_t EMAC_TM_NSEC;                /*!< timestamp nano-second register */
     uint32_t EMAC_TM_SECUPDT;             /*!< timestamp second update register */
     uint32_t EMAC_TM_NSECUPDT;            /*!< timestamp nano-second update register */
     uint32_t EMAC_TM_ADDEND;              /*!< timestamp addend register */
     uint32_t EMAC_TM_TGTM;                /*!< target time seconds register */
     uint32_t EMAC_TM_NTGTM;               /*!< target time nano-second register */
     uint32_t EMAC_TM_HISEC;               /*!< system time high register */
     uint32_t EMAC_TM_STMPSTAT;            /*!< timestamp status register */
     uint32_t EMAC_TM_PPSCTL;              /*!< timestamp pps control register */
     uint32_t EMAC_TM_AUXSTMP_NSEC;        /*!< auxiliary timestamp nanosecond register */
     uint32_t EMAC_TM_AUXSTMP_SEC;         /*!< auxiliary timestamp seconds register */

#if defined (GEMAC_SUPPORT_AV)
     uint32_t EMAC_MAC_AVCTL;
     uint8_t  PAD_16[36];
     uint32_t EMAC_TM_PPS0INTVL;
     uint32_t EMAC_TM_PPS0WIDTH;
     uint8_t  PAD_17[24];
     uint32_t EMAC_TM_PPS1TGTM;
     uint32_t EMAC_TM_PPS1NTGTM;
     uint32_t EMAC_TM_PPS1INTVL;
     uint32_t EMAC_TM_PPS1WIDTH;
     uint8_t  PAD_18[16];
     uint32_t EMAC_TM_PPS2TGTM;
     uint32_t EMAC_TM_PPS2NTGTM;
     uint32_t EMAC_TM_PPS2INTVL;
     uint32_t EMAC_TM_PPS2WIDTH;
     uint8_t  PAD_19[16];
     uint32_t EMAC_TM_PPS3TGTM;
     uint32_t EMAC_TM_PPS3NTGTM;
     uint32_t EMAC_TM_PPS3INTVL;
     uint32_t EMAC_TM_PPS3WIDTH;
     uint8_t  PAD_20[2096];
#else
     uint8_t  PAD__AV[2248];               /*!< padding */
#endif

     uint32_t EMAC_DMA_BUSMODE;            /*!< emac dma busmode register */
     uint32_t EMAC_DMA_TXPOLL;             /*!< emac dma tx poll register */
     uint32_t EMAC_DMA_RXPOLL;             /*!< emac dma rx poll register */
     uint32_t EMAC_DMA_RXDSC_ADDR;         /*!< emac dma rx descriptor address register */
     uint32_t EMAC_DMA_TXDSC_ADDR;         /*!< emac dma tx descriptor address register */
     uint32_t EMAC_DMA_STAT;               /*!< emac dma status register */
     uint32_t EMAC_DMA_OPMODE;             /*!< emac dma opmode register */
     uint32_t EMAC_DMA_IEN;                /*!< emac dma interrupt enable register */
     uint32_t EMAC_DMA_MISS_FRM;           /*!< emad dma missed frame register */
     uint32_t EMAC_DMA_RXIWDOG;            /*!< dma watchdog register */
     uint32_t EMAC_DMA_BMMODE;             /*!< dma busmode register */
     uint32_t EMAC_DMA_BMSTAT;             /*!< dma busmode status register */
     uint8_t  PAD22[24];                   /*!< pad 10 */
     uint32_t EMAC_DMA_TXDSC_CUR;          /*!< dma current tx descriptor register */
     uint32_t EMAC_DMA_RXDSC_CUR;          /*!< dma current rx descriptor register */
     uint32_t EMAC_DMA_TXBUF_CUR;          /*!< dma current tx buffer register */
     uint32_t EMAC_DMA_RXBUF_CUR;          /*!< dma current rx buffer register */

#if defined(GEMAC_SUPPORT_DMA1) && defined(GEMAC_SUPPORT_DMA2)
#if defined(GEMAC_SUPPORT_DMA1)
     uint8_t  PAD23[168];
     uint32_t EMAC_DMA1_BUSMODE;
     uint32_t EMAC_DMA1_TXPOLL;
     uint32_t EMAC_DMA1_RXPOLL;
     uint32_t EMAC_DMA1_RXDSC_ADDR;
     uint32_t EMAC_DMA1_TXDSC_ADDR;
     uint32_t EMAC_DMA1_STAT;
     uint32_t EMAC_DMA1_OPMODE;
     uint32_t EMAC_DMA1_IEN;
     uint32_t EMAC_DMA1_MISS_FRM;
     uint32_t EMAC_DMA1_RXIWDOG;
     uint8_t  PAD24[8];
     uint32_t EMAC_DMA1_CHSFCS;
     uint8_t  PAD25[20];
     uint32_t EMAC_DMA1_TXDSC_CUR;
     uint32_t EMAC_DMA1_RXDSC_CUR;
     uint32_t EMAC_DMA1_TXBUF_CUR;
     uint32_t EMAC_DMA1_RXBUF_CUR;
     uint8_t  PAD26[8];
     uint32_t EMAC_DMA1_CHCBSCTL;
     uint32_t EMAC_DMA1_CHCBSSTAT;
     uint32_t EMAC_DMA1_CHISC;
     uint32_t EMAC_DMA1_CHSSC;
     uint32_t EMAC_DMA1_CHHIC;
     uint32_t EMAC_DMA1_CHLOC;
#endif
#if defined(GEMAC_SUPPORT_DMA2)
     uint8_t  PAD27[136];
     uint32_t EMAC_DMA2_BUSMODE;
     uint32_t EMAC_DMA2_TXPOLL;
     uint32_t EMAC_DMA2_RXPOLL;
     uint32_t EMAC_DMA2_RXDSC_ADDR;
     uint32_t EMAC_DMA2_TXDSC_ADDR;
     uint32_t EMAC_DMA2_STAT;
     uint32_t EMAC_DMA2_OPMODE;
     uint32_t EMAC_DMA2_IEN;
     uint32_t EMAC_DMA2_MISS_FRM;
     uint32_t EMAC_DMA2_RXIWDOG;
     uint8_t  PAD28[8];
     uint32_t EMAC_DMA2_CHSFCS;
     uint8_t  PAD29[20];
     uint32_t EMAC_DMA2_TXDSC_CUR;
     uint32_t EMAC_DMA2_RXDSC_CUR;
     uint32_t EMAC_DMA2_TXBUF_CUR;
     uint32_t EMAC_DMA2_RXBUF_CUR;
     uint8_t  PAD30[8];
     uint32_t EMAC_DMA2_CHCBSCTL;
     uint32_t EMAC_DMA2_CHCBSSTAT;
     uint32_t EMAC_DMA2_CHISC;
     uint32_t EMAC_DMA2_CHSSC;
     uint32_t EMAC_DMA2_CHHIC;
     uint32_t EMAC_DMA2_CHLOC;
#endif
#endif

} ADI_EMAC_REGISTERS;

/*! EMAC DMA Descriptor */
typedef struct ADI_EMAC_DMADESC
{
    uint32_t                   Status;         /*!< descriptor status       */
    uint32_t                   ControlDesc;    /*!< descriptor control bits */
    uint32_t                   StartAddr;      /*!< dma start address       */
    struct ADI_EMAC_DMADESC   *pNextDesc;      /*!< next descriptor pointer */
    uint32_t                   ExtendedStatus; /*!< extended status         */
    uint32_t                   Reserved;       /*!< reserved                */
    uint32_t                   TimeStampLo;  /*!< ieee-1588 timestamp low */
    uint32_t                   TimeStampHi;  /*!< ieee-1588 timestamp high */

} ADI_EMAC_DMADESC;

/*! Dma descriptor size */
#define ADI_EMAC_DMADESC_SIZE  (sizeof(ADI_EMAC_DMADESC))

/*! EMAC buffer information */
typedef struct ADI_EMAC_BUFINFO
{
   ADI_EMAC_DMADESC           *pDmaDesc;   /*!< pointer to DMA descriptor */

} ADI_EMAC_BUFINFO;


/*! Frame queue */
typedef struct ADI_EMAC_FRAME_Q
{
    ADI_ETHER_BUFFER  *pQueueHead;        /*!< frame queue head */
    ADI_ETHER_BUFFER  *pQueueTail;        /*!< frame queue tail */
    int32_t            ElementCount;      /*!< number of buffers in a queue */

} ADI_EMAC_FRAME_Q;

/* DMA Channel */
typedef struct ADI_EMAC_DMA_CHANNEL
{
	ADI_EMAC_FRAME_Q   Active;           /*!< active dma queue */
	ADI_EMAC_FRAME_Q   Queued;           /*!< queued dma queue - not bound with descriptor  */
	ADI_EMAC_FRAME_Q   Pending;          /*!< pending queue - buffers bound with descriptor */
	ADI_EMAC_FRAME_Q   Completed;        /*!< completed buffer queue */

	ADI_EMAC_DMADESC  *pDmaDescHead;     /*!< free dma descriptor head */
	ADI_EMAC_DMADESC  *pDmaDescTail;     /*!< free dma descriptor tail */

	int32_t            NumAvailDmaDesc;  /*!< number of available dma descriptors */
} ADI_EMAC_DMA_CHANNEL;

/*! Emac channel */
typedef struct ADI_EMAC_CHANNEL
{
	ADI_EMAC_DMA_CHANNEL 	DMAChan[GEMAC_SUPPORT_NUM_DMA_DEVICES];       /*!< DMA Channels */

	void*                   pDMADescMem;      /*!< Pointer to DMA Descriptor Memory (should be 32-byte aligned) */
	uint32_t                nDMADescNum;      /*!< Number of descriptors that is available in the memory pointer by the DMA Desc Memory */
	bool              		Recv;             /*!< receive channel identifier */

} ADI_EMAC_CHANNEL;

/*! Enumeration of EMAC capabilities */
typedef enum ADI_EMAC_CAPABILITY {
	ADI_EMAC_CAPABILITY_LPI         = (0x00000001 << 0u),
	ADI_EMAC_CAPABILITY_RGMII       = (0x00000001 << 1u),
	ADI_EMAC_CAPABILITY_RMII        = (0x00000001 << 2u),
	ADI_EMAC_CAPABILITY_PTP         = (0x00000001 << 3u),
	ADI_EMAC_CAPABILITY_AV          = (0x00000001 << 4u),
	ADI_EMAC_CAPABILITY_PPS         = (0x00000001 << 5u),
	ADI_EMAC_CAPABILITY_ALARM       = (0x00000001 << 6u),

	ADI_EMAC_CAPABILITY_AV_DMA1     = (0x00000001 << 10u),
	ADI_EMAC_CAPABILITY_AV_DMA2     = (0x00000001 << 11u)
}ADI_EMAC_CAPABILITY;

/*! Enumeration for GEMAC PHY configuration */
typedef enum ADI_GEMAC_PHY_CFG {
    ADI_GEMAC_PHY_CFG_AUTO_NEGOTIATE_EN   = (1U << 0),   /*!< Enable Auto Negotiation */
    ADI_GEMAC_PHY_CFG_FULL_DUPLEX_EN      = (1U << 1),   /*!< Enable Full-Duplex (Ignored if Auto Negotiation is Enabled) */
    ADI_GEMAC_PHY_CFG_10Mbps_EN           = (1U << 2),   /*!< Enable 10 Mbps Speed (Ignored if Auto Negotiation is Enabled) */
    ADI_GEMAC_PHY_CFG_100Mbps_EN          = (1U << 3),   /*!< Enable 100 Mbps Speed (Ignored if Auto Negotiation is Enabled) */
    ADI_GEMAC_PHY_CFG_1000Mbps_EN         = (1U << 4),   /*!< Enable 1000 Mbps Speed (Ignored if Auto Negotiation is Enabled) */
    ADI_GEMAC_PHY_CFG_LOOPBACK_EN         = (1U << 5)    /*!< Enable Loopback */
} ADI_GEMAC_PHY_CFG;

#ifdef ADI_ETHER_SUPPORT_PPS
/* Enum for the various PPS commands */
typedef enum __PPS_CMD {
	PPS_CMD_NO_CMD = 0,
	PPS_CMD_START_SINGLE_PULSE,
	PPS_CMD_START_PULSE_TRAIN,
	PPS_CMD_CANCEL_START,
	PPS_CMD_STOP_PULSE_AT_TIME,
	PPS_CMD_STOP_TRAIN_IMMEDIATELY,
	PPS_CMD_CANCEL_STOP_PULSE_TRAIN
} PPS_CMD;

/* Enum for the PPS trigger modes */
typedef enum __PPS_TRIG_MODE {
	PPS_TRIG_MODE_INTERRUPT_ONLY  = 0u,
	PPS_TRIG_MODE_INTERRUPT_PULSE = 2u,
	PPS_TRIG_MODE_PULSE_ONLY      = 3u
} PPS_TRIG_MODE;
#endif /* ADI_ETHER_SUPPORT_PPS */

#ifdef ADI_ETHER_SUPPORT_PTP
typedef struct ADI_EMAC_PTP_DEVICE
{
	bool 					bEnabled;
	uint32_t    			nInputClkFreq;
	uint32_t 				PTPClkFreq;
	uint32_t    			nRxFilterFlag;
	ADI_ETHER_TIME 			nInitTime;
	ADI_ETHER_CALLBACK_FN 	pfCallback;
}ADI_EMAC_PTP_DEVICE;
#endif /* ADI_ETHER_SUPPORT_PTP */

#if defined(ADI_ETHER_SUPPORT_ALARM) && defined(ADI_ETHER_SUPPORT_PPS)
typedef struct ADI_EMAC_PPS_ALARM_DEVICE
{

	bool                       bAlarm;
	bool 					   bConfigured;
	bool 					   bEnabled;
	bool                       bTriggerPending;
    ADI_ETHER_GEMAC_PPS_PULSE_MODE ePulseMode;
	ADI_ETHER_TIME 			   StartTime;
	ADI_ETHER_TIME 		       EndTime;
	ADI_ETHER_CALLBACK_FN      pfCallback;
	PPS_TRIG_MODE			   eTriggerMode;
	uint32_t                   nInterval;
	uint32_t                   nWidth;

} ADI_EMAC_PPS_ALARM_DEVICE;
#endif

#ifdef ADI_ETHER_SUPPORT_AV
typedef enum ADI_EMAC_AV_CONFIG {
	ADI_EMAC_AV_CONFIG_AV_EN         = (1u << 0u),

	ADI_EMAC_AV_CONFIG_DMA1_EN       = (1u << 1u),
	ADI_EMAC_AV_CONFIG_DMA1_TX_EN    = (1u << 2u),
	ADI_EMAC_AV_CONFIG_DMA1_RX_EN    = (1u << 3u),
	ADI_EMAC_AV_CONFIG_DMA1_CBS_EN   = (1u << 4u),
	ADI_EMAC_AV_CONFIG_DMA1_SLOT_EN  = (1u << 5u),

	ADI_EMAC_AV_CONFIG_DMA2_EN       = (1u << 6u),
	ADI_EMAC_AV_CONFIG_DMA2_TX_EN    = (1u << 7u),
	ADI_EMAC_AV_CONFIG_DMA2_RX_EN    = (1u << 8u),
	ADI_EMAC_AV_CONFIG_DMA2_CBS_EN   = (1u << 9u),
	ADI_EMAC_AV_CONFIG_DMA2_SLOT_EN  = (1u << 10u)

} ADI_EMAC_AV_CONFIG;

typedef struct _ADI_EMAC_AV_DMA_CHANNEL {
	uint32_t   NumReservedDesc;

} ADI_EMAC_AV_DMA_CHANNEL;

typedef struct _ADI_EMAC_AV_CHANNEL
{
	uint32_t 	nSlotCtlStatReg;
	uint32_t 	nCBSCtlReg;
	uint32_t 	nIdleSlopeReg;
	uint32_t 	nSendSlopeReg;
	uint32_t 	nHiCreditReg;
	uint32_t 	nLowCreditReg;

	ADI_EMAC_AV_DMA_CHANNEL  Tx;
	ADI_EMAC_AV_DMA_CHANNEL  Rx;
} ADI_EMAC_AV_CHANNEL;

typedef struct ADI_EMAC_AV_DEVICE
{
	uint32_t 			config;
	uint32_t            nAVMACCtlReg;
	ADI_EMAC_AV_CHANNEL Chan1;
	ADI_EMAC_AV_CHANNEL Chan2;
} ADI_EMAC_AV_DEVICE;
#endif

/*! EMAC device   */
typedef struct ADI_EMAC_DEVICE
{
	uint32_t            nNumDmaChannels; /*!< Number of DMA Channels */
	uint32_t            TxIntPeriod;
	uint32_t            Capability;    /*!< Ored value of all the EMAC capabilities */
    ADI_EMAC_REGISTERS *pEMAC_REGS;   /*!< pointer to the EMAC registers */
    ADI_PHY_DEVICE     *pPhyDevice;
    uint32_t            Version;       /*!< EMAC version information      */
    uint32_t            Interrupt;     /*!< EMAC interrupt                */
    uint8_t             MacAddress[6]; /*!< MAC address                   */
    bool                Started;       /*!< driver is started or not      */
    bool                Opened;        /*!< driver is opened or not       */
    uint32_t            MdcClk;        /*!< MDC clock to the PHY          */

    /* PHY configuration */
    uint32_t            nPhyConfig;    /*!< PHY configuration */
    bool                Cache;         /*!< data cache on                 */
    uint32_t            MDCClockRange; /*!< MDC clock range               */

#if defined(ADI_ETHER_SUPPORT_PPS) && defined(ADI_ETHER_SUPPORT_ALARM)
    /* Module Configurations */
    ADI_EMAC_PTP_DEVICE 		PTPDevice;
    ADI_EMAC_PPS_ALARM_DEVICE   PPS_Alarm_Devices[NUM_PPS_ALARM_DEVICES];
#endif
#ifdef ADI_ETHER_SUPPORT_AV
    ADI_EMAC_AV_DEVICE          AVDevice;
#endif


    /* Channels */
    ADI_EMAC_CHANNEL    Rx;            /*!< receive channel              */
    ADI_EMAC_CHANNEL    Tx;            /*!< transmit channel             */
    int32_t MulticastBinCount[ADI_EMAC_NUM_MCAST_BINS]; /*!< 64 bin hash entires */
    ADI_ETHER_CALLBACK_FN  pEtherCallback;  /*!< driver callback         */

#ifdef ADI_ETHER_STATISTICS
    ADI_EMAC_STATS     Stats;         /*!< statistics collection */
#endif

    void *pUsrPtr;

} ADI_EMAC_DEVICE;

/*! EMAC Dma Descriptor status */
typedef enum ADI_EMAC_DMADESC_STATUS
{
    ENUM_DS_DAFILT_FAIL = 0x40000000,
    ENUM_DS_FRMLEN_MASK = 0x3FFF0000,
    ENUM_DS_DESC_ERR    = 0x00008000,
    ENUM_DS_RX_TRUNCATED= 0x00004000,
    ENUM_DS_SAFILT_FAIL = 0x00002000,
    ENUM_DS_RXLEN_ERR   = 0x00001000,
    ENUM_DS_RXOVF_ERR   = 0x00000800,
    ENUM_DS_RXVLAN_TAG  = 0x00000400,
    ENUM_DS_RXFIRST_DESC= 0x00000200,
    ENUM_DS_RXLAST_DESC = 0x00000100,
    ENUM_DS_RXLONG_FRAME= 0x00000080,
    ENUM_DS_RXETH_FRAME = 0x00000020,
    ENUM_DS_RXWATCHDOG  = 0x00000010,
    ENUM_DS_RXMII_ERR   = 0x00000008,
    ENUM_DS_RXDRIBBLE   = 0x00000004,
    ENUM_DS_RXCRC_ERR   = 0x00000002,
    ENUM_DS_RXEXT_STAT  = 0x00000001,

    ENUM_DS_TXINT_ENA   = 0x40000000,
    ENUM_DS_TXLAST_DESC = 0x20000000,
    ENUM_DS_TXFIRST_DESC= 0x10000000,
    ENUM_DS_TXDIS_CRC   = 0x08000000,
    ENUM_DS_TXDIS_PAD   = 0x04000000,
    ENUM_DS_TXCSUM_MASK = 0x00C00000,
    ENUM_DS_TXCSUM_BYPASS = 0x00000000,
    ENUM_DS_TXCSUM_TCP  = 0x00800000,
    ENUM_DS_TXCSUM_IPV4   = 0x00400000,
    ENUM_DS_TXCSUM_PESUDO = 0x00C00000,
    ENUM_DS_TXENDOF_RING  = 0x00200000,
    ENUM_DS_TXDESC_CHAIN  = 0x00100000,
    ENUM_DS_TXIPHDR_ERR   = 0x00010000,
    ENUM_DS_TXJAB_TIMEOUT = 0x00004000,
    ENUM_DS_TXFRAME_FLUSH = 0x00002000,
    ENUM_DS_TXPAYLOAD_ERR = 0x00001000,
    ENUM_DS_TXLOST_CARRIER= 0x00000800,
    ENUM_DS_TXNO_CARRIER  = 0x00000400,
    ENUM_DS_TXLATE_COLL   = 0x00000200,
    ENUM_DS_TXEXS_COLL    = 0x00000100,
    ENUM_DS_TXVLAN_FRAME  = 0x00000080,
    ENUM_DS_TXCOLL_MASK   = 0x00000078,
    ENUM_DS_TXEXC_DEF     = 0x00000004,
    ENUM_DS_TX_UNDFLOW    = 0x00000002,
    ENUM_DS_TX_DEF        = 0x00000001

} ADI_EMAC_DMADESC_STATUS;

void pps_set_trigger_mode (
		ADI_EMAC_DEVICE* const pDev,
		uint32_t nDeviceID,
		uint32_t TriggerMode
    );


#endif /* _ADI_GEMAC_INT_H_ */
