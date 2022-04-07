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
 * \file:   periphutil.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Define utility functions for dealing with slave plugins and
 *          peripherals.
 *
 *=============================================================================
 */

#ifndef A2B_PERIPHUTIL_H_
#define A2B_PERIPHUTIL_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Plugin;

typedef void (* a2b_OpDoneFunc)(a2b_HResult result, a2b_Handle userData);

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT a2b_Int32 a2b_periphInit(struct a2b_Plugin* plugin,
                                        a2b_Int16 startNode,
                                        a2b_Int16 endNode,
                                        a2b_OpDoneFunc cb,
                                        a2b_Handle userData,
                                        a2b_HResult* status);

A2B_EXPORT a2b_Int32 a2b_periphDeinit(struct a2b_Plugin* plugin,
                                        a2b_Int16 startNode,
                                        a2b_Int16 endNode,
                                        a2b_OpDoneFunc cb,
                                        a2b_Handle userData,
                                        a2b_HResult* status);



A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_PERIPHUTIL_H_ */
