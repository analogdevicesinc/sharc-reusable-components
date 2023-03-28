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
 * \file:   timer.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the timer services.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/timer.h"
#include "a2b/error.h"
#include "timer_priv.h"
#include "stack_priv.h"
#include "stackctx.h"
#include "a2b/trace.h"
#include "a2b/seqchart.h"
#include "utilmacros.h"

/*======================= D E F I N E S ===========================*/

/** Flag used to indicate whether or not a timer is in use (allocated) */
#define A2B_TIMER_STATUS_INUSE     ((a2b_UInt32)1u << (a2b_UInt32)0u)

/** Flag used to indicate that a timer is active or not */
#define A2B_TIMER_STATUS_ACTIVE    ((a2b_UInt32)1u << (a2b_UInt32)1u)

/*======================= L O C A L  P R O T O T Y P E S  =========*/
static void a2b_timerInit(struct a2b_StackContext*    ctx,
    struct a2b_Timer* timer, a2b_TimerFunc onTimeout,
    a2b_Handle userData);

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
* 
*  \b   a2b_timerInit
* 
*  Initializes (e.g. constructs) a timer instance.
* 
*  \param   [in]    ctx         The parent stack context owning the timer.
* 
*  \param   [in]    timer       The timer to initialize.
* 
*  \param   [in]    onTimeout   The timer callback function.
* 
*  \param   [in]    userData    Pointer to the opaque user data.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
static void
a2b_timerInit
    (
    struct a2b_StackContext*    ctx,
    struct a2b_Timer*           timer,
    a2b_TimerFunc               onTimeout,
    a2b_Handle                  userData
    )
{
    if ( (A2B_NULL != ctx) && (A2B_NULL != timer) )
    {
        timer->after = (a2b_UInt32)0;
        timer->repeat = (a2b_UInt32)0;
        timer->lastTime = (a2b_UInt32)0;
        timer->status &= (a2b_UInt8)~(A2B_TIMER_STATUS_ACTIVE);
        timer->userData = userData;
        timer->expireFunc = onTimeout;
        timer->ctx = ctx;
        timer->refCnt = (a2b_UInt16)1;
        timer->status |= (a2b_UInt8)A2B_TIMER_STATUS_INUSE;

        /* Link in the timer at the head of the list */
        SLIST_INSERT_HEAD(&ctx->stk->timerList, timer, link);
    }

} /* a2b_timerInit */


/*!****************************************************************************
* 
*  \b   a2b_timerTick
* 
*  The primary timer services driver. Called as a by-product of the
*  `a2b_stackTick()` function. Scans through the list of allocated timers
*  for this stack and does the necessary processing on each timer including
*  expiring timers and invoking necessary timeout callback functions. This
*  is the "engine" that drives all the timers.
* 
*  \param   [in]    ctx     The parent A2B stack context.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
A2B_DSO_LOCAL void
a2b_timerTick
    (
    struct a2b_StackContext*   ctx
    )
{
    a2b_Timer* timer;
    a2b_Timer* next;
    a2b_UInt32 nTimerRemaining = 0u;
    a2b_Bool bTimterActive = A2B_FALSE;

    if ( A2B_NULL != ctx )
    {
        next = SLIST_FIRST(&ctx->stk->timerList);
        while ( next != SLIST_END(&ctx->stk->timerList) )
        {
            timer = next;

            /* Reference the timer in case it's unreferenced in the
             * timer callback.
             */
            a2b_timerRef(timer);

            bTimterActive = a2b_timerIsActive(timer);
            if ( bTimterActive )
            {
                nTimerRemaining = a2b_timerRemaining(timer);
            }

            if ( (bTimterActive) && (nTimerRemaining == 0u) )
            {
                if ( A2B_NULL != timer->expireFunc )
                {
                    timer->expireFunc(timer, timer->userData);
                }

                /* See if the timer is still active and no time remains
                 * (e.g. perhaps the user did not restart it)
                 */
                bTimterActive = a2b_timerIsActive(timer);
                if ( bTimterActive )
                {
                    nTimerRemaining = a2b_timerRemaining(timer);
                }
                if ( (bTimterActive) && (nTimerRemaining == 0u) )
                {
                    /* If it's not configured to repeat again then ... */
                    if ( timer->repeat == (a2b_UInt32)0 )
                    {
                        /* Make sure it's stopped */
                        a2b_timerStop(timer);
                    }
                    /* Else restart the timer using the repeat interval */
                    else
                    {
                        a2b_timerSet(timer, timer->repeat, timer->repeat);
                        a2b_timerStart(timer);
                    }
                }
            }

            next = SLIST_NEXT(timer, link);
            (void)a2b_timerUnref(timer);
        }
    }

} /* a2b_timerTick */


/*!****************************************************************************
* 
*  \b   a2b_timerAlloc
* 
*  Allocates an available timer instance for the stack. If no timer is
*  available then A2B_NULL is returned. An allocated timer is returned with
*  a reference count of one (1). To "free" the timer it should be
*  unreferenced (`a2b_timerUnref()`).
* 
*  \param   [in]    ctx         The parent stack context to associate the
*                               timer with.
* 
*  \param   [in]    onTimeout   A user supplied timer callback function. May be
*                               A2B_NULL if no callback is required.
* 
*  \param   [in]    userData    Pointer to the opaque user data. Passed back in
*                               the timer callback.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  Returns an allocated timer or A2B_NULL if no timer could be
*           allocated or there was an error initializing the timer. An
*           allocated stack is returned with its reference count set to
*           one (1).
* 
******************************************************************************/
A2B_DSO_PUBLIC struct a2b_Timer*
a2b_timerAlloc
    (
    struct a2b_StackContext*    ctx,
    a2b_TimerFunc               onTimeout,
    a2b_Handle                  userData
    )
{
    a2b_Timer* timer = A2B_NULL;

    if ( A2B_NULL != ctx )
    {
        A2B_SEQ_CHART3((ctx,
                        ((ctx->domain == A2B_DOMAIN_APP) ?
                            A2B_SEQ_CHART_ENTITY_APP :
                            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerAlloc(0x%p,0x%p,0x%p)",
                       (void*)ctx, (void*)onTimeout, (void*)userData));
        A2B_TRACE3((ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerAlloc(0x%p,0x%p,0x%p)",
                    (void*)ctx, (void*)onTimeout, (void*)userData));

        timer = A2B_MALLOC(ctx->stk, sizeof(*timer));
        if ( A2B_NULL != timer )
        {
            a2b_timerInit(ctx, timer, onTimeout, userData);
        }

        A2B_SEQ_CHART1((ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((ctx->domain == A2B_DOMAIN_APP) ?
                           A2B_SEQ_CHART_ENTITY_APP :
                           A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                               ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerAlloc(0x%p)", timer));
        A2B_TRACE1((ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerAlloc(0x%p)", timer));
    }
    return timer;

} /* a2b_timerAlloc */


/*!****************************************************************************
* 
*  \b   a2b_timerRef
* 
*  Increment the reference count of the timer.
* 
*  \param   [in]    timer   The timer to increment the reference count.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_timerRef
    (
    struct a2b_Timer*   timer
    )
{
    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART1((timer->ctx,
                        ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                            A2B_SEQ_CHART_ENTITY_APP :
                            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerRef(0x%p)",
                       timer));
        A2B_TRACE2((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerRef(0x%p) : RefCnt=%bd", timer,
                    &timer->refCnt));

        timer->refCnt++;

        A2B_SEQ_CHART1((timer->ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                           A2B_SEQ_CHART_ENTITY_APP :
                           A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                               timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerRef(0x%p)", timer));
        A2B_TRACE2((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerRef(0x%p) : RefCnt=%bd",
                    timer, &timer->refCnt));
    }

} /* a2b_timerRef */


/*!****************************************************************************
* 
*  \b   a2b_timerUnref
* 
*  Decrement the reference count of the timer. When the reference count
*  reaches zero (0) the timer is de-initialized and deallocated and
*  returned to the pool of available timers.
* 
*  \param   [in]    timer   The timer to decrement the reference count.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  The remaining reference count.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt16
a2b_timerUnref
    (
    struct a2b_Timer*   timer
    )
{
    a2b_UInt16 refCount = 0u;

    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART1((timer->ctx,
                        ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                            A2B_SEQ_CHART_ENTITY_APP :
                            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerUnref(0x%p)",
                       timer));
        A2B_TRACE2((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerUnref(0x%p) : RefCnt=%bd", timer,
                    &timer->refCnt));

        if ( timer->refCnt == (a2b_UInt16)0 )
        {
            A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_ERROR),
                        "Timer (0x%p) is already unreferenced", timer));
        }
        else
        {
            timer->refCnt--;
            refCount = timer->refCnt;

            A2B_SEQ_CHART1((timer->ctx,
                           A2B_SEQ_CHART_ENTITY_STACK,
                           ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                               A2B_SEQ_CHART_ENTITY_APP :
                               A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                   timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                           A2B_SEQ_CHART_COMM_REPLY,
                           A2B_SEQ_CHART_LEVEL_6,
                           "a2b_timerUnref(0x%p)", timer));
            A2B_TRACE2((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                        "Exit: a2b_timerUnref(0x%p) : RefCnt=%bd",
                        timer, &timer->refCnt));

            if (timer->refCnt == (a2b_UInt16)0 )
            {
                /* Unlink the driver from the link list of timers */
                SLIST_REMOVE(&timer->ctx->stk->timerList, timer,
                                a2b_Timer, link);

                /* Free the timer */
                A2B_FREE(timer->ctx->stk, timer);
            }
        }
    }

    return refCount;

} /* a2b_timerUnref */


/*!****************************************************************************
* 
*  \b   a2b_timerSet
* 
*  Initializes the timer with an expiration time (`after`) and a repeat
*  interval (`repeat`). All times are in milliseconds. If the timer is
*  currently active then it will be stopped and restarted. If `repeat` is
*  zero then the timeout will *not* be reloaded. After the first timeout
*  (`after` msec) subsequent timeouts occur at the `repeat` interval.
* 
*  \param   [in]    timer   The timer to set the current and repeating timeouts.
* 
*  \param   [in]    after   The number of milliseconds after the timer is
*                           initially started until a timeout event.
* 
*  \param   [in]    repeat  The timeout interval (in milliseconds) the timer
*                           will be reloaded with after the first timeout. If
*                           set to zero (0) the timer will *not* automatically
*                           be restarted.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_timerSet
    (
    struct a2b_Timer*   timer,
    a2b_UInt32          after,
    a2b_UInt32          repeat
    )
{
    a2b_Bool isActive;
    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART3((timer->ctx,
                        ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                            A2B_SEQ_CHART_ENTITY_APP :
                            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerSet(0x%p,%ld,%ld)",
                       timer, &after, &repeat));
        A2B_TRACE3((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerSet(0x%p,%ld,%ld)", timer,
                    &after, &repeat));

        /* See if the timer is already active */
        isActive = a2b_timerIsActive(timer);
        if ( isActive )
        {
            a2b_timerStop(timer);
        }

        timer->after = after;
        timer->repeat = repeat;

        /* If the timer was active when initially called then ... */
        if ( isActive )
        {
            a2b_timerStart(timer);
        }

        A2B_SEQ_CHART1((timer->ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                           A2B_SEQ_CHART_ENTITY_APP :
                           A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                               timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerSet(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerSet(0x%p)", timer));
    }

} /* a2b_timerSet */


/*!****************************************************************************
* 
*  \b   a2b_timerSetHandler
* 
*  Modifies the function that is called when a timeout expires.
* 
*  \param   [in]    timer       The timer to modify the current timeout handler.
* 
*  \param   [in]    onTimeout   The new timeout handling function.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_timerSetHandler
    (
    struct a2b_Timer*   timer,
    a2b_TimerFunc       onTimeout
    )
{
    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART2((timer->ctx,
                        ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                            A2B_SEQ_CHART_ENTITY_APP :
                            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerSetHandler(0x%p,0x%p)",
                       (void*)timer, (void*)onTimeout));
        A2B_TRACE2((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerSetHandler(0x%p,0x%p)", (void*)timer,
                    (void*)onTimeout));

        timer->expireFunc = onTimeout;

        A2B_SEQ_CHART1((timer->ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                           A2B_SEQ_CHART_ENTITY_APP :
                           A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                               timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerSetHandler(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerSetHandler(0x%p)", timer));
    }

} /* a2b_timerSetHandler */


/*!****************************************************************************
* 
*  \b   a2b_timerSetData
* 
*  Modifies the timer's user data.
* 
*  \param   [in]    timer       The timer to modify the current timeout handler.
* 
*  \param   [in]    userData    The new user data to set.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_timerSetData
    (
    struct a2b_Timer*   timer,
    a2b_Handle          userData
    )
{
    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART2((timer->ctx,
                        ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                            A2B_SEQ_CHART_ENTITY_APP :
                            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerSetData(0x%p,0x%p)",
                       timer, userData));
        A2B_TRACE2((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerSetData(0x%p,0x%p)", timer, userData));

        timer->userData = userData;

        A2B_SEQ_CHART1((timer->ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                           A2B_SEQ_CHART_ENTITY_APP :
                           A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                               timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerSetData(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerSetData(0x%p)", timer));
    }

} /* a2b_timerSetData */


/*!****************************************************************************
* 
*  \b   a2b_timerRepeat
* 
*  Loads the timer with the `repeat` interval (see #a2b_timerSet()) and
*  re-starts it.
* 
*  \param   [in]    timer   The timer to repeat.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_timerRepeat
    (
    struct a2b_Timer*   timer
    )
{
    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART1((timer->ctx,
                        ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                            A2B_SEQ_CHART_ENTITY_APP :
                            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerRepeat(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerRepeat(0x%p)", timer));

        /* Force it active so timerSet() will res-start it */
        timer->status |= (a2b_UInt8)A2B_TIMER_STATUS_ACTIVE;
        a2b_timerSet(timer, timer->repeat, timer->repeat);

        A2B_SEQ_CHART1((timer->ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                           A2B_SEQ_CHART_ENTITY_APP :
                           A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                               timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerRepeat(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerRepeat(0x%p)", timer));
    }

} /* a2b_timerRepeat */


/*!****************************************************************************
* 
*  \b   a2b_timerRemaining
* 
*  The amount of time (in msec) remaining on the active timer. If the
*  timer is *not* active then the `after` value is returned.
* 
*  \param   [in]    timer   The timer to return the time remaining.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  The time remaining (in msec) before the timer expires. If the
*           timer is not active then the current `after` value is returned.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_timerRemaining
    (
    struct a2b_Timer*   timer
    )
{
    a2b_UInt32 remaining = 0u;

    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART1((timer->ctx,
                        ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                            A2B_SEQ_CHART_ENTITY_APP :
                            A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                                timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerRemaining(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerRemaining(0x%p)", timer));

        if ( !a2b_timerIsActive(timer) )
        {
            remaining = timer->after;
        }
        else
        {
            remaining = timer->ctx->stk->pal.timerGetSysTime() - 
                        timer->lastTime;

            if ( remaining > timer->after )
            {
                remaining = 0u;
            }
            else
            {
                remaining = timer->after - remaining;
            }
        }

        A2B_SEQ_CHART2((timer->ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                           A2B_SEQ_CHART_ENTITY_APP :
                           A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                               timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerRemaining(0x%p,%ld)", timer,
                       &remaining));
        A2B_TRACE2((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerRemaining(0x%p,%ld)",
                    timer, &remaining));
    }

    return remaining;

} /* a2b_timerRemaining */


/*!****************************************************************************
* 
*  \b   a2b_timerStart
* 
*  Starts the timer running. If it was already running it is first stopped
*  before being reset and re-started.
* 
*  \param   [in]    timer   The timer to start.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_timerStart
    (
    struct a2b_Timer*   timer
    )
{
    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART1((timer->ctx,
                        ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                        A2B_SEQ_CHART_ENTITY_APP :
                        A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                            timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerStart(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerStart(0x%p)", timer));

        if ( a2b_timerIsActive(timer) )
        {
            a2b_timerStop(timer);
        }
        timer->lastTime = timer->ctx->stk->pal.timerGetSysTime();
        timer->status |= (a2b_UInt8)A2B_TIMER_STATUS_ACTIVE;

        A2B_SEQ_CHART1((timer->ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                           A2B_SEQ_CHART_ENTITY_APP :
                           A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                               timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerStart(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerStart(0x%p)", timer));
    }

} /* a2b_timerStart */


/*!****************************************************************************
* 
*  \b   a2b_timerStop
* 
*  Stops the timer. The timer becomes inactive.
* 
*  \param   [in]    timer   The timer to stop.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  None
* 
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_timerStop
    (
    struct a2b_Timer*   timer
    )
{
    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART1((timer->ctx,
                        ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                           A2B_SEQ_CHART_ENTITY_APP :
                           A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                               timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerStop(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerStop(0x%p)", timer));

        timer->status &= (a2b_UInt8)~A2B_TIMER_STATUS_ACTIVE;
        timer->lastTime = 0u;

        A2B_SEQ_CHART1((timer->ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                          A2B_SEQ_CHART_ENTITY_APP :
                          A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                              timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerStop(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerStop(0x%p)", timer));
    }

} /* a2b_timerStop */


/*!****************************************************************************
*  
*  \b   a2b_timerIsActive
*  
*  Returns whether or not the timer is running (active) or stopped.
*  
*  \param   [in]    timer   The timer to check to see if it's active.
*  
*  \pre     None
*  
*  \post    None
*  
*  \return  Returns A2B_TRUE if the timer is active or A2B_FALSE if it
*           timed out or is stopped (or never started).
*  
******************************************************************************/
A2B_DSO_PUBLIC a2b_Bool
a2b_timerIsActive
    (
    struct a2b_Timer*   timer
    )
{
    a2b_Bool isActive = A2B_FALSE;

    if ( A2B_NULL != timer )
    {
        A2B_SEQ_CHART1((timer->ctx,
                ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                       A2B_SEQ_CHART_ENTITY_APP :
                       A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                           timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_ENTITY_STACK,
                       A2B_SEQ_CHART_COMM_REQUEST,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerIsActive(0x%p)", timer));
        A2B_TRACE1((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_timerIsActive(0x%p)", timer));

        isActive = (timer->status & A2B_TIMER_STATUS_ACTIVE) ?
                    A2B_TRUE : A2B_FALSE;

        A2B_SEQ_CHART2((timer->ctx,
                       A2B_SEQ_CHART_ENTITY_STACK,
                       ((timer->ctx->domain == A2B_DOMAIN_APP) ?
                          A2B_SEQ_CHART_ENTITY_APP :
                          A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                              timer->ctx->ccb.plugin.nodeSig.nodeAddr)),
                       A2B_SEQ_CHART_COMM_REPLY,
                       A2B_SEQ_CHART_LEVEL_6,
                       "a2b_timerIsActive(0x%p,%s)", timer,
                       isActive ? "true" : "false"));
        A2B_TRACE2((timer->ctx, (A2B_TRC_DOM_TIMERS | A2B_TRC_LVL_TRACE1),
                    "Exit: a2b_timerIsActive(0x%p,%s)", timer,
                    isActive ? "true" : "false"));
    }

    return isActive;

} /* a2b_timerIsActive */

/*!****************************************************************************
*
*  \b   a2b_ActiveDelay
*
*  Provide a blocking delay using the timer functions
*
*  \param   [in]    nTime   The delay time in mSec
*
*  \pre     None
*
*  \post    None
*
*  \return  None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_ActiveDelay
    (
        struct a2b_StackContext* ctx,
		a2b_UInt32 nTime
    )
{
	a2b_UInt32 nStartTime, nCurrTime;

	nCurrTime = ctx->stk->pal.timerGetSysTime();
	nStartTime = nCurrTime;

	while(nTime > (nCurrTime - nStartTime))
	{
		nCurrTime = ctx->stk->pal.timerGetSysTime();
	}
}
