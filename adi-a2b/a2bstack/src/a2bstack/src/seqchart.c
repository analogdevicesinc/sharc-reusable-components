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
 * \file:   seqchart.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the sequence chart trace routines.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/stack.h"
#include "a2b/error.h"
#include "a2b/seqchartctl.h"
#include "stack_priv.h"
#include "a2b/seqchart.h"
#include "a2b/util.h"
#include "stackctx.h"

#ifdef A2B_FEATURE_SEQ_CHART

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/

static void a2b_seqChartLog0(struct a2b_StackContext* ctx,
    const a2b_Char*             fmt);
static void a2b_seqChartLog1(struct a2b_StackContext* ctx,
    const a2b_Char*  fmt, void* arg1);
static void a2b_seqChartLogTime(struct a2b_StackContext* ctx);
static void a2b_seqChartHeader(struct a2b_StackContext* ctx,
    const a2b_Char*             title);
static void a2b_seqChartFooter(struct a2b_StackContext* ctx);
static void a2b_seqChartPrefix(struct a2b_StackContext* ctx,
    a2b_SeqChartEntity src, a2b_SeqChartEntity dest,
    a2b_SeqChartCommType commType);

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
*  \ingroup        a2bstack_seqchart_priv
* 
*  \b              a2b_seqChartLog0
*
*  Logs a format string to the sequence chart channel that has no arguments.
*  No prefix is output prior to the string.
*
*  \param          [in]    ctx      The A2B stack context.
* 
*  \param          [in]    fmt      The format string to log.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
*
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_seqChartLog0
    (
    struct a2b_StackContext*    ctx,
    const a2b_Char*             fmt
    )
{
    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, A2B_NULL, 0);
        ctx->stk->pal.logWrite(chan->hnd, chan->buf);
        a2b_stringBufferClear(&chan->strBuf);
    }
} /* a2b_seqChartLog0 */


/*!****************************************************************************
*  \ingroup        a2bstack_seqchart_priv
* 
*  \b              a2b_seqChartLog1
*
*  Logs a format string to the sequence chart channel with a single argument.
*  No prefix is output prior to the string.
*
*  \param          [in]    ctx      The A2B stack context.
* 
*  \param          [in]    fmt      The format string to log.
* 
*  \param          [in]    arg1     The first argument of the format string.
*
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
*
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_seqChartLog1
    (
    struct a2b_StackContext*    ctx,
    const a2b_Char*             fmt,
    void*                       arg1
    )
{
    void* args[1u];

    args[0u] = arg1;

    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                    A2B_ARRAY_SIZE(args));
        ctx->stk->pal.logWrite(chan->hnd, chan->buf);
        a2b_stringBufferClear(&chan->strBuf);
    }
} /* a2b_seqChartLog1 */


/*!****************************************************************************
*  \ingroup        a2bstack_seqchart_priv
* 
*  \b              a2b_seqChartLogTime
*
*  Logs the current time as a sequence chart "note".
*
*  \param          [in]    ctx      The A2B stack context.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            A sequence chart event has just been logged and this "note"
*                  follows immediately as a time stamp for the event.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_seqChartLogTime
    (
    struct a2b_StackContext* ctx
    )
{
    a2b_UInt32 curTime;
    if ( A2B_NULL != ctx )
    {
        curTime = ctx->stk->pal.timerGetSysTime();
        a2b_seqChartLog1(ctx, "note left: TS=%010u", &curTime);
    }
} /* a2b_seqChartLogTime */


/*!****************************************************************************
*  \ingroup        a2bstack_seqchart_priv
* 
*  \b              a2b_seqChartHeader
*
*  Outputs a configuration header for a sequence chart.
*
*  \param          [in]    ctx      The A2B stack context.
* 
*  \param          [in]    title    An optional title to assign to the sequence
*                                   chart.  If A2B_NULL then no title is given
*                                   to the sequence chart.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            The sequence chart has been allocated but a configuration 
*                  header for the chart has *not* been written yet.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           The sequence chart configuration header is written.
*
*  \return         None
*
******************************************************************************/
static void
a2b_seqChartHeader
    (
    struct a2b_StackContext*    ctx,
    const a2b_Char*             title
    )
{
    /** Plant UML sequence chart header */
    static const a2b_Char* gsSeqChartHeader[] = {
        "@startuml",
        "skinparam backgroundColor #EEEBDC",
        "skinparam sequence {",
            "ArrowColor DeepSkyBlue",
            "ActorBorderColor DeepSkyBlue",
            "LifeLineBorderColor blue",
            "LifeLineBackgroundColor #A9DCDF",
            "ParticipantBorderColor DeepSkyBlue",
            "ParticipantBackgroundColor DodgerBlue",
            "ParticipantFontName Impact",
            "ParticipantFontSize 17",
            "ParticipantFontColor #black",
            "ActorBackgroundColor aqua",
            "ActorFontColor DeepSkyBlue",
            "ActorFontSize 17",
            "ActorFontName Aapex",
            "TitleFontSize 24",
        "}",
        "participant Application #90EE90",
        "participant Stack #ADD8E6",
        "participant Platform #gray",
        "participant PluginM #FFDB00"
    };
    a2b_UInt16 idx;

    if ( A2B_NULL != ctx )
    {
        for ( idx = 0; idx < A2B_ARRAY_SIZE(gsSeqChartHeader); ++idx )
        {
            a2b_seqChartLog0(ctx, gsSeqChartHeader[idx]);
        }

        if ( ctx->stk->seqChartChan->options & A2B_SEQ_CHART_OPT_AUTONUMBER )
        {
            a2b_seqChartLog0(ctx, "autonumber \"<b>[000]\"");
        }

        if ( A2B_NULL != title )
        {
            a2b_seqChartLog1(ctx, "title %s\n", (void*)title);
        }
    }
} /* a2b_seqChartHeader */


/*!****************************************************************************
*  \ingroup        a2bstack_seqchart_priv
* 
*  \b              a2b_seqChartFooter
*
*  Called to issue the *closing* PlantUml instruction which is meant to
*  close out/end the chart.
*
*  \param          [in]    ctx      The A2B stack context.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            The sequence chart has already been allocated and started.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_seqChartFooter
    (
    struct a2b_StackContext* ctx
    )
{
    if ( A2B_NULL != ctx )
    {
        a2b_seqChartLog0(ctx, "@enduml");
    }
} /* a2b_seqChartFooter */


/*!****************************************************************************
*  \ingroup        a2bstack_seqchart_priv 
*
*  \b              a2b_seqChartPrefix
*
*  Called to apply the sequence prefix before the actual sequence text so
*  the formatting between source and destination entities remains consistent.
*
*  \param          [in]    ctx          The A2B stack context.
* 
*  \param          [in]    src          The source entity in the sequence.
* 
*  \param          [in]    dest         The destination entity in the sequence.
* 
*  \param          [in]    commType     The type of sequence communication:
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.
*
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
*
*  \post           The actual PlantUml sequence instruction have not been
*                  emitted yet. It is assumed additional text will be appended
*                  to the end of the prefix and then logged.
*
*  \return         None
*
******************************************************************************/
static void
a2b_seqChartPrefix
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType
    )
{
    const a2b_Char* arrow;
    a2b_SeqChartEntity entities[2u];
    a2b_Char* names[A2B_ARRAY_SIZE(entities)];
    a2b_UInt16 idx;
    a2b_Int16 nodeAddr;
    a2b_Char nameBuf[A2B_ARRAY_SIZE(entities)][sizeof("PluginXX")];
    void* args[A2B_ARRAY_SIZE(entities) + 1];

    entities[0u] = src;
    entities[1u] = dest;

    if ( A2B_NULL != ctx )
    {
        switch ( commType )
        {
            case A2B_SEQ_CHART_COMM_REQUEST:
                arrow = "-[#blue]>>";
                break;

            case A2B_SEQ_CHART_COMM_REPLY:
                arrow = "-[#blue]->>";
                break;

            case A2B_SEQ_CHART_COMM_NOTIFY:
                arrow = "o[#green]->>";
                break;

            default:
                arrow = "-[#orange]>";
                break;
        }

        for ( idx = 0; idx < A2B_ARRAY_SIZE(entities); ++idx )
        {
            switch ( entities[idx] )
            {
                case A2B_SEQ_CHART_ENTITY_APP:
                    names[idx] = "Application";
                    break;
                case A2B_SEQ_CHART_ENTITY_STACK:
                    names[idx] = "Stack";
                    break;
                case A2B_SEQ_CHART_ENTITY_PLATFORM:
                    names[idx] = "Platform";
                    break;
                case A2B_SEQ_CHART_ENTITY_PLUGIN_MASTER:
                    names[idx] = "PluginM";
                    break;
                default:
                    /* If it's a slave plugin entity */
                    if ( (entities[idx] >=
                        A2B_SEQ_CHART_ENTITY_PLUGIN_SLAVE_START) &&
                        (entities[idx] <
                        A2B_SEQ_CHART_ENTITY_PLUGIN_SLAVE_END) )
                    {
                        nodeAddr = (a2b_Int16)(entities[idx] -
                                    A2B_SEQ_CHART_ENTITY_PLUGIN_SLAVE_START);
                        args[0] = &nodeAddr;
                        (void)a2b_vsnprintf(nameBuf[idx], A2B_ARRAY_SIZE(nameBuf[0]),
                                "Plugin%hd", args, 1);
                        names[idx] = &nameBuf[idx][0];
                    }
                    else
                    {
                        names[idx] = "Unknown";
                    }
                    break;
            }
        }

        args[0] = names[0]; /* Source */
        args[1] = (void*)arrow;
        args[2] = names[1]; /* Destination */

        (void)a2b_vsnprintfStringBuffer(&ctx->stk->seqChartChan->strBuf,
                                  "%s %s %s : ", args, A2B_ARRAY_SIZE(args));
    }
} /* a2b_seqChartPrefix */


/*!****************************************************************************
* 
*  \b              a2b_seqChartAlloc
*
*  Allocates a sequence chart trace channel from the pool of available (unused)
*  channels for the stack. The underlying (platform specific) logging channel
*  is opened as a result of this call.
*
*  \param          [in]    ctx      The A2B stack context to allocate and open 
*                                   a sequence chart trace channel.
* 
*  \param          [in]    name     Platform specific name to associate with 
*                                   the sequence chart channel.
* 
*  \param          [in]    level    The bitmask of the levels that should be 
*                                   enabled and captured in the sequence chart.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL: ctx->stk
*
*  \post           None
*
*  \return         A pointer to the sequence chart channel or A2B_NULL if one 
*                  could not be allocated for the stack or there was an error 
*                  opening the underlying logging channel.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_SeqChartChannel*
a2b_seqChartAlloc
    (
    struct a2b_StackContext*   ctx,
    const a2b_Char*     name,
    a2b_UInt32          level
    )
{
    /** Pool of available sequence chart channels */
    static struct a2b_SeqChartChannel
        gsSeqChartChanPool[A2B_CONF_MAX_NUM_MASTER_NODES];
    a2b_UInt16 idx;
    a2b_SeqChartChannel* chan = A2B_NULL;

    if ( (A2B_NULL != ctx) && (A2B_NULL != name) )
    {
        for ( idx = 0; idx < A2B_ARRAY_SIZE(gsSeqChartChanPool); ++idx )
        {
            if ( !gsSeqChartChanPool[idx].inUse )
            {
                chan = &gsSeqChartChanPool[idx];
                chan->hnd = ctx->stk->pal.logOpen(name);
                if ( A2B_NULL == chan->hnd )
                {
                    chan = A2B_NULL;
                }
                else
                {
                    chan->ctx = ctx;
                    a2b_stringBufferInit(&chan->strBuf, chan->buf,
                                            A2B_ARRAY_SIZE(chan->buf));
                    chan->inUse = A2B_TRUE;
                    /* The "always" bit should be forced to be enabled */
                    chan->levelMask = level | A2B_SEQ_CHART_LEVEL_ALWAYS;
                    chan->options = A2B_SEQ_CHART_OPT_NONE;
                }
                break;
            }
        }
    }
    return chan;
} /* a2b_seqChartAlloc */


/*!****************************************************************************
* 
*  \b              a2b_seqChartFree
*
*  De-allocates (or frees) the referenced sequence chart trace channel.
*
*  \param          [in]    chan     The A2B channel to free.
*
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_seqChartFree
    (
    a2b_SeqChartChannel*   chan
    )
{
    if ( A2B_NULL != chan )
    {
        chan->ctx->stk->pal.logClose(chan->hnd);
        chan->inUse = A2B_FALSE;
    }
} /* a2b_seqChartFree */


/*!****************************************************************************
*
*  \b              a2b_seqChartStart
*
*  This function starts the capture of a sequence chart for the primary
*  APIs of the stack. If a sequence chart was already started it will be
*  closed and a new one started. The resulting sequence chart statements
*  can be rendered graphically by the PlantUML tool.
*
*  \param          [in]    ctx      The A2B stack context to generate the
*                                   sequence chart. 
* 
*  \param          [in]    url      The URL of where the sequence chart 
*                                   statements will be sent. The supported 
*                                   URL protocols are platform specific and 
*                                   generally take the form of:
*                                   \verbatim
                                    <protocol>//:<resource>:[port]
                                    \endverbatim
 
*  \param          [in]    level    The bitmask of the levels that should be 
*                                   captured in the sequence chart. 
* 
*  \param          [in]    options  Global sequence chart options encoded as 
*                                   a bit mask.  Options including 
*                                   auto-numbering and time stamping sequences. 
* 
*  \param          [in]    title    An optional title for the sequence chart. 
*                                   If equal to A2B_NULL then no title is shown.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled. 
* 
*  \pre            Expectation is that the following are NON-NULL: ctx->stk
*
*  \post           Tracing of API calls via a sequence chart has been started.
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_seqChartStart
    (
    struct a2b_StackContext*    ctx,
    const a2b_Char*             url,
    a2b_UInt32                  level,
    a2b_UInt32                  options,
    const a2b_Char*             title
    )
{
    a2b_HResult status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
                                            A2B_EC_INVALID_PARAMETER);
    if ( (A2B_NULL != ctx) && (A2B_NULL != url) )
    {
        /* If a sequence chart is active now then stop this one and start
         * a new one. There can only be one active one at a time.
         */
        if ( A2B_NULL != ctx->stk->seqChartChan )
        {
            status = a2b_seqChartStop(ctx);
        }
        else
        {
            status = A2B_RESULT_SUCCESS;
        }

        if ( A2B_SUCCEEDED(status) )
        {
            ctx->stk->seqChartChan = a2b_seqChartAlloc(ctx, url, level);
            if ( A2B_NULL == ctx->stk->seqChartChan )
            {
                status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
                                        A2B_EC_ALLOC_FAILURE);
            }
            else
            {
                ctx->stk->seqChartChan->options = options;
                /* Start the sequence chart */
                a2b_seqChartHeader(ctx, title);
            }
        }
    }

    return status;
} /* a2b_seqChartStart */


/*!****************************************************************************
*
*  \b              a2b_seqChartStop
*
*  This function stops the capture of a sequence chart for the primary
*  APIs of the stack. The resulting sequence chart statements can be
*  rendered graphically by the PlantUML tool.
*
*  \param          [in]    ctx      The A2B stack context to generate the
*                                   sequence chart.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            The stack is currently generating a sequence chart of
*                  API calls.
* 
*  \pre            Expectation is that the following are NON-NULL: ctx->stk
*  
*  \post           The tracing of API calls for a sequence chart has ended.
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC  a2b_HResult
a2b_seqChartStop
    (
    struct a2b_StackContext*    ctx
    )
{
    a2b_HResult status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
                                          A2B_EC_INVALID_PARAMETER);

    if ( A2B_NULL != ctx )
    {
        if ( A2B_NULL != ctx->stk->seqChartChan )
        {
            a2b_seqChartFooter(ctx);
            a2b_seqChartFree(ctx->stk->seqChartChan);
            ctx->stk->seqChartChan = A2B_NULL;
        }
        status = A2B_RESULT_SUCCESS;
    }

    return status;
} /* a2b_seqChartStop */


/*!****************************************************************************
*
*  \b              a2b_seqChartGetOptions
*
*  This function is used to get the current options for a specific
*  stack context sequence chart instance.
*
*  \param          [in]    ctx      A2B stack context
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL: ctx->stk
*                  and ctx->stk->seqChartChan (otherwise the return code is
*                  ambiguous)
*
*  \post           None
*
*  \return         Current options bitmask
*                  (e.g. #A2B_SEQ_CHART_OPT_AUTONUMBER, etc)
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_seqChartGetOptions
    (
    struct a2b_StackContext*    ctx
    )
{
    if (( A2B_NULL != ctx ) && ( A2B_NULL != ctx->stk->seqChartChan ))
    {
        return ctx->stk->seqChartChan->options;
    }
    return 0;
} /* a2b_seqChartGetOptions */


/*!****************************************************************************
*
*  \b              a2b_seqChartSetOptions
*
*  This function is used to set the current options for a specific
*  stack context sequence chart instance.
*
*  \param          [in]    ctx         A2B stack context
* 
*  \param          [in]    options     Bitmask of options to set for
*                                      the sequence chart
*                                      (e.g. #A2B_SEQ_CHART_OPT_AUTONUMBER, etc)
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL: ctx->stk
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChartSetOptions
    (
    struct a2b_StackContext*    ctx,
    a2b_UInt32                  options
    )
{
    if (( A2B_NULL != ctx ) && ( A2B_NULL != ctx->stk->seqChartChan ))
    {
        ctx->stk->seqChartChan->options = options;
    }
} /* a2b_seqChartSetOptions */


/*!****************************************************************************
*
*  \b              a2b_seqChartInject
*
*  This function injects raw user text into an active sequence
*  chart. If the sequence chart is not active then an error will be
*  returned. The injected text must constitute a valid PlantUML
*  instruction.
* 
*  \see            http://plantuml.sourceforge.net/index.html
* 
*  \param          [in]    ctx      The A2B stack context to inject text into
*                                   the sequence chart.
* 
*  \param          [in]    level    The sequence chart trace level the text 
*                                   should appear at.
* 
*  \param          [in]    text     The text that should be injected into the
*                                   sequence diagram. It is assumed this text
*                                   constitutes a valid PlantUML instruction.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            The stack is currently generating a sequence chart of
*                  API calls.
* 
*  \pre            Expectation is that the following are NON-NULL: ctx->stk
*
*  \post           None
*
*  \return         A status code that can be checked with the #A2B_SUCCEEDED()
*                  or #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_seqChartInject
    (
    struct a2b_StackContext*    ctx,
    a2b_UInt32                  level,
    const a2b_Char*             text
    )
{
    a2b_HResult status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
                                            A2B_EC_INVALID_PARAMETER);
    if ( (A2B_NULL != ctx) && (A2B_NULL != text)  )
    {
        if ( A2B_NULL == ctx->stk->seqChartChan )
        {
            status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_STACK,
                                      A2B_EC_INVALID_STATE);
        }
        else
        {
            if ( ctx->stk->seqChartChan->levelMask & level )
            {
                a2b_seqChartLog0(ctx, text);
                if ( ctx->stk->seqChartChan->options &
                        A2B_SEQ_CHART_OPT_TIMESTAMP )
                {
                    a2b_seqChartLogTime(ctx);
                }
            }
            status = A2B_RESULT_SUCCESS;
        }
    }

    return status;
} /* a2b_seqChartInject */


/*!****************************************************************************
*
*  \b              a2b_seqChartIsEnabled
*
*  Returns either A2B_TRUE or A2B_FALSE depending on whether or not the
*  given sequence chart channel is enabled (started) or not.
* 
*  \note
*  This interface is meant to match the #a2b_seqChart0..n API's
*  so that creating very simple/compact macros is possible (e.g.
*  #A2B_SEQ_CHART0..x).  Therefore, to use this function directly
*  you will need to provide values for some paramters that are not
*  checked/needed.  The recommended usage is through the macro
*  #A2B_SEQ_CHART_ENABLED.
*
*  \param          [in]    ctx          The A2B stack context.
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (*NOTE USED*)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (*NOTE USED*)
*                                                                                            
*  \param          [in]    commType     The type of sequence communication.
*                                       (*NOTE USED*)
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL: ctx->stk
*
*  \post           None
*
*  \return         Returns A2B_TRUE if the sequence chart has been started or
*                  A2B_FALSE if it is stopped.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Bool
a2b_seqChartIsEnabled
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    ...
    )
{
    a2b_Bool enabled = A2B_FALSE;

    A2B_UNUSED(src);
    A2B_UNUSED(dest);
    A2B_UNUSED(commType);

    if ( (A2B_NULL != ctx) && 
         (A2B_NULL != ctx->stk->seqChartChan) && 
         (level & ctx->stk->seqChartChan->levelMask) )
    {
        enabled = A2B_TRUE;
    }

    return enabled;
} /* a2b_seqChartIsEnabled */


/*!****************************************************************************
*
*  \b              a2b_seqChart0
*
*  Outputs a formatted sequence chart trace with zero (0) parameters specified
*  in the format string.
*
*  \param          [in]    ctx          The A2B stack context.                               
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
* 
*  \param          [in]    commType     The type of sequence communication:                  
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.                        
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
*                                       (A2B_SEQ_CHART_LEVEL_1, etc)
*                                                                                            
*  \param          [in]    fmt          The trace format string.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChart0
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    const a2b_Char*             fmt
    )
{

    A2B_UNUSED(level);
    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        if ( level & chan->levelMask )
        {
            a2b_seqChartPrefix(ctx, src, dest, commType);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, A2B_NULL, 0);
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);

            if ( chan->options & A2B_SEQ_CHART_OPT_TIMESTAMP )
            {
                a2b_seqChartLogTime(ctx);
            }
        }
    }
} /* a2b_seqChart0 */


/*!****************************************************************************
*
*  \b              a2b_seqChart1
*
*  Outputs a formatted sequence chart trace with one (1) parameter specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
*
*  \param          [in]    ctx          The A2B stack context.                               
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
* 
*  \param          [in]    commType     The type of sequence communication:                  
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.                        
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
*                                       (A2B_SEQ_CHART_LEVEL_1, etc)
*                                                                                            
*  \param          [in]    fmt          The trace format string.
* 
*  \param          [in]    a1           First format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChart1
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    const a2b_Char*             fmt,
    void*                       a1
    )
{
    void* args[1u];

    args[0u] = a1;

    A2B_UNUSED(level);

    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        if ( level & chan->levelMask )
        {
            a2b_seqChartPrefix(ctx, src, dest, commType);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);

            if ( chan->options & A2B_SEQ_CHART_OPT_TIMESTAMP )
            {
                a2b_seqChartLogTime(ctx);
            }
        }
    }
} /* a2b_seqChart1 */


/*!****************************************************************************
*
*  \b              a2b_seqChart2
*
*  Outputs a formatted sequence chart trace with two (2) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
*
*  \param          [in]    ctx          The A2B stack context.                               
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
* 
*  \param          [in]    commType     The type of sequence communication:                  
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.                        
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
*                                       (A2B_SEQ_CHART_LEVEL_1, etc)
*                                                                                            
*  \param          [in]    fmt          The trace format string.
* 
*  \param          [in]    a1           First format parameter.
* 
*  \param          [in]    a2           Second format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChart2
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    const a2b_Char*             fmt,
    void*                       a1,
    void*                       a2
    )
{
    void* args[2u];

    args[0u] = a1;
    args[1u] = a2;

    A2B_UNUSED(level);

    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        if ( level & chan->levelMask )
        {
            a2b_seqChartPrefix(ctx, src, dest, commType);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);

            if ( chan->options & A2B_SEQ_CHART_OPT_TIMESTAMP )
            {
                a2b_seqChartLogTime(ctx);
            }
        }
    }
} /* a2b_seqChart2 */


/*!****************************************************************************
*
*  \b              a2b_seqChart3
*
*  Outputs a formatted sequence chart trace with three (3) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx          The A2B stack context.                               
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
* 
*  \param          [in]    commType     The type of sequence communication:                  
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.                        
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
*                                       (A2B_SEQ_CHART_LEVEL_1, etc)
*                                                                                            
*  \param          [in]    fmt          The trace format string.
* 
*  \param          [in]    a1           First format parameter.
* 
*  \param          [in]    a2           Second format parameter.
* 
*  \param          [in]    a3           Third format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChart3
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    const a2b_Char*             fmt,
    void*                       a1,
    void*                       a2,
    void*                       a3
    )
{
    void* args[3u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;

    A2B_UNUSED(level);

    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        if ( level & chan->levelMask )
        {
            a2b_seqChartPrefix(ctx, src, dest, commType);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);

            if ( chan->options & A2B_SEQ_CHART_OPT_TIMESTAMP )
            {
                a2b_seqChartLogTime(ctx);
            }
        }
    }
} /* a2b_seqChart3 */


/*!****************************************************************************
*
*  \b              a2b_seqChart4
*
*  Outputs a formatted sequence chart trace with four (4) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx          The A2B stack context.                               
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
* 
*  \param          [in]    commType     The type of sequence communication:                  
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.                        
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
*                                       (A2B_SEQ_CHART_LEVEL_1, etc)
*                                                                                            
*  \param          [in]    fmt          The trace format string.
* 
*  \param          [in]    a1           First format parameter.
* 
*  \param          [in]    a2           Second format parameter.
* 
*  \param          [in]    a3           Third format parameter.
* 
*  \param          [in]    a4           Fourth format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChart4
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    const a2b_Char*             fmt,
    void*                       a1,
    void*                       a2,
    void*                       a3,
    void*                       a4
    )
{
    void* args[4u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;
    args[3u] = a4;

    A2B_UNUSED(level);

    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        if ( level & chan->levelMask )
        {
            a2b_seqChartPrefix(ctx, src, dest, commType);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);

            if ( chan->options & A2B_SEQ_CHART_OPT_TIMESTAMP )
            {
                a2b_seqChartLogTime(ctx);
            }
        }
    }
} /* a2b_seqChart4 */


/*!****************************************************************************
*
*  \b              a2b_seqChart5
*
*  Outputs a formatted sequence chart trace with five (5) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx          The A2B stack context.                               
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
* 
*  \param          [in]    commType     The type of sequence communication:                  
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.                        
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
*                                       (A2B_SEQ_CHART_LEVEL_1, etc)
*                                                                                            
*  \param          [in]    fmt          The trace format string.
* 
*  \param          [in]    a1           First format parameter.
* 
*  \param          [in]    a2           Second format parameter.
* 
*  \param          [in]    a3           Third format parameter.
* 
*  \param          [in]    a4           Fourth format parameter.
* 
*  \param          [in]    a5           Fifth format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChart5
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    const a2b_Char*             fmt,
    void*                       a1,
    void*                       a2,
    void*                       a3,
    void*                       a4,
    void*                       a5
    )
{
    void* args[5u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;
    args[3u] = a4;
    args[4u] = a5;

    A2B_UNUSED(level);

    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        if ( level & chan->levelMask )
        {
            a2b_seqChartPrefix(ctx, src, dest, commType);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);

            if ( chan->options & A2B_SEQ_CHART_OPT_TIMESTAMP )
            {
                a2b_seqChartLogTime(ctx);
            }
        }
    }
} /* a2b_seqChart5 */


/*!****************************************************************************
*
*  \b              a2b_seqChart6
*
*  Outputs a formatted sequence chart trace with six (6) parameters specified
*  in the format string.
*
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx          The A2B stack context.                               
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
* 
*  \param          [in]    commType     The type of sequence communication:                  
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.                        
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
*                                       (A2B_SEQ_CHART_LEVEL_1, etc)
*                                                                                            
*  \param          [in]    fmt          The trace format string.
* 
*  \param          [in]    a1           First format parameter.
* 
*  \param          [in]    a2           Second format parameter.
* 
*  \param          [in]    a3           Third format parameter.
* 
*  \param          [in]    a4           Fourth format parameter.
* 
*  \param          [in]    a5           Fifth format parameter.
* 
*  \param          [in]    a6           Sixth format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChart6
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    const a2b_Char*             fmt,
    void*                       a1,
    void*                       a2,
    void*                       a3,
    void*                       a4,
    void*                       a5,
    void*                       a6
    )
{
    void* args[6u];

    args[0u] = a1;
    args[1u] = a2;
    args[2u] = a3;
    args[3u] = a4;
    args[4u] = a5;
    args[5u] = a6;

    A2B_UNUSED(level);

    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        if ( level & chan->levelMask )
        {
            a2b_seqChartPrefix(ctx, src, dest, commType);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);

            if ( chan->options & A2B_SEQ_CHART_OPT_TIMESTAMP )
            {
                a2b_seqChartLogTime(ctx);
            }
        }
    }
} /* a2b_seqChart6 */


/*!****************************************************************************
*
*  \b              a2b_seqChart7
*
*  Outputs a formatted sequence chart trace with seven (7) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx          The A2B stack context.                               
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
* 
*  \param          [in]    commType     The type of sequence communication:                  
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.                        
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
*                                       (A2B_SEQ_CHART_LEVEL_1, etc)
*                                                                                            
*  \param          [in]    fmt          The trace format string.
* 
*  \param          [in]    a1           First format parameter.
* 
*  \param          [in]    a2           Second format parameter.
* 
*  \param          [in]    a3           Third format parameter.
* 
*  \param          [in]    a4           Fourth format parameter.
* 
*  \param          [in]    a5           Fifth format parameter.
* 
*  \param          [in]    a6           Sixth format parameter.
* 
*  \param          [in]    a7           Seventh format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChart7
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    const a2b_Char*             fmt,
    void*                       a1,
    void*                       a2,
    void*                       a3,
    void*                       a4,
    void*                       a5,
    void*                       a6,
    void*                       a7
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

    A2B_UNUSED(level);

    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        if ( level & chan->levelMask )
        {
            a2b_seqChartPrefix(ctx, src, dest, commType);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);

            if ( chan->options & A2B_SEQ_CHART_OPT_TIMESTAMP )
            {
                a2b_seqChartLogTime(ctx);
            }
        }
    }
} /* a2b_seqChart7 */


/*!****************************************************************************
*
*  \b              a2b_seqChart8
*
*  Outputs a formatted sequence chart trace with eight (8) parameters specified
*  in the format string.
* 
*  \note
*  [Design Note]   This was not implemented with variadic support (ellipses)
*                  to reduce PAL porting requirements and dependencies.
*                  As a result there is a single parameter for each format
*                  parameter.
* 
*  \param          [in]    ctx          The A2B stack context.                               
*                                                                                            
*  \param          [in]    src          The source entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
*                                                                                            
*  \param          [in]    dest         The destination entity in the sequence.
*                                       (A2B_SEQ_CHART_ENTITY_xxx)
* 
*  \param          [in]    commType     The type of sequence communication:                  
*                                       #A2B_SEQ_CHART_COMM_REQUEST,
*                                       #A2B_SEQ_CHART_COMM_REPLY,
*                                       or #A2B_SEQ_CHART_COMM_NOTIFY.                        
*                                                                                            
*  \param          [in]    level        The sequence event level.                            
*                                       (A2B_SEQ_CHART_LEVEL_1, etc)
*                                                                                            
*  \param          [in]    fmt          The trace format string.
* 
*  \param          [in]    a1           First format parameter.
* 
*  \param          [in]    a2           Second format parameter.
* 
*  \param          [in]    a3           Third format parameter.
* 
*  \param          [in]    a4           Fourth format parameter.
* 
*  \param          [in]    a5           Fifth format parameter.
* 
*  \param          [in]    a6           Sixth format parameter.
* 
*  \param          [in]    a7           Seventh format parameter.
* 
*  \param          [in]    a8           Eighth format parameter.
* 
*  \pre            Only available when #A2B_FEATURE_SEQ_CHART is enabled.
* 
*  \pre            Expectation is that the following are NON-NULL:
*                  ctx->stk and ctx->stk->seqChartChan
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_seqChart8
    (
    struct a2b_StackContext*    ctx,
    a2b_SeqChartEntity          src,
    a2b_SeqChartEntity          dest,
    a2b_SeqChartCommType        commType,
    a2b_UInt32                  level,
    const a2b_Char*             fmt,
    void*                       a1,
    void*                       a2,
    void*                       a3,
    void*                       a4,
    void*                       a5,
    void*                       a6,
    void*                       a7,
    void*                       a8
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

    A2B_UNUSED(level);

    if ( A2B_NULL != ctx )
    {
        struct a2b_SeqChartChannel* chan = ctx->stk->seqChartChan;

        if ( level & chan->levelMask )
        {
            a2b_seqChartPrefix(ctx, src, dest, commType);
            (void)a2b_vsnprintfStringBuffer(&chan->strBuf, fmt, args,
                                        A2B_ARRAY_SIZE(args));
            ctx->stk->pal.logWrite(chan->hnd, chan->buf);
            a2b_stringBufferClear(&chan->strBuf);

            if ( chan->options & A2B_SEQ_CHART_OPT_TIMESTAMP )
            {
                a2b_seqChartLogTime(ctx);
            }
        }
    }
} /* a2b_seqChart8 */

#endif /* A2B_FEATURE_SEQ_CHART */
