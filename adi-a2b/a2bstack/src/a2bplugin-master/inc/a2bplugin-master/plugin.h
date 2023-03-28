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
 * \brief:  This is the definition of a A2B master node plugin.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bplugin_master_plugin         Master Plugin Module
 *  
 * This is the definition of a A2B master node plugin.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_MASTER_PLUGIN_H_
#define A2B_MASTER_PLUGIN_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bplugin_master_cmd            Message Commands
 *  
 * Master Plugin Specific Request Message Commands
 *
 * \{ */
/*----------------------------------------------------------------------------*/

#define A2B_MPLUGIN_CMD_START           (1u + A2B_MSGREQ_CUSTOM)

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bplugin_master_overrides      Master Discovery Overrides
 *  
 * Payload uses: a2b_MsgReq_CustomUInt32 where:
 * \code
 *      args[0] == 0x00 = all overrides off
 *      args[1] == 0x00 = all overrides off
 * \endcode
 *
 * \{ */
/*----------------------------------------------------------------------------*/

#define A2B_MPLUGIN_DSCVRY_OVERRIDES        (1u + A2B_MPLUGIN_CMD_START)

    /*------------------------------------------------------------------------*/
    /** 
     * \defgroup a2bplugin_master_overrides0     A2B_MPLUGIN_DSCVRY_OVERRIDES arg[0] bitmasks
     *  
     * Master Discovery Overrides arg[0] bitmasks
     *  
     * \{ */ 
    /*------------------------------------------------------------------------*/

    /** Forces the system to ignore any EEPROM during discovery */
    #define     A2B_MPLUGIN_IGN_EEPROM                  (0x00000001u)

    /** Only grab the EEPROM version for plugin processing, does
     *  NOT process any other EEPROM contents.
     */
    #define     A2B_MPLUGIN_EEPROM_VER_ONLY             (0x00000002u)

    /** Forces a Simple discovery regardless of the BDD */
    #define     A2B_MPLUGIN_FORCE_SIMPLE                (0x00000004u)

    /** Forces a Modified discovery regardless of the BDD */
    #define     A2B_MPLUGIN_FORCE_MODIFIED              (0x00000008u)

    /** Forces a Optimized discovery regardless of the BDD */
    #define     A2B_MPLUGIN_FORCE_OPTIMIZED             (0x00000400u)

    /** Forces a Advanced discovery regardless of the BDD */
    #define     A2B_MPLUGIN_FORCE_ADVANCED              (0x00000800u)

    /** Ignore any peripheral write errors.  This is useful for
     *  testing the EEPROM parsing and handling but don't
     *  care if the peripheral writes fail.
     */
    #define     A2B_MPLUGIN_IGN_PERIPH_ERR              (0x00000010u)

    /** Used to override the BDD config method setting
     *  and force to BDD mode
     */
    #define     A2B_MPLUGIN_FORCE_BDD_CFG_METHOD        (0x00000020u)

    /** Used to override the BDD config method setting
     *  and force to HYBRID mode
     */
    #define     A2B_MPLUGIN_FORCE_HYBRID_CFG_METHOD     (0x00000040u)

    /** Used to override A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS and
     *  force the value to be zero (i.e. NO other peripheral cfg
     *  block processing on a cfg delay).
     *  NOTE: If A2B_CONSECUTIVE_PERIPHERAL_CFGBLOCKS is already
     *  set to zero this will alter the value to be more cooperative
     *  with a value of two.
     */
    #define     A2B_MPLUGIN_FORCE_ZERO_CONSECUTIVE_CFGBLOCKS (0x00000080u)

    /*------------------------------------------------------------------------*/
    /** 
     * \name    GPIO 0 Override
     *  
     * Used to configure GPIO 0 pin on all slave nodes as an interrupt input
     * source. These slave node interrupts propagate to the master
     * node where they can be detected.
     *  
     * \{ */ 
    /*------------------------------------------------------------------------*/
    /** ACT_HIGH - active high trigger (0 -> 1) on GPIO-0 */
    #define     A2B_MPLUGIN_CFG_GPIO0_INTRPT_ACT_HIGH   (0x00000100u)

    /** ACT_LOW - active low trigger (1 -> 0) on GPIO-0 */
    #define     A2B_MPLUGIN_CFG_GPIO0_INTRPT_ACT_LOW    (0x00000200u)
    /** \} */

    /** \} -- a2bplugin_master_overrides0 */

    /*------------------------------------------------------------------------*/
    /** 
     * \defgroup a2bplugin_master_overrides1     A2B_MPLUGIN_DSCVRY_OVERRIDES arg[1] bitmasks
     *  
     * Master Discovery Overrides arg[1] bitmasks
     *
     * \{ */
    /*------------------------------------------------------------------------*/

    /*------------------------------------------------------------------------*/
    /** 
     * \name    Ignore Slave EEPROM
     *  
     * These are used to strategically ignore specific EEPROM
     * configuration. Can be useful in case of problems or testing.
     * All these are ignored when A2B_MPLUGIN_EEPROM_VER_ONLY or
     * A2B_MPLUGIN_IGN_EEPROM have been enabled.
     *
     * \{ */
    /*------------------------------------------------------------------------*/
    #define     A2B_MPLUGIN_IGN_SLAVE0_EEPROM           (0x00000001u)
    #define     A2B_MPLUGIN_IGN_SLAVE1_EEPROM           (0x00000002u)
    #define     A2B_MPLUGIN_IGN_SLAVE2_EEPROM           (0x00000004u)
    #define     A2B_MPLUGIN_IGN_SLAVE3_EEPROM           (0x00000008u)
    #define     A2B_MPLUGIN_IGN_SLAVE4_EEPROM           (0x00000010u)
    #define     A2B_MPLUGIN_IGN_SLAVE5_EEPROM           (0x00000020u)
    #define     A2B_MPLUGIN_IGN_SLAVE6_EEPROM           (0x00000040u)
    #define     A2B_MPLUGIN_IGN_SLAVE7_EEPROM           (0x00000080u)
    #define     A2B_MPLUGIN_IGN_SLAVE8_EEPROM           (0x00000100u)
    /** \} */

    /** \} -- a2bplugin_master_overrides1 */

/** \} -- a2bplugin_master_overrides */

/* Last well defined master plugin command */
#define A2B_MPLUGIN_MAX                     (2u + A2B_MPLUGIN_CMD_START)

/** \} -- a2bplugin_master_cmd */

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bplugin_master_init       External Library API
 *  
 * This is the external entry point for the library (shared or archive).
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/** The master plugin name. Must be less than A2B_CONF_DEFAULT_PLUGIN_NAME_LEN
 * characters long (including A2B_NULL terminator). Should be chosen to be
 * unique from other plugins.
 */
#define A2B_MPLUGIN_PLUGIN_NAME     "A2B Master Plugin"


#ifdef A2B_STATIC_PLUGIN
/* Static linking so this name MUST be unique */

/* Give the option to override this name in case there is a naming conflict. */
#ifndef A2B_MASTER_PLUGIN_INIT
#define A2B_MASTER_PLUGIN_INIT     a2b_pluginMasterInit
#endif

#else
/* Dynamically linked so name should not change since shared library
 * loader will look for this symbol name in the plugin shared library.
 */
#define A2B_MASTER_PLUGIN_INIT     a2b_Master_pluginInit
#endif

#define PRBS_TEST					(0U)
#define AUDIO_TEST					(1U)

/** \} -- a2bplugin_master_init */
/** \} -- a2bplugin_master_plugin */
/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/*! \addtogroup BERT
 *  @{
 */

/*! \struct ADI_A2B_BERT_HANDLER
    Configuration and state preserving structure for BERT calculation
*/
typedef struct
{
    /*! Test Mode */
	a2b_UInt8 nBERTMode; //0-PRBS, 1- Audio

    /*! PRBS error counter */
	a2b_UInt32 nPRBSCount[A2B_CONF_MAX_NUM_SLAVE_NODES + 1u];

    /*! Counter for various errors */
	a2b_UInt32 nErrorCount[A2B_CONF_MAX_NUM_SLAVE_NODES + 1u];

    /*! Indicates if result read is complete (1) or not (0) */
    a2b_UInt8  bReadFlag;

    /*! Auto reset window in Microseconds */
    a2b_UInt32 nAutoResetWindowTime;

    /*!  Overflow flag */
    a2b_UInt32 bOverFlowCount[A2B_CONF_MAX_NUM_SLAVE_NODES + 1u];

}ADI_A2B_BERT_HANDLER;

/**
 @}
*/
/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*! \addtogroup a2bplugin_master_plugin
 *  @{
 */
A2B_DSO_PUBLIC a2b_Bool A2B_CALL A2B_MASTER_PLUGIN_INIT(
                                                    struct a2b_PluginApi* api);
/**
 @}
*/
A2B_END_DECLS

/*======================= D A T A =================================*/



#endif /* A2B_PLUGIN_H_ */
