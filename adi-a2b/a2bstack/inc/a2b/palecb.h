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
 * \file:   palecb.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Platform specific extensions to the environment control block (ECB).
 *
 *=============================================================================
 */

/*============================================================================*/
/**
 * \defgroup a2bstack_palecb        PAL ECB
 *
 * Platform specific extensions to the environment control block (ECB).
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_PALECB_H_
#define A2B_PALECB_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"


/*======================= D E F I N E S ===========================*/


/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

typedef struct a2b_PalEcb
{
    void *usrPtr;
} a2b_PalEcb;


/*======================= P U B L I C  P R O T O T Y P E S ========*/


A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_palecb */

#endif /* A2B_PALECB_H_ */
