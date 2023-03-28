/*=============================================================================
 *
 * Project: a2bstack
 *
 * Copyright (c) 2015 - Analog Devices Inc. All Rights Reserved.
 * This software is subject to the terms and conditions of the license set 
 * forth in the project LICENSE file. Downloading, reproducing, distributing or 
 * otherwise using the software constitutes acceptance of the license. The 
 * software may not be used except as expressly authorized under the license.
 *
 *=============================================================================
 *
 * \file:   override.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Defines the BDD override APIs.
 *
 *=============================================================================
 */

#ifndef A2B_OVERRIDE_H
#define A2B_OVERRIDE_H

/*======================= I N C L U D E S =========================*/
#ifndef PB_SYSTEM_HEADER
#define PB_SYSTEM_HEADER    "pb_syshdr.h"
#endif

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "bdd_pb2.pb.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward Declarations */
struct a2b_Plugin;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT bdd_DiscoveryMode a2b_ovrGetDiscMode(
                                            const struct a2b_Plugin* plugin);
A2B_EXPORT bdd_ConfigMethod a2b_ovrGetDiscCfgMethod(
                                            const struct a2b_Plugin* plugin);
A2B_EXPORT a2b_UInt32 a2b_ovrApplyIntrActive(const struct a2b_Plugin* plugin,
                                           a2b_Int16 bddNodeIdx,
                                           a2b_UInt32 regOffset);


A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_OVERRIDE_H */
