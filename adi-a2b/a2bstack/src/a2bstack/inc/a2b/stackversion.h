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
 * \file:   stackversion.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The A2B Stack API version information.
 *
 *=============================================================================
 */

#ifndef A2B_STACKVERSION_H_
#define A2B_STACKVERSION_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/

#define A2B_STACK_VER_MAJOR     (0u)
#define A2B_STACK_VER_MINOR     (0u)
#define A2B_STACK_VER_RELEASE   (0u)

#define A2B_STACK_VER_STRING    "0.0.0"

#define A2B_STACK_VERSION       ((0u << 16u) | \
                                (0u << 8u) | \
                                (0u))
                                
/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/*======================= P U B L I C  P R O T O T Y P E S ========*/


A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_STACKVERSION_H_ */
