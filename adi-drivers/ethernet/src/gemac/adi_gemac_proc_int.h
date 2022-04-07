/*!
*********************************************************************************
 *
 * @file:    adi_gemac_proc_int.h
 *
 * @brief:   Ethernet GEMAC driver internal processor header file
 *
 * @version: $Revision: 62469 $
 *
 * @date:    $Date: 2019-08-26 13:37:40 -0400 (Mon, 26 Aug 2019) $
 * ------------------------------------------------------------------------------
 *
 * Copyright (c) 2015-2019 Analog Devices, Inc.
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
#ifndef _ADI_GEMAC_PROC_H_
#define _ADI_GEMAC_PROC_H_

/* Note: Some Configurations are part adi_ether.h */

/* Processor Specifin Configuration */
#include <sys/platform.h>
#include <drivers/ethernet/adi_ether.h>
#include <drivers/ethernet/adi_ether_misra.h>


/*========== S U P P O R T E D    P H Y s ==========*/
#define PHY_DEV_DP8386X      (83860)
#define PHY_DEV_DP83848      (83848)


/*========== S U P P O R T    C O N F I G U R A T I O N ==========*/
#if defined (__ADSP215xx__)
#   define GEMAC_SUPPORT_EMAC0

#   define GEMAC_SUPPORT_RGMII
#   define GEMAC_SUPPORT_LPI
#   define GEMAC_SUPPORT_DMA0
#   define GEMAC_SUPPORT_DMA1
#   define GEMAC_SUPPORT_DMA2
#   define GEMAC_SUPPORT_WDOG
#   define GEMAC_SUPPORT_MAC_ADDR1
#   define GEMAC_SUPPORT_LAYER3_LAYER4

#   define GEMAC_SUPPORT_STAT_REG_RX_RCV_ERR
#   define GEMAC_SUPPORT_STAT_REG_RX_CTL_FRM_G
#   define GEMAC_SUPPORT_STAT_REG_TX_OVERSIZE_G

#   define GEMAC_SUPPORT_NUM_DMA_DEVICES 	(3)

#if defined(__ADSPSC587__) || defined(__ADSPSC589__)
#   define GEMAC_SUPPORT_EMAC1
#endif




#elif defined (__ADSPBF6xx__) || defined(ADI_ADSP_CM40Z)

#   define GEMAC_SUPPORT_RMII
#   define GEMAC_SUPPORT_DMA0
#   define GEMAC_SUPPORT_REG_VERSION

#   define GEMAC_SUPPORT_EMAC0

#   define GEMAC_SUPPORT_NUM_DMA_DEVICES 	(1)

#   if defined (__ADSPBF6xx__)
#       define GEMAC_SUPPORT_EMAC1
#   endif



#else
#   error "Processor not Supported"




#endif

#if defined(GEMAC_SUPPORT_EMAC0) && defined(GEMAC_SUPPORT_EMAC1)
#define EMAC_NUM_DEV   (2)
#elif defined(GEMAC_SUPPORT_EMAC0)
#define EMAC_NUM_DEV   (1)
#else
#error "Configuration Error"
#endif




/*========== C A P A B I L I T Y    C O N F I G U R A T I O N ==========*/
#if defined(__ADSP215xx__)
#   define GEMAC0_CAPABILITY    (  ADI_EMAC_CAPABILITY_LPI       \
                                 | ADI_EMAC_CAPABILITY_RGMII     \
                                 | ADI_EMAC_CAPABILITY_PTP       \
                                 | ADI_EMAC_CAPABILITY_AV        \
                                 | ADI_EMAC_CAPABILITY_PPS       \
                                 | ADI_EMAC_CAPABILITY_ALARM     \
                                 | ADI_EMAC_CAPABILITY_AV_DMA1   \
                                 | ADI_EMAC_CAPABILITY_AV_DMA2   \
                                )
#   define GEMAC0_NUM_DMA_DEVICES		(3)

#if defined(__ADSPSC587__) || defined(__ADSPSC589__)
#   define GEMAC1_CAPABILITY    (ADI_EMAC_CAPABILITY_RMII)
#   define GEMAC1_NUM_DMA_DEVICES		(1)
#endif


#   define NUM_PPS_ALARM_DEVICES  (4)


#elif defined (__ADSPBF6xx__) || defined(ADI_ADSP_CM40Z)

#   define GEMAC0_CAPABILITY    (ADI_EMAC_CAPABILITY_RMII)
#   define GEMAC1_CAPABILITY    (ADI_EMAC_CAPABILITY_RMII)

#   define GEMAC0_NUM_DMA_DEVICES		(1)
#   define GEMAC1_NUM_DMA_DEVICES		(1)

#else
#   error "Processor not Supported"
#endif

#ifdef GEMAC_SUPPORT_EMAC0
#define ADI_EMAC0_BASE_ADDRESS (REG_EMAC0_MACCFG)
#endif /* GEMAC_SUPPORT_EMAC0 */

#ifdef GEMAC_SUPPORT_EMAC1
#define ADI_EMAC1_BASE_ADDRESS (REG_EMAC1_MACCFG)
#endif /* GEMAC_SUPPORT_EMAC1 */



/*========== O T H E R    C O N F I G U R A T I O N ==========*/
#if defined(__ADSP215xx__)

#   define GEMAC_MDC_CLK        (0x4)
#   define GEMAC_VERSION        (0)
    /*! EMAC DMA thershold above which descriptors get added to live active dma queue */
#   define ADI_EMAC_THRESHOLD         (10)


#   define GEMAC_REG_CFG_BMMODE     (  (0x3u << BITP_EMAC_DMA0_BMMODE_WROSRLMT)  \
                                     | (0x3u << BITP_EMAC_DMA0_BMMODE_RDOSRLMT)  \
                                     | (0x1u << BITP_EMAC_DMA0_BMMODE_UNDEF))
                                     
                                     
#   define GEMAC_REG_CFG_BUSMODE  	(  (1UL  << BITP_EMAC_DMA_BUSMODE_ATDS) \
                                     | (1UL  << BITP_EMAC_DMA_BUSMODE_PBL8) \
                                     | (32UL << BITP_EMAC_DMA_BUSMODE_PBL))
									 
#	define EMAC_REG_CFG_BUSMODE		(  (1UL  << BITP_EMAC_DMA_BUSMODE_ATDS) \
                                     | (1UL  << BITP_EMAC_DMA_BUSMODE_PBL8) \
                                     | (8UL << BITP_EMAC_DMA_BUSMODE_PBL))
                                     
                                     
#   define GEMAC_REG_CFG_OPMODE     (                              \
                                      BITM_EMAC_DMA_OPMODE_FEF     \
                                    | BITM_EMAC_DMA_OPMODE_FUF     \
    	                            | ENUM_EMAC_DMA_OPMODE_TTC_64  \
    	                            | ENUM_EMAC_DMA_OPMODE_RTC_64  \
                                    | BITM_EMAC_DMA0_OPMODE_OSF   )                                
                                    
/*! Abnormal and Normal Interrupts */
#   define ADI_EMAC_AIS_NIS_INTERRUPTS   ((1UL << BITP_EMAC_DMA0_IEN_NIE)  |  \
                                         (1UL << BITP_EMAC_DMA0_IEN_AIE)   |  \
                                         (1UL << BITP_EMAC_DMA0_IEN_RUE)   |  \
                                         (1UL << BITP_EMAC_DMA0_IEN_RIE)   |  \
                                         (1UL << BITP_EMAC_DMA0_IEN_UNE)   |  \
                                         (1UL << BITP_EMAC_DMA0_IEN_TUE)   |  \
                                         (1UL << BITP_EMAC_DMA0_IEN_TIE))
                                    
                                    
#elif defined (__ADSPBF6xx__) || defined(ADI_ADSP_CM40Z)

#   define GEMAC_VERSION        (0)

    /*! EMAC DMA thershold above which descriptors get added to live active dma queue */                                    
#   define ADI_EMAC_THRESHOLD         (3)                                    

#   ifdef __ADSPBF6xx__
#       define GEMAC_MDC_CLK        (0x0)
#   else
#       define GEMAC_VERSION        (0)
#       define GEMAC_MDC_CLK        (0x1)
#   endif


#   define GEMAC_REG_CFG_BUSMODE 	(1UL  << BITP_EMAC_DMA_BUSMODE_ATDS)
#   define EMAC_REG_CFG_BUSMODE 	(1UL  << BITP_EMAC_DMA_BUSMODE_ATDS)

#   define GEMAC_REG_CFG_OPMODE     (                                \
    	                              BITM_EMAC_DMA_OPMODE_FEF     \
    	                            | BITM_EMAC_DMA_OPMODE_FUF     \
    	                            | ENUM_EMAC_DMA_OPMODE_TTC_64  \
    	                            | ENUM_EMAC_DMA_OPMODE_RTC_64 )

/*! Abnormal and Normal Interrupts */
#   define ADI_EMAC_AIS_NIS_INTERRUPTS   ((1UL << BITP_EMAC_DMA_IEN_NIS)|  \
                                         (1UL << BITP_EMAC_DMA_IEN_AIS) |  \
                                         (1UL << BITP_EMAC_DMA_IEN_RU)  |  \
                                         (1UL << BITP_EMAC_DMA_IEN_RI)  |  \
                                         (1UL << BITP_EMAC_DMA_IEN_UNF) |  \
                                         (1UL << BITP_EMAC_DMA_IEN_TU)  |  \
                                         (1UL << BITP_EMAC_DMA_IEN_TI))
                                    
#else
#   error "Processor not Supported"
#endif














/*========== P H Y    C O N F I G U R A T I O N ==========*/
#if defined(__SAM_V1__)
#define EMAC0_PHY_NUM_DEV      (1)
#define EMAC0_PHY_DEV          (0)

#define EMAC0_PHY0_DEV         (PHY_DEV_DP8386X)
#define EMAC0_PHY0_ADDR        (0x1)

#define EMAC1_PHY_NUM_DEV      (0)
#define EMAC1_PHY_DEV          (0)

#elif defined(__ADSPSC589_FAMILY__) 
#define EMAC0_PHY_NUM_DEV      (1)
#define EMAC0_PHY_DEV          (0)

#define EMAC0_PHY0_DEV         (PHY_DEV_DP8386X)
#define EMAC0_PHY0_ADDR        (0x1)

#if defined(__ADSPSC587__) || defined(__ADSPSC589__)
#define EMAC1_PHY_NUM_DEV      (1)
#define EMAC1_PHY_DEV          (0)

#define EMAC1_PHY0_DEV         (PHY_DEV_DP83848)
#define EMAC1_PHY0_ADDR        (0x1)
#endif

#elif defined(__ADSPSC573_FAMILY__)

#define EMAC0_PHY_NUM_DEV      (1)
#define EMAC0_PHY_DEV          (0)

#define EMAC0_PHY0_DEV         (PHY_DEV_DP8386X)
#define EMAC0_PHY0_ADDR        (0x1)


#elif defined(__ADSPBF6xx__)

#define EMAC0_PHY_NUM_DEV      (1)
#define EMAC0_PHY_DEV          (0)

#define EMAC0_PHY0_DEV         (PHY_DEV_DP83848)
#define EMAC0_PHY0_ADDR        (0x1)

#define EMAC1_PHY_NUM_DEV      (0)

#elif defined(ADI_ADSP_CM40Z)

#define EMAC0_PHY_NUM_DEV      (1)
#define EMAC0_PHY_DEV          (0)

#define EMAC0_PHY0_DEV         (PHY_DEV_DP83848)
#define EMAC0_PHY0_ADDR        (0x1)

#else
#   error "PHY Cfg Error"
#endif




/*========== I NC L U D E S ==========*/
#include <adi_types.h>
#if defined(__ADSP215xx__)
#include <runtime/cache/adi_cache.h>
#endif /* defined(__ADSP215xx__) */

/*========== S U P P O R T    M A P ==========*/
#ifdef ADI_ETHER_SUPPORT_AV
#   define GEMAC_SUPPORT_AV
#endif
#ifdef ADI_ETHER_SUPPORT_PTP
#   define GEMAC_SUPPORT_PTP
#endif
#ifdef ADI_ETHER_SUPPORT_PPS
#   define GEMAC_SUPPORT_PPS
#endif
#ifdef ADI_ETHER_SUPPORT_ALARM
#   define GEMAC_SUPPORT_ALARM
#endif




/*========== M A C R O    T R A N S L A T I O N ==========*/
#if defined(__ADSP215xx__)
#define BITM_EMAC_DMA_STAT_TU               BITM_EMAC_DMA0_STAT_TU
#define BITM_EMAC_DMA_STAT_RU               BITM_EMAC_DMA0_STAT_RU
#define BITM_EMAC_DMA_STAT_TS               BITM_EMAC_DMA0_STAT_TS
#define ENUM_EMAC_DMA_STAT_TS_SUSPENDED     ENUM_EMAC_DMA0_STAT_TS_SUSPENDED
#define ENUM_EMAC_DMA_STAT_TS_STOPPED       ENUM_EMAC_DMA0_STAT_TS_STOPPED
#define BITM_EMAC_DMA_OPMODE_ST             BITM_EMAC_DMA0_OPMODE_ST
#define BITM_EMAC_DMA_OPMODE_SR             BITM_EMAC_DMA0_OPMODE_SR
#define ENUM_EMAC_DMA_STAT_RS_SUSPENDED     ENUM_EMAC_DMA0_STAT_RS_SUSPENDED
#define ENUM_EMAC_DMA_STAT_RS_STOPPED       ENUM_EMAC_DMA0_STAT_RS_STOPPED
#define BITM_EMAC_DMA_STAT_TPS              BITM_EMAC_DMA0_STAT_TPS
#define BITM_EMAC_DMA_STAT_TJT              BITM_EMAC_DMA0_STAT_TJT
#define BITM_EMAC_DMA_STAT_RPS              BITM_EMAC_DMA0_STAT_RPS
#define BITM_EMAC_DMA_STAT_UNF              BITM_EMAC_DMA0_STAT_UNF
#define BITM_EMAC_DMA_STAT_RWT              BITM_EMAC_DMA0_STAT_RWT
#define BITM_EMAC_DMA_STAT_ETI              BITM_EMAC_DMA0_STAT_ETI
#define BITM_EMAC_DMA_STAT_ERI              BITM_EMAC_DMA0_STAT_ERI
#define BITM_EMAC_DMA_STAT_FBI              BITM_EMAC_DMA0_STAT_FBI
#define BITM_EMAC_DMA_STAT_MCI              BITM_EMAC_DMA0_STAT_MCI
#define BITM_EMAC_DMA_STAT_RI               BITM_EMAC_DMA0_STAT_RI
#define BITM_EMAC_DMA_STAT_TI               BITM_EMAC_DMA0_STAT_TI
#define BITM_EMAC_DMA_STAT_AIS              BITM_EMAC_DMA0_STAT_AIS
#define BITM_EMAC_DMA_STAT_TTI              BITM_EMAC_DMA0_STAT_TTI
#define BITM_EMAC_DMA_BUSMODE_PBL8          BITM_EMAC_DMA0_BUSMODE_PBL8
#define BITM_EMAC_DMA_BUSMODE_SWR           BITM_EMAC_DMA0_BUSMODE_SWR
#define BITM_EMAC_DMA_STAT_RS               BITM_EMAC_DMA0_STAT_RS
#define BITP_EMAC_DMA_BUSMODE_ATDS          BITP_EMAC_DMA0_BUSMODE_ATDS
#define BITP_EMAC_DMA_BUSMODE_PBL8          BITP_EMAC_DMA0_BUSMODE_PBL8
#define BITP_EMAC_DMA_BUSMODE_PBL           BITP_EMAC_DMA0_BUSMODE_PBL
#define BITP_EMAC_DMA_BUSMODE_FB            BITP_EMAC_DMA0_BUSMODE_FB
#define BITP_EMAC_DMA_IEN_NIS               BITP_EMAC_DMA0_IEN_NIE
#define BITP_EMAC_DMA_IEN_AIS               BITP_EMAC_DMA0_IEN_AIE
#define BITP_EMAC_DMA_IEN_RU                BITP_EMAC_DMA0_IEN_RUE
#define BITP_EMAC_DMA_IEN_RI                BITP_EMAC_DMA0_IEN_RIE
#define BITP_EMAC_DMA_IEN_UNF               BITP_EMAC_DMA0_IEN_UNE
#define BITP_EMAC_DMA_IEN_TU                BITP_EMAC_DMA0_IEN_TUE
#define BITP_EMAC_DMA_IEN_TI                BITP_EMAC_DMA0_IEN_TIE
#define BITM_EMAC_DMA_OPMODE_FEF            BITM_EMAC_DMA0_OPMODE_FEF
#define BITM_EMAC_DMA_OPMODE_FUF            BITM_EMAC_DMA0_OPMODE_FUF
#define BITM_EMAC_DMA_OPMODE_TSF            BITM_EMAC_DMA0_OPMODE_TSF
#define ENUM_EMAC_DMA_OPMODE_TTC_64         ENUM_EMAC_DMA0_OPMODE_TTC_64
#define ENUM_EMAC_DMA_OPMODE_TTC_256        ENUM_EMAC_DMA0_OPMODE_TTC_256
#define ENUM_EMAC_DMA_OPMODE_RTC_64         ENUM_EMAC_DMA0_OPMODE_RTC_64
#define ENUM_EMAC_DMA_OPMODE_RTC_128        ENUM_EMAC_DMA0_OPMODE_RTC_128
#endif



/*========== M A C R O S ==========*/
#if defined (__ADSPARM__)
#   define VAR_UNUSED_DECR(X)  X __attribute__((unused))
#   define VAR_UNUSED(X)
#   define FUN_UNUSED  __attribute__((unused))
#   define ALWAYS_INLINE  extern inline
#elif defined(ADI_ADSP_CM40Z)
#   define VAR_UNUSED_DECR(X)  X
#   define VAR_UNUSED(X)   (void)X;
#   define FUN_UNUSED
#   define ALWAYS_INLINE  extern inline
#else
#   define VAR_UNUSED_DECR(X)  (X)
#   define VAR_UNUSED(X)
#   define FUN_UNUSED
#   define ALWAYS_INLINE  _Pragma("inline")
#endif



/*========== C O D E ==========*/

/********* Cache Code *********/
#ifdef __ADSPBLACKFIN__
/*!
 * If the cached data line is dirty then this macro writes the line out
 * and marks the cache line a clean. The macro increments the supplied
 * address by cache line size.
 */
#define FLUSH(P)                                   \
    do {                                           \
        asm volatile("FLUSH[%0++];" : "+p"(P));    \
    } while (0)

/*!
 * This macro flushes and invalidates the cache line. So the cache line
 * will be loaded again from next level of memory.The macro increments
 * the supplied cache line pointer by the cache size.
 */
#define FLUSHINV(P)                                \
    do {                                           \
        asm volatile("FLUSHINV[%0++];" : "+p"(P)); \
    } while (0)

/*!
 * This macro also performs flush and invalidate operation for single cache
 * line.
 */
#define SIMPLEFLUSHINV(P)                          \
    do {                                           \
        ssync();                                   \
        asm volatile("FLUSHINV[%0];"::"#p"(P));    \
        ssync();                                   \
    } while (0)

#define SIMPLEINV(P)    do { SIMPLEFLUSHINV(P) } while (0)

#define FLUSH_AREA_CODE                                                                        \
	do {																					   \
		uint32_t count;                                                                            \
		uint32_t x;                                                                                \
		count = (bytes + 31U + (((uint32_t)start) & 31U))/32U;                                     \
		/* System Sync */                                                                          \
		ssync();                                                                                   \
		/* Flush given cache area */                                                               \
		for (x=0U; x < count; x++)  {                                                              \
			/* Flush the cache line at location start and increment start to next cache line */    \
			FLUSH(start);                                                                          \
		}                                                                                          \
		/* System Sync */                                                                          \
		ssync();																				   \
	} while(0)
    
#define FLUSHINV_AREA_CODE                                                                     \
	do {																					   \
		uint32_t count;                                                                            \
		uint32_t x;                                                                                \
		count = (bytes + 31U + (((uint32_t)start) & 31U))/32U;                                     \
		/* System Sync */                                                                          \
		ssync();                                                                                   \
		/* Flush given cache area */                                                               \
		for (x=0U; x < count; x++)  {                                                              \
			/* Flush the cache line at location start and increment start to next cache line */    \
			FLUSHINV(start);                                                                       \
		}                                                                                          \
		/* System Sync */                                                                          \
		ssync();																				   \
	} while (0)

#elif defined(__ADSP215xx__)
    
/*!
 * This macro also performs flush and invalidate operation for single cache
 * line.
 */
#define SIMPLEFLUSHINV(P)                          \
    do {                                           \
    	flush_data_buffer(P,P,ADI_FLUSH_DATA_INV); \
    } while (0)

#define FLUSH_AREA_CODE                                                                        \
    /* Calculate the start and end address to 32-byte cache boundaries */                      \
    uint32_t start_u32 = ((uint32_t)start) & (~((uint32_t)31u));                               \
    uint32_t end_u32   = ((uint32_t)start + bytes - 1) & (~((uint32_t)31u));                   \
    flush_data_buffer((void*)start_u32, (void*)end_u32, ADI_FLUSH_DATA_INV);
    
#define FLUSHINV_AREA_CODE      FLUSH_AREA_CODE

#elif defined(ADI_ADSP_CM40Z)

#   define SYS_SSYNC
#   define FLUSH_AREA_CODE
#   define FLUSHINV_AREA_CODE
#   define SIMPLEFLUSHINV

#else
#error "Processor not supported"
#endif


    
/********* Sync Code *********/

#ifdef __ADSPBLACKFIN__
#   define SYS_SSYNC  ssync();
#elif defined(ADI_ADSP_CM40Z) || defined(__ADSP215xx__)
#   define SYS_SSYNC
#else
#   error "Processor not supported"
#endif



/********* Wait Code *********/

#if  defined(ADI_ADSP_CM40Z) || defined(__ADSP215xx__)

#    define WAIT_MILLISECOND_CODE                       \
        volatile unsigned long long int cur,nd;         \
        cur = 0;                                        \
        nd = 1000;                                      \
        while (cur < nd) { cur++;  }

    
#elif defined (__ADSPBLACKFIN__)
#   include <cycle_count_bf.h>

#   define WAIT_MILLISECOND_CODE                        \
		do {											\
        volatile unsigned long long int cur,nd;         \
        _GET_CYCLE_COUNT(cur);                          \
        nd = cur + (__PROCESSOR_SPEED__/1000);          \
        while (cur < nd) { _GET_CYCLE_COUNT(cur); }	    \
		} while(0)
    
    
#else
#   error "Processor not supported"
#endif







/********* PHY Interrupt Code *********/

#if defined(__SAM_V1__)
#define EMAC0_PHY0_DP8386X_PORT_CFG                   \
    *pREG_PINT4_ASSIGN  |= (BITM_PINT_ASSIGN_B0MAP) ;   \
    *pREG_PINT4_MSK_SET  = BITM_PINT_MSK_SET_PIQ6  ;   \
    *pREG_PINT4_EDGE_CLR = BITM_PINT_EDGE_SET_PIQ6 ;   \
    *pREG_PINT4_INV_SET  = BITM_PINT_INV_SET_PIQ6  ;   \
                                                        \
    *pREG_PORTF_FER_CLR  = BITM_PORT_FER_CLR_PX6  ;    \
    *pREG_PORTF_DIR_CLR  = BITM_PORT_DIR_CLR_PX6  ;    \
    *pREG_PORTF_INEN_SET = BITM_PORT_INEN_SET_PX6 ;    \
    *pREG_PORTF_POL_SET  = BITM_PORT_POL_SET_PX6

#define EMAC0_PHY0_DP8386X_ACK_PHY_INT    *pREG_PINT4_REQ      = BITM_PINT_REQ_PIQ6

#define EMAC0_PHY0_DP8386X_INT             INTR_PINT4_BLOCK

#elif defined(__ADSPBF6xx__)

#define EMAC0_PHY0_DP83848_PORT_CFG                         \
do { 									                    \
    *pREG_PINT2_ASSIGN   |= (BITM_PINT_ASSIGN_B0MAP);       \
    *pREG_PINT2_MSK_SET   = BITM_PINT_MSK_SET_PIQ6 ;        \
    *pREG_PINT2_INV_SET   = BITM_PINT_INV_SET_PIQ6 ;        \
                                                            \
    *pREG_PORTD_FER_CLR   = BITM_PORT_FER_CLR_PX6  ;        \
    *pREG_PORTD_DIR_CLR   = BITM_PORT_DIR_CLR_PX6  ;        \
    *pREG_PORTD_INEN_SET  = BITM_PORT_INEN_SET_PX6 ;        \
    *pREG_PORTD_POL_SET   = BITM_PORT_POL_SET_PX6  ;        \
                                                            \
    *pREG_SEC0_GCTL      |= ENUM_SEC_GCTL_EN;               \
    *pREG_SEC0_CCTL0     |= ENUM_SEC_CCTL_EN;               \
                                                            \
    *pREG_PINT2_LATCH    = BITM_PINT_LATCH_PIQ6;            \
} while (0)
    
#define EMAC0_PHY0_DP83848_INT    (INTR_PINT2_BLOCK)

    
#define EMAC0_PHY0_DP83848_ACK_PHY_INT    do { *pREG_PINT2_REQ = BITM_PINT_REQ_PIQ6; } while (0)
    
    
#elif defined(__ADSPSC589_FAMILY__)

#define EMAC0_PHY0_DP8386X_PORT_CFG                   \
	*pREG_PINT1_ASSIGN  |= (BITM_PINT_ASSIGN_B1MAP) ;   \
    *pREG_PINT1_MSK_SET  = BITM_PINT_MSK_SET_PIQ15  ;   \
    *pREG_PINT1_EDGE_CLR = BITM_PINT_EDGE_SET_PIQ15 ;   \
    *pREG_PINT1_INV_SET  = BITM_PINT_INV_SET_PIQ15  ;   \
                                                        \
    *pREG_PORTC_FER_CLR  = BITM_PORT_FER_CLR_PX15  ;    \
    *pREG_PORTC_DIR_CLR  = BITM_PORT_DIR_CLR_PX15  ;    \
    *pREG_PORTC_INEN_SET = BITM_PORT_INEN_SET_PX15 ;    \
    *pREG_PORTC_POL_SET  = BITM_PORT_POL_SET_PX15


#define EMAC0_PHY0_DP8386X_ACK_PHY_INT    *pREG_PINT1_REQ      = BITM_PINT_REQ_PIQ15

#define EMAC0_PHY0_DP8386X_INT             INTR_PINT1_BLOCK

#if defined(__ADSPSC587__) || defined(__ADSPSC589__)

#define EMAC1_PHY0_DP83848_PORT_CFG                     \
    *pREG_PINT2_ASSIGN   |= (BITM_PINT_ASSIGN_B0MAP);   \
    *pREG_PINT2_MSK_SET   = BITM_PINT_MSK_SET_PIQ0 ;    \
    *pREG_PINT2_INV_SET   = BITM_PINT_INV_SET_PIQ0 ;    \
                                                        \
    *pREG_PORTD_FER_CLR   = BITM_PORT_FER_CLR_PX0  ;    \
    *pREG_PORTD_DIR_CLR   = BITM_PORT_DIR_CLR_PX0  ;    \
    *pREG_PORTD_INEN_SET  = BITM_PORT_INEN_SET_PX0 ;    \
    *pREG_PORTD_POL_SET   = BITM_PORT_POL_SET_PX0  ;
    
#define EMAC1_PHY0_DP83848_INT    INTR_PINT2_BLOCK
                                                        
    
#define EMAC1_PHY0_DP83848_ACK_PHY_INT     *pREG_PINT2_REQ      = BITM_PINT_REQ_PIQ0;
#endif
#elif defined(__ADSPSC573_FAMILY__)

#define EMAC0_PHY0_DP8386X_PORT_CFG                   \
    *pREG_PINT0_ASSIGN   = (*pREG_PINT0_ASSIGN & ~(uint32_t)BITM_PINT_ASSIGN_B2MAP);  \
    *pREG_PINT0_MSK_SET  = BITM_PINT_MSK_SET_PIQ20  ; \
    *pREG_PINT0_EDGE_CLR = BITM_PINT_EDGE_SET_PIQ20 ; \
    *pREG_PINT0_INV_SET  = BITM_PINT_INV_SET_PIQ20  ; \
            										  \
    *pREG_PORTA_FER_CLR  = BITM_PORT_FER_CLR_PX4  ;   \
    *pREG_PORTA_DIR_CLR  = BITM_PORT_DIR_CLR_PX4  ;   \
    *pREG_PORTA_INEN_SET = BITM_PORT_INEN_SET_PX4 ;   \
    *pREG_PORTA_POL_SET  = BITM_PORT_POL_SET_PX4


#define EMAC0_PHY0_DP8386X_ACK_PHY_INT    *pREG_PINT0_REQ      = BITM_PINT_REQ_PIQ20

#define EMAC0_PHY0_DP8386X_INT             INTR_PINT0_BLOCK
    
#elif defined(ADI_ADSP_CM40Z)    

#define EMAC0_PHY0_DP83848_PORT_CFG                                                         \
        *pREG_PINT2_ASSIGN    = (*pREG_PINT2_ASSIGN & ~(uint32_t)BITM_PINT_ASSIGN_B0MAP);   \
        *pREG_PINT2_MSK_SET   = BITM_PINT_MSK_SET_PIQ6 ;    \
        *pREG_PINT2_INV_SET   = BITM_PINT_INV_SET_PIQ6 ;    \
                                                            \
        *pREG_PORTC_FER_CLR   = BITM_PORT_FER_CLR_PX6  ;    \
        *pREG_PORTC_DIR_CLR   = BITM_PORT_DIR_CLR_PX6  ;    \
        *pREG_PORTC_INEN_SET  = BITM_PORT_INEN_SET_PX6 ;    \
        *pREG_PORTC_POL_SET   = BITM_PORT_POL_SET_PX6  ;    \
        *pREG_PINT2_LATCH     = BITM_PINT_LATCH_PIQ6;
        
        
#define EMAC0_PHY0_DP83848_INT    INTR_PINT2_BLOCK

#define EMAC0_PHY0_DP83848_ACK_PHY_INT     *pREG_PINT2_REQ      = BITM_PINT_REQ_PIQ6;

#else
#error "PHY configuration Error "    
#endif







#endif /* _ADI_GEMAC_PROC_H_ */
