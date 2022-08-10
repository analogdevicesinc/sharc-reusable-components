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
 * \file:   pluginapi.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The prototypes/definitions for the API the A2B stack calls on
 *          registered slave/master plugins.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_pluginapi         Plugin API Module
 *  
 * The prototypes/definitions for the API the A2B stack calls on
 * registered slave/master plugins.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_PLUGINAPI_H_
#define A2B_PLUGINAPI_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/features.h"
#include "a2b/defs.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/** Forward declarations */
struct a2b_StackContext;
struct a2b_Msg;


/* ===========================================
 *
 * Plugin Types and Definitions
 *
 *==========================================*/


/* TODO: NEED TO REWORK SEQUENCE DIAGRAM BELOW */
/**
 * <!--
 * @startuml pluginapi-loadmodule.png
 * Stack -[#blue]> Plugin : a2b_stk2ModOpen(ctx, nodeInfo, nodeAddr)
 * note left
 *  The A2B Stack attempts to find the plugin
 *  that manages an A2B node found during the discovery process
 *  by trying to open each plugin. Each plugin is passed
 *  the vid/pid/version of the newly discovered node along with
 *  it's node address (i.e. order in the discovery process). If the
 *  plugin can manage this node then it returns a handle or
 *  null otherwise. The first plugin that returns a non-null handle
 *  is considered the owner. The plugin's handle is always passed
 *  back in all calls made from the plugin back into the A2B stack
 *  using the API defined in plug2stkapi.h. Similarly, the A2B stack
 *  context is delivered with every call to the plugin from the stack.
 *  Note: Node calls to the plug2stkapi.h can not be executed with the "open"
 *  call since the A2B stack doesn't know (yet) whether this plugin
 *  claims ownership of the node.
 * end note
 * Plugin -[#red]> Stack : Valid handle (can manage)\nor null (if cannot manage)
 * note left
 *  Assume the plugin claims ownership of the node.
 * end note
 * Stack -[#blue]> Plugin: a2b_stk2PlugCommand(A2B_PLUGIN_CMD_INIT, args)
 * note right
 *  Plugin does any necessary initialization.
 *  Can call back into the stack via the APIs
 *  available in plug2stkapi.h.
 * end note
 * Plugin -[#red]> Stack: Initialization succeeds
 * note left
 *  Stack now tells the plugin to
 *  initialize any connected peripherals
 * end note
 * Stack -[#blue]> Plugin: a2b_stk2PlugCommand(A2B_PLUGIN_CMD_CONFIG, args)
 * Plugin -[#red]> Stack
 * note left: The peripherals are configured.
 * @enduml
 * -->
 * @image html pluginapi-loadmodule.png "Flow for Loading a Plugin"
 */
typedef struct a2b_PluginApi
{
    a2b_Char name[A2B_CONF_DEFAULT_PLUGIN_NAME_LEN];

    /*!************************************************************************
    *
    *  \b              open
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
    *                  msg.#a2b_msgGetUserData() to get the handle.
    *
    *  \return         A2B_NULL=error, plugin does NOT handle the nodeSig
    *                  NON-NULL=nodeSig handled by this plugin
    *
    ***************************************************************************/
    a2b_Handle (A2B_CALL * open)(struct a2b_StackContext*   ctx,
                                const a2b_NodeSignature*    nodeSig);

    /*!************************************************************************
    *
    *  \b              close
    *
    *  Called to close the plugin
    *
    *  \param          [in]    hnd    Handle returned to the stack from the
    *                                 plugin open call.
    *
    *  \pre            None
    *
    *  \post           None
    *
    *  \return         Success or Error
    *
    ***************************************************************************/
    a2b_HResult (A2B_CALL * close)(a2b_Handle hnd);

    /*!************************************************************************
    *
    *  \b              execute
    *
    *  Called when a message (Request/Notify/Custom) needs executing by
    *  this plugin.
    *
    *  \param          [in]    msg          The A2B request message.
    *
    *  \param          [in]    pluginHnd    The plugin handle.
    *
    *  \param          [in]    ctx          The stack context associated with
    *                                       the plugin.
    *
    *  \pre            Called via the JobExecutor
    *
    *  \post           None
    *
    *  \return         #A2B_EXEC_COMPLETE == Execution is now complete
    *                  #A2B_EXEC_SCHEDULE == Execution is unfinished - schedule
    *                                        again
    *                  #A2B_EXEC_SUSPEND  == Execution is unfinished - suspend 
    *                                        scheduling until a later event
    *
    ***************************************************************************/
    a2b_Int32 (A2B_CALL * execute)(struct a2b_Msg*  msg,
                          a2b_Handle                pluginHnd,
                          struct a2b_StackContext*  ctx);

    /*!************************************************************************
    *
    *  \b              interrupt
    *
    *  Called to process an interrupt for the plugin.
    *
    *  \param          [in]    ctx       A2B stack context
    *  \param          [in]    hnd       Plugin handler
    *  \param          [in]    intrSrc   The interrupt source per the
    *                                    #A2B_REG_INTSRC register definition.
    *                                    Use masks in regdefs.h to extract
    *                                    specific fields of interest.
    *  \param          [in]    intrType  interrupt type (A2B_ENUM_INTTYPE_*)
    *
    *  \pre            Not called from the JobExecutor
    *
    *  \post           None
    *
    *  \return         None
    *
    ***************************************************************************/
    void (A2B_CALL * interrupt)(struct a2b_StackContext*    ctx,
                                       a2b_Handle           hnd,
                                       a2b_UInt8            intrSrc,
                                       a2b_UInt8            intrType);
} a2b_PluginApi;


/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_pluginapi */

#endif /* A2B_PLUGINAPI_H_ */
