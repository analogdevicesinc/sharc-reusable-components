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

/* These settings are for SOMCRR EZKIT 1.0 */

#define EMAC0_PHY_NUM_DEV      (1)
#define EMAC0_PHY_DEV          (0)

#define EMAC0_PHY0_DEV         (PHY_DEV_DP8386X)
#define EMAC0_PHY0_ADDR        (0x1)

#define EMAC1_PHY_NUM_DEV      (1)
#define EMAC1_PHY0_DEV         (PHY_DEV_DP83848)
#define EMAC1_PHY0_ADDR        (0x1)

#define EMAC0_PHY0_DP8386X_PORT_CFG                   \
    *pREG_PINT5_ASSIGN  |= (BITM_PINT_ASSIGN_B1MAP) ;   \
    *pREG_PINT5_MSK_SET  = BITM_PINT_MSK_SET_PIQ12 ;   \
    *pREG_PINT5_EDGE_CLR = BITM_PINT_EDGE_SET_PIQ12;   \
    *pREG_PINT5_INV_SET  = BITM_PINT_INV_SET_PIQ12 ;   \
                                                        \
    *pREG_PORTG_FER_CLR  = BITM_PORT_FER_CLR_PX12 ;    \
    *pREG_PORTG_DIR_CLR  = BITM_PORT_DIR_CLR_PX12 ;    \
    *pREG_PORTG_INEN_SET = BITM_PORT_INEN_SET_PX12;    \
    *pREG_PORTG_POL_SET  = BITM_PORT_POL_SET_PX12

#define EMAC0_PHY0_DP8386X_ACK_PHY_INT  *pREG_PINT5_REQ = BITM_PINT_REQ_PIQ12

#define EMAC0_PHY0_DP8386X_INT          INTR_PINT5_BLOCK

#define EMAC_PHY_CONFIG         (ADI_GEMAC_PHY_CFG_AUTO_NEGOTIATE_EN)

#define EMAC1_PHY0_DP83848_PORT_CFG                   \
    *pREG_PINT6_ASSIGN  |= (BITM_PINT_ASSIGN_B1MAP) ;   \
    *pREG_PINT6_MSK_SET  = BITM_PINT_MSK_SET_PIQ2 ;   \
    *pREG_PINT6_EDGE_CLR = BITM_PINT_EDGE_SET_PIQ2;   \
    *pREG_PINT6_INV_SET  = BITM_PINT_INV_SET_PIQ2 ;   \
                                                        \
    *pREG_PORTH_FER_CLR  = BITM_PORT_FER_CLR_PX2 ;    \
    *pREG_PORTH_DIR_CLR  = BITM_PORT_DIR_CLR_PX2 ;    \
    *pREG_PORTH_INEN_SET = BITM_PORT_INEN_SET_PX2;    \
    *pREG_PORTH_POL_SET  = BITM_PORT_POL_SET_PX2

#define EMAC1_PHY0_DP83848_ACK_PHY_INT  *pREG_PINT6_REQ = BITM_PINT_REQ_PIQ2

#define EMAC1_PHY0_DP83848_INT          INTR_PINT6_BLOCK

#endif
