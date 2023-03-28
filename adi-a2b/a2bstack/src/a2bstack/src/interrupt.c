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
 * \file:   interrupt.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the public API for A2B interrupt handling.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include <string.h>
#include "a2b/error.h"
#include "a2b/conf.h"
#include "a2b/defs.h"
#include "a2b/util.h"
#include "a2b/msg.h"
#include "a2b/trace.h"
#include "a2b/timer.h"
#include "a2b/msgrtr.h"
#include "stackctx.h"
#include "stack_priv.h"
#include "a2b/interrupt.h"
#include "interrupt_priv.h"
#include "utilmacros.h"


/*======================= D E F I N E S ===========================*/


/*======================= L O C A L  P R O T O T Y P E S  =========*/
static void a2b_intrOnSysTimeout(struct a2b_Timer *timer,
    a2b_Handle userData);

/*======================= D A T A  ================================*/


/*======================= C O D E =================================*/


/*!****************************************************************************
*  \ingroup         a2bstack_interrupt_priv
* 
*  \b               a2b_intrOnSysTimeout
*
*  Called periodically to periodically check the master for interrupts then
*  process any interrupts that were found.  This routine is only called from
*  the timer started from #a2b_intrStartIrqPoll().
*
*  \param          [in]    timer        The timer instance.
* 
*  \param          [in]    userData     An opaque pointer to user data
*                                       originally passed into the timer
*                                       allocation function.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_intrOnSysTimeout
    (
    struct a2b_Timer *timer,
    a2b_Handle userData
    )
{
    a2b_StackContext *ctx = (a2b_StackContext*)userData;

    A2B_UNUSED(timer);

    if ( A2B_NULL != ctx )
    {
        (void)a2b_intrQueryIrq(ctx);
    }

} /* a2b_intrOnSysTimeout */


/*!****************************************************************************
*
*  \b              a2b_intrGetMask
*
*  This function returns the interrupt mask for a specific node.
* 
*  \param          [in]    ctx       A2B Stack Context
* 
*  \param          [in]    nodeAddr  node address: [#A2B_NODEADDR_MASTER(-1)..n]
*
*  \pre            None
*
*  \post           None
*
*  \return         Returns an bitmask of A2B_INTRMASK_*.
*                  Returns #A2B_INTRMASK_READERR on read errors
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_intrGetMask
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr
    )
{
    a2b_UInt32          rBuf = 0u;
    a2b_UInt16          wBuf = (a2b_UInt16)A2B_REG_INTMSK0;
    a2b_HResult         ret;

    if (A2B_NULL == ctx)
    {
        return (a2b_UInt32)A2B_INTRMASK_INVPARAM;
    }

    if (A2B_NODEADDR_MASTER == nodeAddr)
    {
        ret = a2b_i2cMasterWriteRead( ctx, 1u, &wBuf, 3u, &rBuf);
    }
    else
    {
        /* A slave node does not have an INTMSK2 register so we
         * only read INTMSK0-1
         */
        ret = a2b_i2cSlaveWriteRead( ctx, nodeAddr, 1u, &wBuf, 2u, &rBuf);
    }

    if ( A2B_SUCCEEDED(ret) )
    {
        return ((a2b_UInt32)0x00FFFFFF & rBuf);
    }

    return A2B_INTRMASK_READERR;

} /* a2b_intrGetMask */


/*!****************************************************************************
*
*  \b              a2b_intrSetMask
*
*  This sets the interrupt mask for a specific node or ALL nodes.
*
*  \param          [in]    ctx       A2B Stack Context
* 
*  \param          [in]    nodeAddr  node address: [#A2B_NODEADDR_MASTER(-1)..n]
*                                    OR #A2B_NODEADDR_ALL to set all nodes 
*                                    with the same value.
* 
*  \param          [in]    mask      bitmask of A2B_INTRMASK_*
* 
*  \pre            None
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_intrSetMask
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt32                  mask 
    )
{
    a2b_Char            wBuf[4];
    a2b_HResult         ret;
    a2b_UInt8 nTempVar;

    if ((A2B_NULL == ctx))
    {
        return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_INTERRUPT, 
                                A2B_EC_INVALID_PARAMETER);
    }

    wBuf[0] = (a2b_Char)A2B_REG_INTMSK0;

    nTempVar = (a2b_UInt8)mask & 0xFFu;
    wBuf[1] = ((a2b_Char)nTempVar);

    nTempVar = (a2b_UInt8)(mask >> 8u) & 0xFFu;
    wBuf[2] = ((a2b_Char)nTempVar);

    nTempVar = (a2b_UInt8)(mask >> 16u) & 0xFFu;
    wBuf[3] = ((a2b_Char)(nTempVar));

    if (A2B_NODEADDR_MASTER == nodeAddr)
    {
        ret = a2b_i2cMasterWrite( ctx, 4u, &wBuf);
    }
    else
    {
        /* A slave node does not have an INTMSK2 register */
        ret = a2b_i2cSlaveWrite( ctx, nodeAddr, 3u, &wBuf);
    }

    if ( !A2B_SUCCEEDED(ret) )
    {
        return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_INTERRUPT, 
                                A2B_EC_INTERNAL);
    }

    return A2B_RESULT_SUCCESS;

} /* a2b_intrSetMask */


/*!****************************************************************************
*
*  \b              a2b_intrStartIrqPoll
*
*  This will start the timer for interrupt polling/processing.  If hardware
*  interrupts are used t his function is not needed.
*
*  \param          [in]    ctx   A2B stack context
* 
*  \param          [in]    rate  polling rate in msec
*
*  \pre            Returns an error if called from a plugin.
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_intrStartIrqPoll
    (
    struct a2b_StackContext*  ctx,
    a2b_UInt32                rate 
    )
{
    a2b_HResult         status = A2B_RESULT_SUCCESS;
    a2b_IntrInfo*       intrInfo = A2B_NULL;

    if (ctx)
    {
        if (A2B_DOMAIN_APP != ctx->domain)
        {
            return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_INTERRUPT, 
                                    A2B_EC_PERMISSION);
        }

        if (A2B_NULL == ctx->stk->intrInfo)
        {
            intrInfo = (a2b_IntrInfo*)A2B_MALLOC(ctx->stk, sizeof(*intrInfo));
            if (A2B_NULL == intrInfo)
            {
                return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_INTERRUPT, 
                                        A2B_EC_ALLOC_FAILURE);
            }

            (void)a2b_memset(intrInfo, 0, sizeof(*intrInfo));

            ctx->stk->intrInfo = intrInfo;
        }

        if (ctx->stk->intrInfo->timer)
        {
            /* Previously running, stop it first */
            a2b_timerStop(ctx->stk->intrInfo->timer);
        }
        else
        {
            /* Create a new timer */
            ctx->stk->intrInfo->timer = a2b_timerAlloc(ctx, 
                                                     &a2b_intrOnSysTimeout, ctx);
        }

        if ( A2B_NULL == ctx->stk->intrInfo->timer )
        {
            status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_INTERRUPT,
                                      A2B_EC_ALLOC_FAILURE);
        }
        else
        {
            a2b_timerSet(ctx->stk->intrInfo->timer, rate, rate);
            a2b_timerStart(ctx->stk->intrInfo->timer);
        }
    }
    else
    {
        status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_INTERRUPT, 
                                  A2B_EC_INVALID_PARAMETER);
    }

    return status;

} /* a2b_intrStartIrqPoll */


/*!****************************************************************************
*
*  \b              a2b_intrStopIrqPoll
*
*  This will stop the timer for interrupt polling/processing.
*
*  \param          [in]    ctx   A2B stack context
*
*  \pre            Returns an error if called from a plugin.
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_intrStopIrqPoll
    (
    struct a2b_StackContext*  ctx 
    )
{
    if (ctx)
    {
        if (A2B_DOMAIN_APP != ctx->domain)
        {
            return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_INTERRUPT, 
                                    A2B_EC_PERMISSION);
        }

        if ((ctx->stk->intrInfo) && (ctx->stk->intrInfo->timer))
        {
            a2b_timerStop(ctx->stk->intrInfo->timer);
        }
    }

    return A2B_RESULT_SUCCESS;

} /* a2b_intrStopIrqPoll */


/*!****************************************************************************
*
*  \b              a2b_intrQueryIrq
*
*  This is called to trigger the stack to read/check the interrupt status
*  of the master.  If interrupts are pending they will be processed.  At
*  most #A2B_CONF_CONSECUTIVE_INTERRUPTS will be processed in a row before
*  exiting this routine. 
*
*  \param          [in]    ctx  A2B stack context
*
*  \pre            None
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_intrQueryIrq
    (
    struct a2b_StackContext*  ctx
    )
{
    a2b_UInt16          idx;
    a2b_UInt8           intSrc;
    a2b_UInt8           intType;
    a2b_UInt8           regOffset;
    a2b_HResult         ret = A2B_RESULT_SUCCESS;
    a2b_StackContext*   masterCtx;
    struct a2b_Msg*     notifyMsg;
    a2b_Interrupt*      interrupt;
    a2b_Bool            gpioIntrpt = A2B_FALSE;
    a2b_StackContext*   slaveCtx;
    a2b_Bool 			bBreakLoop = A2B_FALSE;
    a2b_UInt8 			nTempVar;

    if ( A2B_NULL == ctx )
    {
        ret = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_INTERRUPT,
                                A2B_EC_INVALID_PARAMETER);
    }
    else
    {
        masterCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        
        /* Process up to A2B_CONF_CONSECUTIVE_INTERRUPTS at once.
         */
        for ( idx = 0u; idx < A2B_CONF_CONSECUTIVE_INTERRUPTS; idx++ )
        {
            /* NOTE: This is tricky. You should only read INTSRC
             * first to see if there is an interrupt pending. Only if there is
             * one pending should you find out which type of the interrupt
             * (INTTYPE). If all both registers are read
             * at once there is a possibility an interrupt could occur *between*
             * reading the two registers (which would not indicate an
             * interrupt) and reading the INTTYPE registers (which will clear
             * the register and thus possibly miss processing the
             * interrupt that arrived in the middle of the two register reads).
             */
            regOffset = A2B_REG_INTSRC;
            ret = a2b_i2cMasterWriteRead( masterCtx, 1u, &regOffset, 1u, &intSrc);
            if ( A2B_FAILED(ret) )
            {
                /* Read failure, move on */
                A2B_TRACE0((ctx, (A2B_TRC_DOM_STACK |
                            A2B_TRC_LVL_ERROR),
                            "a2b_intrQueryIrq: "
                            "failed to read A2B_REG_INTSRC"));
                bBreakLoop = A2B_TRUE;
            }
            else
            {
                /* If neither the master or slave nodes triggered an
                 * interrupt
                 */
                if ( 0u == (intSrc & (A2B_BITM_INTSRC_MSTINT |
                    A2B_BITM_INTSRC_SLVINT)) )
                {
                    /* Else no interrupt detected - move on */
                    bBreakLoop = A2B_TRUE;
                }
                else
                {
                    regOffset = A2B_REG_INTTYPE;
                    ret = a2b_i2cMasterWriteRead( masterCtx, 1u, &regOffset, 1u,
                                                &intType );
                    if ( A2B_FAILED(ret) )
                    {
                        /* Read failure, move on */
                        A2B_TRACE0((ctx, (A2B_TRC_DOM_STACK |
                                    A2B_TRC_LVL_ERROR),
                                    "a2b_intrQueryIrq: "
                                    "failed to read INTTYPE"));
                        bBreakLoop = A2B_TRUE;
                    }
                    else
                    {
                        if ( (intType >= A2B_ENUM_INTTYPE_IO0PND) &&
                            (intType <= A2B_ENUM_INTTYPE_IO6PND) )
                        {
                            gpioIntrpt = A2B_TRUE;
                        }

                        /* Allocate a notification message */
                        notifyMsg = a2b_msgAlloc(ctx,
                                                    A2B_MSG_NOTIFY,
                                                    A2B_MSGNOTIFY_INTERRUPT);

                        if ( A2B_NULL == notifyMsg )
                        {
                            A2B_TRACE0((ctx, (A2B_TRC_DOM_STACK |
                                    A2B_TRC_LVL_ERROR),
                                    "a2b_intrQueryIrq: "
                                    "failed to allocate notification"));
                        }
                        else
                        {
                        	nTempVar = intSrc & (a2b_UInt8)A2B_BITM_INTSRC_INODE;
                            interrupt = (a2b_Interrupt*)
                                            a2b_msgGetPayload(notifyMsg);
                            interrupt->intrType = intType;
                            interrupt->nodeAddr = (intSrc &
                                    A2B_BITM_INTSRC_MSTINT) ?
                                    A2B_NODEADDR_MASTER :
                                    (a2b_Int16)(nTempVar);
                            /* Make best effort delivery of notification */
                            ret = a2b_msgRtrNotify(notifyMsg);
                            if ( A2B_FAILED(ret) )
                            {
                                A2B_TRACE1((ctx,
                                    (A2B_TRC_DOM_STACK | A2B_TRC_LVL_ERROR),
                                    "a2b_intrQueryIrq: failed to emit power "
                                    "interrupt notification: 0x%lX",
                                    &ret));
                            }

                            if ( gpioIntrpt )
                            {
                                ret = a2b_msgSetCmd(notifyMsg,
                                          A2B_MSGNOTIFY_GPIO_INTERRUPT);
                                if ( A2B_FAILED(ret) )
                                {
                                    A2B_TRACE1((ctx,
                                        (A2B_TRC_DOM_STACK | A2B_TRC_LVL_ERROR),
                                        "a2b_intrQueryIrq: failed set GPIO "
                                        "interrupt notify command: 0x%lX",
                                        &ret));
                                }
                                else
                                {
                                    ret = a2b_msgRtrNotify(notifyMsg);
                                    if ( A2B_FAILED(ret) )
                                    {
                                        A2B_TRACE1((ctx,
                                        (A2B_TRC_DOM_STACK | A2B_TRC_LVL_ERROR),
                                        "a2b_intrQueryIrq: failed to emit GPIO "
                                        "interrupt notification: 0x%lX",
                                        &ret));
                                    }
                                }
                            }

                            /* We no longer need this notification message */
                            (void)a2b_msgUnref(notifyMsg);
                        }

                        /* We'll assume there is no available slave plugin */
                        slaveCtx = A2B_NULL;

                        /* See if it was a slave interrupt */
                        if (intSrc & A2B_BITM_INTSRC_SLVINT)
                        {
                            /* Get the plugin context (if it exists) for the
                             * slave node.intrSrc
                             */
                        	nTempVar = intSrc & (a2b_UInt8)A2B_BITM_INTSRC_INODE;
                            slaveCtx = a2b_stackContextFind(masterCtx,
                                             (a2b_Int16)(nTempVar));
                        }


                        /* If there is an associated slave plugin AND it's
                         * a GPIO interrupt (only) then ...
                         */
                        if ((slaveCtx) &&
                            (slaveCtx->ccb.plugin.pluginApi) &&
                            (slaveCtx->ccb.plugin.pluginApi->interrupt) &&
                            gpioIntrpt)
                        {
                            /* Slave plugins handle their own GPIO interrupts.
                             * All other interrupts get directed to the master
                             * plugin.
                             */
                            slaveCtx->ccb.plugin.pluginApi->interrupt(
                                              slaveCtx,
                                              slaveCtx->ccb.plugin.pluginHnd,
                                              intSrc, intType);
                        }
                        /* Else the master plugin receives all the interrupts
                         * that aren't handled by a slave plugin (e.g. slave
                         * plugin only handles GPIO interrupts). This includes
                         * interrupts for a slave node that does not have an
                         * associated slave plugin.
                         */
                        else
                        {
                            /* Always tell the master plugin about
                             * an interrupt
                             */
                            masterCtx->ccb.plugin.pluginApi->interrupt(
                                            masterCtx,
                                            masterCtx->ccb.plugin.pluginHnd,
                                            intSrc, intType);
                        }
                    }
                }
            }

            if(bBreakLoop)
            {
                break;
            }
        }
    }

    return ret;

} /* a2b_intrQueryIrq */


/*!****************************************************************************
*
*  \b              a2b_intrDestroy
*
*  Destroy a stacks interrupt polling.  This will stop any timers and free
*  the resources used by the stack.  Calling #a2b_intrStartIrqPoll will
*  always reallocate resources if needed.
*
*  \param          [in]    stk  A2B stack
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_intrDestroy
    (
    struct a2b_Stack* stk
    )
{
    if (stk)
    {
        if (stk->intrInfo)
        {
            if (stk->intrInfo->timer)
            {
                a2b_timerStop(stk->intrInfo->timer);
            }
            A2B_FREE(stk, stk->intrInfo);
            stk->intrInfo = A2B_NULL;
        }
    }

} /* a2b_intrDestroy */

