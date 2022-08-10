/**
 * Copyright (c) 2021 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

// Target specific configuration for gemac driver
#ifndef _gemac_config_h_
#define _gemac_config_h_

/*
 * Legacy PHY configuration parameters for a variety of eval boards.
 * Only use this file with a Sharc Audio Module or other
 * standard ADI eval board.
 *
 * This file DOES NOT support any EV-SOMCRR-EZKIT based platforms.
 */

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

#endif


