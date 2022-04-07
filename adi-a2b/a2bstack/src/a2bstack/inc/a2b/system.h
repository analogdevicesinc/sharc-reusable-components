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
 * \file:   system.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The definitions/prototypes for the system-level API.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_sys           System Module
 *  
 *  The definitions/prototypes for the system-level API.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_SYSTEM_H_
#define A2B_SYSTEM_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/ecb.h"
#include "a2b/pal.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_systemInitialize(
                          a2b_PlatformInitFunc  f, 
                          A2B_ECB*              ecb);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_systemShutdown(
                          a2b_PlatformShutdownFunc  f, 
                          A2B_ECB*                  ecb);

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_sys */

#endif /* A2B_SYSTEM_H_ */
