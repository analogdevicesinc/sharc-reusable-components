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
 * \file:   plugin_priv.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is the definition of a A2B master node plugin.
 *
 *=============================================================================
 */

#ifndef A2B_PLUGIN_PRIV_H_
#define A2B_PLUGIN_PRIV_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/defs.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Timer;
struct a2b_Plugin;

typedef struct a2b_Plugin
{
    struct a2b_StackContext*    ctx;
    a2b_NodeSignature           nodeSig;
    a2b_Bool                    inUse;
    struct a2b_Timer*           timer;
    struct a2b_StackContext*    Mstrctx;
} a2b_Plugin;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_PLUGIN_PRIV_H_ */
