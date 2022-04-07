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
 * \file:   netutil.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation the A2B override APIs.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/
#include "a2b/pluginapi.h"
#include "override.h"
#include "plugin_priv.h"
#include "a2bplugin-master/plugin.h"
#include "a2b/regdefs.h"
#include "a2b/trace.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
*
*  \b              a2b_ovrGetDiscMode
*
*  Applies the discovery mode override and returns the resulting value.
*
*  \param          [in]    plugin   The master plugin.
*
*  \pre            It's assumed the plugin is a valid plugin pointer. No error
*                  checking on the pointer is performed.
*
*  \post           None
*
*  \return         A potentially overridden value for the discovery mode.
*
******************************************************************************/
bdd_DiscoveryMode
a2b_ovrGetDiscMode
    (
    const struct a2b_Plugin*    plugin
    )
{
    bdd_DiscoveryMode mode = bdd_DISCOVERY_MODE_SIMPLE;

    /* One or the other, can't do both */
    if ( plugin->overrides[0] & A2B_MPLUGIN_FORCE_MODIFIED )
    {
        mode = bdd_DISCOVERY_MODE_MODIFIED;
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "Applying override for Modified Discovery"));
    }
    else if ( plugin->overrides[0] &  A2B_MPLUGIN_FORCE_SIMPLE )
    {
        mode = bdd_DISCOVERY_MODE_SIMPLE;
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "Applying override for Simple Discovery"));
    }
    else if ( plugin->overrides[0] &  A2B_MPLUGIN_FORCE_OPTIMIZED )
    {
        mode = bdd_DISCOVERY_MODE_OPTIMIZED;
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "Applying override for Simple Discovery"));
    }
    else if ( plugin->overrides[0] &  A2B_MPLUGIN_FORCE_ADVANCED )
    {
        mode = bdd_DISCOVERY_MODE_ADVANCED;
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "Applying override for Simple Discovery"));
    }
    else
    {
    	mode = plugin->bdd->policy.discoveryMode;
    }

    return mode;
}


/*!****************************************************************************
*
*  \b              a2b_ovrGetDiscCfgMethod
*
*  Applies the discovery configuration override and returns the resulting value.
*
*  \param          [in]    plugin   The master plugin.
*
*  \pre            It's assumed the plugin is a valid plugin pointer. No error
*                  checking on the pointer is performed.
*
*  \post           None
*
*  \return         A potentially overridden value for the discovery
*                  configuration method.
*
******************************************************************************/
bdd_ConfigMethod
a2b_ovrGetDiscCfgMethod
    (
    const struct a2b_Plugin*    plugin
    )
{
    bdd_ConfigMethod cfg = plugin->bdd->policy.cfgMethod;

    /* One or the other, can't do both */
    if ( plugin->overrides[0] & A2B_MPLUGIN_FORCE_BDD_CFG_METHOD )
    {
        cfg = bdd_CONFIG_METHOD_BDD;
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "Applying override for BDD Config Method"));
    }
    else if ( plugin->overrides[0] &  A2B_MPLUGIN_FORCE_HYBRID_CFG_METHOD )
    {
        cfg = bdd_CONFIG_METHOD_HYBRID;
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "Applying override for Hybrid Config Method"));
    }
    /* If no override then leave config method to default value  */
    else
    {

    }

    return cfg;
}


/*!****************************************************************************
*
*  \b              a2b_ovrApplyIntrActive
*
*  Applies the GPIO0 interrupt active override to the following set of
*  A2B registers:
*       A2B_REG_PINTINV
*       A2B_REG_PINTEN
*       A2B_REG_GPIOIEN
*       A2B_REG_INTMSK1
*       A2B_REG_INTMSK2
*
*  These are the *only* registers understood by this function. Calling this
*  function with a different register will always return zero (0).
*
*  \param          [in]    plugin       The master plugin.
*
*  \param          [in]    bddNodeIdx   The node index into the BDD structure.
*                                       The index starts at zero (0).
*
*  \param          [in]    regOffset    The AD2410 register offset. It must
*                                       be one of the following values:
*                                           A2B_REG_PINTINV
*                                           A2B_REG_PINTEN
*                                           A2B_REG_GPIOIEN
*                                           A2B_REG_INTMSK1
*                                           A2B_REG_INTMSK2
*
*  \pre            It's assumed the plugin is a valid plugin pointer. No error
*                  checking on the pointer is performed. The bddNodeIdx must
*                  be >= 0. The register offset must be one of the ones
*                  supported.
*
*  \post           None
*
*  \return         A potentially overridden value for the register.
*
******************************************************************************/
a2b_UInt32
a2b_ovrApplyIntrActive
    (
    const struct a2b_Plugin*    plugin,
    a2b_Int16                   bddNodeIdx,
    a2b_UInt32                  regOffset
    )
{
    a2b_UInt32  value = 0u;

    if ( (bddNodeIdx >= 0)  && (bddNodeIdx < (a2b_Int16)plugin->bdd->nodes_count) )
    {
        switch ( regOffset )
        {
            case A2B_REG_PINTINV:
                value = plugin->bdd->nodes[bddNodeIdx].pinIoRegs.pintinv;
                break;

            case A2B_REG_PINTEN:
                value = plugin->bdd->nodes[bddNodeIdx].pinIoRegs.pinten;
                break;

            case A2B_REG_GPIOIEN:
                value = plugin->bdd->nodes[bddNodeIdx].pinIoRegs.gpioien;
                break;

            case A2B_REG_INTMSK1:
                value = plugin->bdd->nodes[bddNodeIdx].intRegs.intmsk1;
                break;

            case A2B_REG_INTMSK2:
                /* This is a master-only register */
                value = plugin->bdd->nodes[0].intRegs.intmsk2;
                break;

            default:
                A2B_TRACE1((plugin->ctx,
                        (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                        "IntrActOverride: invalid register specified: 0x%X",
                        &regOffset));
                break;
        }


        /* If the GPIO_0 interrupt overrides are active then ... */
        if ( plugin->overrides[0] &
            (A2B_MPLUGIN_CFG_GPIO0_INTRPT_ACT_HIGH |
             A2B_MPLUGIN_CFG_GPIO0_INTRPT_ACT_LOW) )
        {

            switch ( regOffset )
            {
                case A2B_REG_PINTINV:
                    if ( plugin->overrides[0] &
                       A2B_MPLUGIN_CFG_GPIO0_INTRPT_ACT_HIGH )
                    {
                       /* Force an active high trigger (0->1 transition) */
                       value &= (a2b_UInt32)~((a2b_UInt32)A2B_BITM_PINTINV_IO0INV);
                    }
                    else
                    {
                       /* Force an active low trigger (1->0 transition) */
                       value |= A2B_BITM_PINTINV_IO0INV;
                    }
                    break;

                case A2B_REG_PINTEN:
                    value |= A2B_BITM_PINTEN_IO0IE;
                    break;

                case A2B_REG_GPIOIEN:
                    value |= A2B_BITM_GPIOIEN_IO0IEN;
                    break;

                case A2B_REG_INTMSK1:
                    value |= A2B_BITM_INTMSK1_IO0PND;
                    break;

                case A2B_REG_INTMSK2:
                    value |= A2B_BITM_INTMSK2_SLVIRQ;
                    break;

                default:
                    break;
            }
        }
    }

    return value;
}
