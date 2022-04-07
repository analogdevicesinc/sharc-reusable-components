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
 * \file:   plugin.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of a simple A2B stack slave plugin.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "adi_a2b_periconfig.h"
#include "plugin-periph-init.h"
#include "plugin_priv.h"
#include "a2b/pluginapi.h"
#include "a2b/error.h"
#include "a2b/conf.h"
#include "a2b/defs.h"
#include "a2b/util.h"
#include "a2b/msg.h"
#include "a2b/msgrtr.h"
#include "a2b/trace.h"
#include "a2b/interrupt.h"
#include "a2b/i2c.h"
#include "a2b/timer.h"
#include "a2b/regdefs.h"
#include "a2b/seqchart.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/
static a2b_Plugin* a2b_pluginFind(a2b_Handle  hnd);
static a2b_Handle a2b_pluginOpen(struct a2b_StackContext* ctx,
    const a2b_NodeSignature*   nodeSig);
static a2b_HResult a2b_pluginClose(a2b_Handle  hnd);
static a2b_Int32 a2b_pluginExecute(struct a2b_Msg*  msg,
    a2b_Handle  pluginHnd, struct a2b_StackContext*  ctx);
static void a2b_pluginInterrupt(struct a2b_StackContext*  ctx,
a2b_Handle  hnd, a2b_UInt8  intrSrc, a2b_UInt8  intrType);

/*======================= D A T A  ================================*/

static a2b_Plugin gsPlugins[A2B_CONF_MAX_NUM_SLAVE_NODES * A2B_CONF_MAX_NUM_MASTER_NODES ];

/*======================= C O D E =================================*/


/*!****************************************************************************
*
*  \b              a2b_pluginFind
*
*  Retrieve the plugin based on its handle.
*
*  \param          [in]    hnd
*
*  \pre            None
*
*  \post           None
*
*  \return         Returns A2B_NULL if the plugin with a matching handle
*                  cannot be found.
*
******************************************************************************/
static a2b_Plugin*
a2b_pluginFind
    (
    a2b_Handle  hnd
    )
{
    a2b_UInt32 idx;
    a2b_Plugin* plugin = A2B_NULL;
    for ( idx = 0u; idx < (a2b_UInt32)A2B_ARRAY_SIZE(gsPlugins); ++idx )
    {
        if ( hnd == (a2b_Handle)&gsPlugins[idx] )
        {
            if ( gsPlugins[idx].inUse )
            {
                plugin = &gsPlugins[idx];
            }
            break;
        }
    }

    return plugin;

} /* a2b_pluginFind */


/*!****************************************************************************
*
*  \b              a2b_pluginOpen
*
*  Called to see if the plugin handles a specific node.
*
*  During discovery we scan through the plugins list trying to open each
*  one giving the nodeInfo and nodeAddr of the discovered node. If the
*  plugin can manage this node then a valid (instantiated) handle is
*  returned, else it returns null and the next plugin is tried. Once a
*  non-null handle is returned it is assumed this is the managing plugin.
*  If no plugin handles a node then it's assumed to be a very dumb node.
*
*  \param          [in]    ctx         A2B stack context
*  \param          [in]    nodeSig     node signature (version/product/etc)
*
*  \pre            None
*
*  \post           The returned handle will be returned on the close() and
*                  is available on all messages to the plugin, use
*                  msg.a2b_msgGetPluginHdl() to get the handle.
*
*  \return         NULL=error, plugin does NOT handle the nodeSig
*                  NON-NULL=nodeSig handled by this plugin
*
******************************************************************************/
static a2b_Handle
a2b_pluginOpen
    (
    struct a2b_StackContext*    ctx,
    const a2b_NodeSignature*   nodeSig
    )
{
    a2b_Plugin* plugin = A2B_NULL;
    struct a2b_Timer* timer = A2B_NULL;
    a2b_UInt32  idx;
    a2b_Int16 nodeAddr = A2B_NODEADDR_NOTUSED;
    static a2b_Bool   bPluginInit = A2B_FALSE;

    if ( A2B_NULL != nodeSig )
    {
        nodeAddr = nodeSig->nodeAddr;

        A2B_TRACE1((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                                 "a2b_pluginOpen: opening slave plugin for "
                                 "nodeAddr = %hd", &nodeAddr));

        if ( !bPluginInit )
        {
            bPluginInit = A2B_TRUE;
            for ( idx = 0u; idx < A2B_ARRAY_SIZE(gsPlugins); ++idx )
            {
                (void)a2b_memset(&gsPlugins[idx], 0, sizeof(gsPlugins[idx]));
            }
        }

        /* We cannot serve as a master plugin */
        if ( nodeAddr == A2B_NODEADDR_MASTER )
        {
            return A2B_NULL;
        }

        /*
         * This BCF slave node peripheral init plugin always instantiates
         * and should be at the end of the plugin list to allow other plugins the
         * opportunity to initialize the slave device if required.
         */

        /*
         * Typically here we might check to see if this slave plugin can
         * actually manage this node. This might be done by interrogating
         * an attached EEPROM to make sure it has the correct vendor and/or
         * product identifier (and version). If it's a manageable node then
         * we'll try to allocate a slave plugin *instance* to associate with
         * the node.
         */

        if (A2B_NULL != ctx)
        {
            /* Look for an available slave plugin control block */
            for ( idx = 0u; idx < (a2b_UInt32)A2B_ARRAY_SIZE(gsPlugins); ++idx )
            {
                if ( !gsPlugins[idx].inUse )
                {
                    timer = a2b_timerAlloc(ctx, A2B_NULL, &gsPlugins[idx]);
                    if ( A2B_NULL == timer )
                    {
                        /* Bail out since we couldn't allocate a timer */
                        A2B_TRACE0((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                             "a2b_pluginOpen: failed to allocate a "
                             "timer for slave plugin"));
                    }
                    else
                    {
                        plugin = &gsPlugins[idx];
                        (void)a2b_memset(plugin, 0, sizeof(*plugin));
                        plugin->timer   = timer;
                        plugin->ctx     = ctx;
                        plugin->nodeSig = *nodeSig;
                        plugin->inUse   = A2B_TRUE;
                    }
                    break;
                }
            }
        }
    }
    return plugin;

} /* a2b_pluginOpen */


/*!****************************************************************************
*
*  \b              a2b_pluginClose
*
*  Called to close the plugin
*
*  \param          [in]    hnd
*
*  \pre            None
*
*  \post           None
*
*  \return         Success or Error
*
******************************************************************************/
static a2b_HResult
a2b_pluginClose
    (
    a2b_Handle  hnd
    )
{
    a2b_Plugin* plugin;
    a2b_HResult status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                            A2B_EC_INVALID_PARAMETER);

    plugin = a2b_pluginFind(hnd);
    if ( A2B_NULL != plugin )
    {
        A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                                 "a2b_pluginClose: closing slave plugin for "
                                 "nodeAddr = %hd", &plugin->nodeSig.nodeAddr));
        (void)a2b_timerUnref(plugin->timer);
        plugin->ctx         = A2B_NULL;
        plugin->inUse       = A2B_FALSE;

        status = A2B_RESULT_SUCCESS;
    }

    return status;

} /* a2b_pluginClose */


/*!****************************************************************************
*
*  \b              a2b_pluginExecute
*
*  Called when a job needs executing by this plugin.
*
*  \param          [in]    msg          A2B message to process.
*
*  \param          [in]    pluginHnd    The plugin's handle.
*
*  \param          [in]    ctx          The plugin's context.
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_EXEC_COMPLETE == Execution is now complete
*                  A2B_EXEC_SCHEDULE == Execution is unfinished - schedule again
*                  A2B_EXEC_SUSPEND  == Execution is unfinished - suspend
*                                       scheduling until a later event
*
******************************************************************************/
static a2b_Int32
a2b_pluginExecute
    (
    struct a2b_Msg*             msg,
    a2b_Handle                  pluginHnd,
    struct a2b_StackContext*    ctx
    )
{
    a2b_Plugin*                 plugin = (a2b_Plugin*)pluginHnd;
    a2b_Int32                   ret    = A2B_EXEC_COMPLETE;
    a2b_UInt32                  cmd;
    a2b_PluginInit*             initMsg;
    struct a2b_PluginVerInfo*   verInfo;
#ifdef ENABLE_PERI_CONFIG_BCF
    a2b_HResult                 nRes;
    ADI_A2B_NODE_PERICONFIG    (*pPeriConfig)[];  /* Pointer to an array of configuration */
    ADI_A2B_NODE_PERICONFIG     *pNodePeriDeviceConfig;
    a2b_Int16                   nodeAddr = a2b_msgGetDestNodeAddr(msg);
#endif

#ifndef A2B_FEATURE_TRACE
    A2B_UNUSED(ctx);
#endif

    if ( (plugin == A2B_NULL) || (A2B_NULL == msg) )
    {
        A2B_TRACE1((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "Exit: %s execute(): Internal error",
                    A2B_SLAVE_PLUGIN_NAME));
        return ret;
    }

    cmd = a2b_msgGetCmd(msg);

    A2B_TRACE2((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1),
                "Enter: %s execute(%ld):", A2B_SLAVE_PLUGIN_NAME, &cmd));

    switch ( cmd )
    {
        case A2B_MSGREQ_PLUGIN_PERIPH_INIT:
            initMsg = (a2b_PluginInit*)a2b_msgGetPayload( msg );
            initMsg->resp.status = A2B_RESULT_SUCCESS;
#ifdef ENABLE_PERI_CONFIG_BCF
            if(initMsg->req.pNodePeriDeviceConfig != A2B_NULL)
            {
                pPeriConfig = (ADI_A2B_NODE_PERICONFIG (*)[])initMsg->req.pNodePeriDeviceConfig;
                pNodePeriDeviceConfig = &((*pPeriConfig)[((a2b_UInt32)nodeAddr + (a2b_UInt32)1)]);
                (void)a2b_ActiveDelay(plugin->ctx, 10);
                nRes = adi_a2b_PeriheralConfig(plugin, pNodePeriDeviceConfig);
            }
#endif
            ret = A2B_EXEC_COMPLETE;
            break;

        case A2B_MSGREQ_PLUGIN_PERIPH_DEINIT:
            ret = A2B_EXEC_COMPLETE;
            break;

        case A2B_MSGREQ_PLUGIN_VERSION:
            verInfo = (struct a2b_PluginVerInfo*)a2b_msgGetPayload(msg);
            verInfo->resp.majorVer  = 0;
            verInfo->resp.minorVer  = 0;
            verInfo->resp.relVer    = 0;
            verInfo->resp.buildInfo = __DATE__;
            ret = A2B_EXEC_COMPLETE;
            break;

        default:
            A2B_TRACE2((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_WARN),
                        "%s execute(%ld): Unhandled command",
                        A2B_SLAVE_PLUGIN_NAME, &cmd));
            break;
    }

    A2B_TRACE3((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1),
                "Exit: %s execute(%ld): 0x%lX", A2B_SLAVE_PLUGIN_NAME,
                &cmd, &ret));

    return ret;

} /* a2b_pluginExecute */


/*!****************************************************************************
*
*  \b              a2b_pluginInterrupt
*
*  Called to process an interrupt for the plugin.
*
*  \param          [in]    ctx       A2B stack context
*
*  \param          [in]    hnd       Plugin handler
*
*  \param          [in]    intrSrc   The source (node address) of the
*                                    interrupt.
*
*  \param          [in]    intrType  Interrupt type (A2B_ENUM_INTTYPE_*)
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
*******************************************************************************/
static void
a2b_pluginInterrupt
    (
    struct a2b_StackContext*    ctx,
    a2b_Handle                  hnd,
    a2b_UInt8                   intrSrc,
    a2b_UInt8                   intrType
    )
{
    a2b_Plugin* plugin = a2b_pluginFind(hnd);

    A2B_UNUSED(ctx);

    if ( A2B_NULL != plugin )
    {
        switch ( intrType )
        {
            /* Slave plugins *only* receive GPIO interrupts */
            case A2B_ENUM_INTTYPE_IO0PND:
            case A2B_ENUM_INTTYPE_IO1PND:
            case A2B_ENUM_INTTYPE_IO2PND:
            case A2B_ENUM_INTTYPE_IO3PND:
            case A2B_ENUM_INTTYPE_IO4PND:
            case A2B_ENUM_INTTYPE_IO5PND:
            case A2B_ENUM_INTTYPE_IO6PND:
            case A2B_ENUM_INTTYPE_IO7PND:
                break;

            default:
                break;
        }
    }

} /* a2b_pluginInterrupt */


/*!****************************************************************************
*
*  \b              A2B_SLAVE_PLUGIN_INIT
*
*  Called by PAL to init the slave plugin.
*
*  \param          [in]    api
*
*  \pre            None
*
*  \post           None
*
*  \return         Returns A2B_TRUE on success, A2B_FALSE otherwise.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Bool
A2B_SLAVE_PERI_INIT_PLUGIN_INIT
    (
    struct a2b_PluginApi*   api
    )
{
    a2b_Bool status = A2B_FALSE;

    if ( A2B_NULL != api )
    {
        api->open      = &a2b_pluginOpen;
        api->close     = &a2b_pluginClose;
        api->execute   = &a2b_pluginExecute;
        api->interrupt = &a2b_pluginInterrupt;

        (void)a2b_strncpy(api->name, A2B_SLAVE_PLUGIN_NAME,
                    (a2b_Size)A2B_ARRAY_SIZE(api->name) - 1u);
        api->name[A2B_ARRAY_SIZE(api->name) - 1u] = '\0';

        status = A2B_TRUE;
    }

    return status;
} /* A2B_PLUGIN_INIT */
