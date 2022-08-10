/*=============================================================================
 *
 * Project: a2bstack
 *
 * Copyright (c) 2015 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary & confidential to Analog Devices, Inc.
 * and its licensors. See LICENSE for complete details.
 *
 *=============================================================================
 *
 * \file:   a2b_bdd_helper.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The definition of BDD Helper functions
 *
 *=============================================================================
 */

#ifndef A2B_BDD_HELPER_H_
#define A2B_BDD_HELPER_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/features.h"
#include "a2b/conf.h"
#include "a2b/error.h"

#include "a2b/ecb.h"
#include "a2b/pal.h"

#ifdef ADI_SIGMASTUDIO_BCF
#include "adi_a2b_busconfig.h"
#endif
/**
 * The following definitions either need to be set globally project-wide
 * (via a compiler definition) or we'll default to these settings. These
 * *MUST* be defined prior to including any pb_xxx.h or bdd_xxx.h header files.
 */
#ifndef PB_SYSTEM_HEADER
#define PB_SYSTEM_HEADER    "pb_syshdr.h"
#endif

#if !defined(PB_FIELD_32BIT) && !defined(PB_FIELD_16BIT)
/* We have at least 256 tags defined for the BDD so we must at least
 * use a 16-bit number for the tag identifiers.
 */
#define PB_FIELD_16BIT
#endif

/* Disable support for custom streams (support only memory buffers) */
#define PB_BUFFER_ONLY 1

#include "pb_decode.h"
#include "bdd_pb2.pb.h"

/*======================= D E F I N E S ===========================*/


/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/*======================= P U B L I C  P R O T O T Y P E S ========*/


A2B_EXPORT a2b_Bool a2b_bddDecode(  const a2b_Byte* bddData,
                                    a2b_UInt32      bddLen,
                                    bdd_Network*    bddOut );

A2B_EXPORT void a2b_bddPalInit(     A2B_ECB*            ecb,
                                    const bdd_Network*  bdd );

/*! \addtogroup Network_Configuration
* @{
*/

/*! \addtogroup Bus_Configuration
* @{
*/
#ifdef ADI_SIGMASTUDIO_BCF
A2B_EXPORT void a2b_bcfParse_bdd(ADI_A2B_BCD *pBusDescription,bdd_Network  *bdd_Graph, a2b_UInt8 nIndex);
A2B_EXPORT void adi_a2b_ParsePeriCfgTable(ADI_A2B_BCD*  pBusDescription, ADI_A2B_NODE_PERICONFIG aPeriDownloadTable[], a2b_UInt8 nIndex);
A2B_EXPORT void adi_a2b_ParsePeriCfgFrComBCF(ADI_A2B_COMPR_BCD*  pBusDescription, ADI_A2B_NODE_PERICONFIG aPeriDownloadTable[], a2b_UInt8 nIndex);
A2B_EXPORT void adi_a2b_ComprBcfParse_bdd( ADI_A2B_COMPR_BCD *pCmprBusDescription, bdd_Network  *bdd_Graph, a2b_UInt8 nIndex);
A2B_EXPORT a2b_HResult a2b_get_bddFromEEPROM(A2B_ECB* ecb, bdd_Network *bdd_Graph, a2b_UInt8* pBuff, a2b_UInt8 pPeriBuf[], ADI_A2B_NETWORK_CONFIG* pTgtProp);
#endif
/**
 @}
*/
A2B_END_DECLS

/*======================= D A T A =================================*/

#endif /* A2B_BDD_HELPER_H_ */
