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
 * \file:   verinfo.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the API for retrieving version/build information
 *          from the plugin.
 *
 *=============================================================================
 */

#ifndef A2B_VERINFO_H_
#define A2B_VERINFO_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_PluginVerInfo;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT void a2b_mstr_getVerInfo(struct a2b_PluginVerInfo* verInfo);

A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_VERINFO_H_ */
