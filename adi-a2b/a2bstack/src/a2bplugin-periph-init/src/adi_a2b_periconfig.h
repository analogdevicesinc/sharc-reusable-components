/*******************************************************************************
Copyright (c) 2016 - Analog Devices Inc. All Rights Reserved.
This software is proprietary & confidential to Analog Devices, Inc.
and its licensors.
******************************************************************************
* @file: adi_a2b_periconfig.h
* @brief: This file contains Typedefs,configurable macros and A2B framework structures
* @version: $Revision: 3888 $
* @date: $Date: 2016-01-17 18:46:38 +0530 (Sun, 17 Jan 2016) $
* Developed by: Automotive Software and Systems team, Bangalore, India
*****************************************************************************/
/*! \addtogroup Network_Configuration
* @{
*/

/*! \addtogroup Remote_Peripheral_Configuration
* @{
*/

#ifndef _ADI_A2B_PERICONFIG_H_
#define _ADI_A2B_PERICONFIG_H_

/*============= I N C L U D E S =============*/

#include "a2b/ctypes.h"
#include "adi_a2b_busconfig.h"

/*============= D E F I N E S =============*/

/*============= D A T A T Y P E S=============*/
struct a2b_Timer;
struct a2b_Plugin;


/*======= P U B L I C P R O T O T Y P E S ========*/
void adi_a2b_Concat_Addr_Data(a2b_UInt8 pDstBuf[] ,a2b_UInt32 nAddrwidth, a2b_UInt32 nAddr);
a2b_HResult adi_a2b_PeriheralConfig(struct a2b_Plugin* plugin, ADI_A2B_NODE_PERICONFIG *pPeriConfig);

/**
 @}
*/

/**
 @}
*/
#endif /* _ADI_A2B_PERICONFIG_H_ */





/*
**
** EOF: $URL$
**
*/

