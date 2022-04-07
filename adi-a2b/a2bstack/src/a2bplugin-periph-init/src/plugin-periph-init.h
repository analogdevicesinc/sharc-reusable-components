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
 * \file:   plugin.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is the definition of a A2B slave node plugin that performs
 *          ADI SigmaStudio BCF slave peripheral I2C initialization.
 *
 *=============================================================================
 */

#ifndef A2B_SLAVE_PERI_INIT_PLUGIN_H
#define A2B_SLAVE_PERI_INIT_PLUGIN_H

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/msgtypes.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

/* Forward declarations */
struct a2b_PluginApi;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/* The name of the plugin. Must be less than A2B_CONF_DEFAULT_PLUGIN_NAME_LEN
 * characters long (including A2B_NULL terminator). Should be chosen to be
 * unique from other plugins.
 */
#define A2B_SLAVE_PLUGIN_NAME     "A2B Peri Init Plugin"

#ifdef A2B_STATIC_PLUGIN
/* Static linking so this MUST be unique for each slave plugin that
 * is defined
 */

/* Give the option to override this name in case there is a naming conflict. */
#ifndef A2B_SLAVE_PERI_INIT_PLUGIN_INIT
#define A2B_SLAVE_PERI_INIT_PLUGIN_INIT     a2b_pluginSlavePeriInit
#endif

#else
/* Dynamically linked so name should not change since shared library
 * loader will look for this symbol name in the plugin shared library.
 */
#define A2B_SLAVE_PLUGIN_INIT     a2b_Slave_pluginInit
#endif

A2B_DSO_PUBLIC a2b_Bool
A2B_CALL A2B_SLAVE_PERI_INIT_PLUGIN_INIT(struct a2b_PluginApi* api);

A2B_END_DECLS

/*======================= D A T A =================================*/

#endif /* A2B_SLAVE_PLUGIN_H */
