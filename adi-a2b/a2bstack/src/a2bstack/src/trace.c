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
 * \file:   trace.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the trace routines.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/tracectl.h"
#include "stackctx.h"
#include "stack_priv.h"
#include "trace_priv.h"
#include "a2b/trace.h"
#include "a2b/util.h"
#include "utilmacros.h"


#ifdef A2B_FEATURE_TRACE

/*======================= D E F I N E S ===========================*/

#define IS_ENABLED(lvl, mask)   (((lvl) & (A2B_TRC_DOM_MASK & (mask))) && \
                                 ((lvl) & (A2B_TRC_LVL_MASK & (mask))))

/*======================= L O C A L  P R O T O T Y P E S  =========*/
static void a2b_traceLevelString(struct a2b_TraceChannel* chan,
    a2b_UInt32 level);
/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!**************************************************************************** 
*  \ingroup        a2bstack_trace_priv
*
*  \b              a2b_traceLevelString
*
*  Writes the trace level (string) to the channel.
*
*  \param          [in]    chan     The A2B trae channel to write to.
* 
*  \param          [in]    level    The level of the trace.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that chan is !NULL, previously checked
*
*  \post           A textual representation of the trace level will be written 
*                  to the trace channel.
*
*  \return         None
*
******************************************************************************/
static void
a2b_traceLevelString
    (
    struct a2b_TraceChannel*   chan,
    a2b_UInt32          level
    )
{
    const a2b_Char* levelStr = A2B_NULL;
    void* args[1];

    if ( IS_ENABLED(level, chan->mask) )
    {
        switch( level & A2B_TRC_LVL_MASK)
        {
            case A2B_TRC_LVL_FATAL:
                levelStr = "FATAL";
                break;
            case A2B_TRC_LVL_ERROR:
                levelStr = "ERROR";
                break;
            case A2B_TRC_LVL_WARN:
                levelStr = "WARN";
                break;
            case A2B_TRC_LVL_INFO:
                levelStr = "INFO";
                break;
            case A2B_TRC_LVL_DEBUG:
                levelStr = "DEBUG";
                break;
            case A2B_TRC_LVL_TRACE1:
                levelStr = "TRACE1";
                break;
            case A2B_TRC_LVL_TRACE2:
                levelStr = "TRACE2";
                break;
            case A2B_TRC_LVL_TRACE3:
                levelStr = "TRACE3";
                break;
            case A2B_TRC_LVL_OFF:
            default:
                break;
        }
    }

    if ( A2B_NULL != levelStr )
    {
        args[0] = (void*)levelStr;
        (void)a2b_vsnprintfStringBuffer(&chan->strBuf, "[%s] ", args,
                                  A2B_ARRAY_SIZE(args));
    }

} /* a2b_traceLevelString */


/*!****************************************************************************
*
*  \b              a2b_traceAlloc
*
*  Allocates a trace channel from the pool of available (unused) channels
*  for the stack. The underlying (platform specific) logging channel is
*  opened as a result of this call.
*
*  \param          [in]    ctx      The A2B stack context to allocate and open 
*                                   a logging/trace channel.
* 
*  \param          [in]    name     Platform specific name to associate with
*                                   the channel.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid: stk
*
*  \post           None
*
*  \return         A pointer to the trace channel or A2B_NULL if one could not
*                  be allocated for the stack or there was an error opening the
*                  underlying logging channel.
*
******************************************************************************/
A2B_DSO_LOCAL struct a2b_TraceChannel*
a2b_traceAlloc
    (
    struct  a2b_StackContext*   ctx,
    const a2b_Char*     name
    )
{
    a2b_TraceChannel* chan = A2B_NULL;

    if ( (A2B_NULL != ctx) && (A2B_NULL != name) )
    {
        chan = A2B_MALLOC(ctx->stk, sizeof(*chan));
        if ( A2B_NULL != chan )
        {
            chan->hnd = ctx->stk->pal.logOpen(name);
            if ( A2B_NULL == chan->hnd )
            {
                A2B_FREE(ctx->stk, chan);
                chan = A2B_NULL;
            }
            else
            {
                chan->ctx  = ctx;
                chan->mask = A2B_TRC_LVL_DEFAULT;
                a2b_stringBufferInit(&chan->strBuf, chan->buf,
                                        A2B_ARRAY_SIZE(chan->buf));
            }
        }
    }
    return chan;

} /* a2b_traceAlloc */


/*!****************************************************************************
*
*  \b              a2b_traceFree
*
*  De-allocates (or frees) the referenced trace channel.
*
*  \param          [in]    chan     The A2B channel to free.
*
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_traceFree
    (
        struct a2b_TraceChannel*   chan
    )
{
    if ( A2B_NULL != chan )
    {
        chan->ctx->stk->pal.logClose(chan->hnd);
		a2b_traceSetMask(chan->ctx, 0);
        A2B_FREE(chan->ctx->stk, chan);
    }
} /* a2b_traceFree */


/*!****************************************************************************
*
*  \b              a2b_traceIsEnabled
*
*  Depending on the configured trace level this function will either
*  return the passed in trace channel (if enabled) or A2B_NULL if the
*  trace is not enabled.
*
*  \param          [in]    ctx      The A2B stack context.
* 
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM_xxx)
*
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
*
*  \post           None
*
*  \return         Returns the passed in stack context if the trace level is
*                  currently enabled by the tracing sub-system or A2B_NULL if
*                  the trace would be disabled by the current settings.
*
******************************************************************************/
A2B_DSO_PUBLIC struct a2b_StackContext*
a2b_traceIsEnabled
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    ...
    )
{
    a2b_TraceChannel* chan = A2B_NULL;
    a2b_Bool enabled = A2B_FALSE;

    if ( A2B_NULL != ctx )
    {
        chan = ctx->stk->traceChan;
    }

    if ( A2B_NULL != chan )
    {
        enabled = (IS_ENABLED(level, chan->mask)) ? A2B_TRUE : A2B_FALSE;
    }
    return enabled ? ctx : A2B_NULL;

} /* a2b_traceIsEnabled */


/*!****************************************************************************
*
*  \b              a2b_tracePrintPrefix
*
*  Outputs a prefix which includes the current time, file name,
*  function (optional), and line number to the trace channel. It is assumed
*  this function is always called prior to the actual trace message.
*
*  \param          [in]    ctx      The A2B stack context.
* 
*  \param          [in]    file     This should represent the file name
*                                   associated with the trace.
* 
*  \param          [in]    funcName This should represent the function name 
*                                   associated with the trace. If none is 
*                                   available then set to A2B_NULL.
* 
*  \param          [in]    line     The line number of the trace.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_tracePrintPrefix
    (
    struct a2b_StackContext*   ctx,
    const a2b_Char*     file,
    const a2b_Char*     funcName,
    a2b_UInt32          line
    )
{
    void* args[4];
    a2b_UInt32 curTime = 0;

    if ( A2B_NULL != ctx )
    {
        const a2b_Char* fileName = a2b_strrchr(file, A2B_CONF_PATH_SEPARATOR);
        if ( A2B_NULL != fileName )
        {
            /* Point one past the path separator */
            fileName++;
        }
        else
        {
            fileName = file;
        }
        curTime = ctx->stk->pal.timerGetSysTime();

        if ( A2B_NULL != funcName )
        {
            args[0] = &curTime;
            args[1] = (void*)fileName;
            args[2] = (void*)funcName;
            args[3] = &line;
            (void)a2b_vsnprintfStringBuffer(&ctx->stk->traceChan->strBuf,
                                      "%lu %s:%s(%lu) ",
                                      args, 4);
        }
        else
        {
            args[0] = &curTime;
            args[1] = (void*)fileName;
            args[2] = &line;
            (void)a2b_vsnprintfStringBuffer(&ctx->stk->traceChan->strBuf,
                                      "%010lu %s(%lu) ",
                                      args, 3);
        }
    }
} /* a2b_tracePrintPrefix */


/*!****************************************************************************
*
*  \b              a2b_traceSetMask
*
*  Sets the trace mask for the A2B stack. The mask is a bitwise "OR" of
*  the following macros: A2B_TRC_LVL_* and/or A2B_TRC_DOM_*
*
*  \param          [in]    ctx      The A2B stack context.
* 
*  \param          [in]    mask     The new trace mask.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_traceSetMask
    (
    struct a2b_StackContext*    ctx,
    a2b_UInt32                  mask
    )
{
    if ( A2B_NULL != ctx )
    {
        ctx->stk->traceChan->mask = mask;
    }
} /* a2b_traceSetMask */


/*!****************************************************************************
*
*  \b              a2b_traceGetMask
*
*  Returns the current trace mask for the A2B stack.
*
*  \param          [in]    ctx      The A2B stack context.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         The trace mask for the stack.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_traceGetMask
    (
    struct a2b_StackContext*   ctx
    )
{
    a2b_UInt32 mask = 0;

    if ( A2B_NULL != ctx )
    {
        mask = ctx->stk->traceChan->mask;
    }

    return mask;
} /* a2b_traceGetMask */


/*!****************************************************************************
*
*  \b              a2b_traceInject
*
*  Injects user defined text into the trace log.
*
*  \param          [in]    ctx      The A2B stack context.                       
*                                                                                
*  \param          [in]    level    The trace level.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM_xxx) 
*                                                                                
*  \param          [in]    text     The user defined text to inject into
*                                   the log.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_traceInject
    (
    struct a2b_StackContext*    ctx,
    a2b_UInt32                  level,
    const a2b_Char*             text
    )
{
    if ( A2B_NULL != ctx )
    {
        if ( IS_ENABLED(level, ctx->stk->traceChan->mask) )
        {
            a2b_tracePrintPrefix(ctx, "user", A2B_NULL, 0);
            a2b_trace0(ctx, level, text);
        }
    }
} /* a2b_traceInject */


/*!****************************************************************************
*
*  \b              a2b_trace0
*
*  Outputs a formatted trace message with zero (0) parameters specified
*  in the format string.
*
*  \param          [in]    ctx      The A2B stack context.                                  
*                                                                                           
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM)
*                                                                                           
*  \param          [in]    fmt      The trace format string.                                
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_trace0
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    const a2b_Char*     fmt
    )
{
    if ( A2B_NULL != ctx )
    {
        struct a2b_TraceChannel* chan = ctx->stk->traceChan;

        if ( IS_ENABLED(level, chan->mask) )
        {
            a2b_traceLevelString(chan, level);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, A2B_NULL, 0);
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);
        }
    }
} /* a2b_trace0 */


/*!****************************************************************************
*
*  \b              a2b_trace1
*
*  Outputs a formatted trace message with one (1) parameter specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter. 
* 
*  \param          [in]    ctx      The A2B stack context.                                  
*                                                                                           
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM)
*                                                                                           
*  \param          [in]    fmt      The trace format string.
* 
*  \param          [in]    a1       First format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_trace1
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    const a2b_Char*     fmt,
    void*               a1
    )
{
    void* args[1u];

    args[0u] = a1;

    if ( A2B_NULL != ctx )
    {
        struct a2b_TraceChannel* chan = ctx->stk->traceChan;

        if ( IS_ENABLED(level, chan->mask) )
        {
            a2b_traceLevelString(chan, level);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);
        }
    }
} /* a2b_trace1 */


/*!****************************************************************************
*
*  \b              a2b_trace2
*
*  Outputs a formatted trace message with two (2) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx      The A2B stack context.                                  
*                                                                                           
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM)
*                                                                                           
*  \param          [in]    fmt      The trace format string.
* 
*  \param          [in]    a1       First format parameter.
* 
*  \param          [in]    a2       Second format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_trace2
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    const a2b_Char*     fmt,
    void*               a1,
    void*               a2
    )
{
    void* args[2u];

    args[0u] = a1;
    args[1u] = a2;

    if ( A2B_NULL != ctx )
    {
        struct a2b_TraceChannel* chan = ctx->stk->traceChan;

        if ( IS_ENABLED(level, chan->mask) )
        {
            a2b_traceLevelString(chan, level);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);
        }
    }
} /* a2b_trace2 */


/*!****************************************************************************
*
*  \b              a2b_trace3
*
*  Outputs a formatted trace message with three (3) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx      The A2B stack context.                                  
*                                                                                           
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM)
*                                                                                           
*  \param          [in]    fmt      The trace format string.
* 
*  \param          [in]    a1       First format parameter.
* 
*  \param          [in]    a2       Second format parameter.
* 
*  \param          [in]    a3       Third format parameter.
*
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_trace3
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    const a2b_Char*     fmt,
    void*               a1,
    void*               a2,
    void*               a3
    )
{
    void* args[3u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;

    if ( A2B_NULL != ctx )
    {
        struct a2b_TraceChannel* chan = ctx->stk->traceChan;

        if ( IS_ENABLED(level, chan->mask) )
        {
            a2b_traceLevelString(chan, level);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);
        }
    }
} /* a2b_trace3 */


/*!****************************************************************************
*
*  \b              a2b_trace4
*
*  Outputs a formatted trace message with four (4) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx      The A2B stack context.                                  
*                                                                                           
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM)
*                                                                                           
*  \param          [in]    fmt      The trace format string.
* 
*  \param          [in]    a1       First format parameter.
* 
*  \param          [in]    a2       Second format parameter.
* 
*  \param          [in]    a3       Third format parameter.
* 
*  \param          [in]    a4       Fourth format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_trace4
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    const a2b_Char*     fmt,
    void*               a1,
    void*               a2,
    void*               a3,
    void*               a4
    )
{
    void* args[4u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;
    args[3u] = a4;

    if ( A2B_NULL != ctx )
    {
        struct a2b_TraceChannel* chan = ctx->stk->traceChan;

        if ( IS_ENABLED(level, chan->mask) )
        {
            a2b_traceLevelString(chan, level);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);
        }
    }
} /* a2b_trace4 */


/*!****************************************************************************
*
*  \b              a2b_trace5
*
*  Outputs a formatted trace message with five (5) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx      The A2B stack context.                                  
*                                                                                           
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM)
*                                                                                           
*  \param          [in]    fmt      The trace format string.
* 
*  \param          [in]    a1       First format parameter.
* 
*  \param          [in]    a2       Second format parameter.
* 
*  \param          [in]    a3       Third format parameter.
* 
*  \param          [in]    a4       Fourth format parameter.
* 
*  \param          [in]    a5       Fifth format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_trace5
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    const a2b_Char*     fmt,
    void*               a1,
    void*               a2,
    void*               a3,
    void*               a4,
    void*               a5
    )
{
    void* args[5u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;
    args[3u] = a4;
    args[4u] = a5;

    if ( A2B_NULL != ctx )
    {
        struct a2b_TraceChannel* chan = ctx->stk->traceChan;

        if ( IS_ENABLED(level, chan->mask) )
        {
            a2b_traceLevelString(chan, level);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);
        }
    }
} /* a2b_trace5 */


/*!****************************************************************************
*
*  \b              a2b_trace6
*
*  Outputs a formatted trace message with six (6) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx      The A2B stack context.                                  
*                                                                                           
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM)
*                                                                                           
*  \param          [in]    fmt      The trace format string.
* 
*  \param          [in]    a1       First format parameter.
* 
*  \param          [in]    a2       Second format parameter.
* 
*  \param          [in]    a3       Third format parameter.
* 
*  \param          [in]    a4       Fourth format parameter.
* 
*  \param          [in]    a5       Fifth format parameter.
* 
*  \param          [in]    a6       Sixth format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_trace6
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    const a2b_Char*     fmt,
    void*               a1,
    void*               a2,
    void*               a3,
    void*               a4,
    void*               a5,
    void*               a6
    )
{
    void* args[6u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;
    args[3u] = a4;
    args[4u] = a5;
    args[5u] = a6;

    if ( A2B_NULL != ctx )
    {
        struct a2b_TraceChannel* chan = ctx->stk->traceChan;

        if ( IS_ENABLED(level, chan->mask) )
        {
            a2b_traceLevelString(chan, level);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);
        }
    }
} /* a2b_trace6 */


/*!****************************************************************************
*
*  \b              a2b_trace7
*
*  Outputs a formatted trace message with seven (7) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx      The A2B stack context.                                  
*                                                                                           
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM)
*                                                                                           
*  \param          [in]    fmt      The trace format string.
* 
*  \param          [in]    a1       First format parameter.
* 
*  \param          [in]    a2       Second format parameter.
* 
*  \param          [in]    a3       Third format parameter.
* 
*  \param          [in]    a4       Fourth format parameter.
* 
*  \param          [in]    a5       Fifth format parameter.
* 
*  \param          [in]    a6       Sixth format parameter.
* 
*  \param          [in]    a7       Seventh format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_trace7
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    const a2b_Char*     fmt,
    void*               a1,
    void*               a2,
    void*               a3,
    void*               a4,
    void*               a5,
    void*               a6,
    void*               a7
    )
{
    void* args[7u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;
    args[3u] = a4;
    args[4u] = a5;
    args[5u] = a6;
    args[6u] = a7;

    if ( A2B_NULL != ctx )
    {
        struct a2b_TraceChannel* chan = ctx->stk->traceChan;

        if ( IS_ENABLED(level, chan->mask) )
        {
            a2b_traceLevelString(chan, level);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);
        }
    }
} /* a2b_trace7 */


/*!****************************************************************************
*
*  \b              a2b_trace8
*
*  Outputs a formatted trace message with eight (8) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx      The A2B stack context.                                  
*                                                                                           
*  \param          [in]    level    The A2B level for the trace.
*                                   (A2B_TRC_LVL_xxx | A2B_TRC_DOM)
*                                                                                           
*  \param          [in]    fmt      The trace format string.
* 
*  \param          [in]    a1       First format parameter.
* 
*  \param          [in]    a2       Second format parameter.
* 
*  \param          [in]    a3       Third format parameter.
* 
*  \param          [in]    a4       Fourth format parameter.
* 
*  \param          [in]    a5       Fifth format parameter.
* 
*  \param          [in]    a6       Sixth format parameter.
* 
*  \param          [in]    a7       Seventh format parameter.
* 
*  \param          [in]    a8       Eighth format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_TRACE is enabled.
* 
*  \pre            Expectation is that the ctx contains a valid:
*                  stk and stk->traceChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_trace8
    (
    struct a2b_StackContext*   ctx,
    a2b_UInt32          level,
    const a2b_Char*     fmt,
    void*               a1,
    void*               a2,
    void*               a3,
    void*               a4,
    void*               a5,
    void*               a6,
    void*               a7,
    void*               a8
    )
{
    void* args[8u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;
    args[3u] = a4;
    args[4u] = a5;
    args[5u] = a6;
    args[6u] = a7;
    args[7u] = a8;

    if ( A2B_NULL != ctx )
    {
        struct a2b_TraceChannel* chan = ctx->stk->traceChan;

        if ( IS_ENABLED(level, chan->mask) )
        {
            a2b_traceLevelString(chan, level);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);
        }
    }
} /* a2b_trace8 */

#endif /* A2B_FEATURE_TRACE */
