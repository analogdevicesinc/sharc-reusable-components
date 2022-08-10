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
 * \file:   i2c.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Implements the stack I2C API for plugins and A2B applications.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \ingroup  a2bstack_i2c 
 * \defgroup a2bstack_i2c_priv          \<Private\> 
 * \private 
 *  
 * This defines I2C API's that are private to the stack.
 */
/*============================================================================*/

/*======================= I N C L U D E S =========================*/

#include "a2b/pal.h"
#include "a2b/util.h"
#include "a2b/trace.h"
#include "a2b/error.h"
#include "a2b/regdefs.h"
#include "a2b/defs.h"
#include "a2b/msgrtr.h"
#include "a2b/stringbuffer.h"
#include "a2b/seqchart.h"
#include "stack_priv.h"
#include "i2c_priv.h"
#include "stackctx.h"
#include "utilmacros.h"
/*======================= D E F I N E S ===========================*/

#define A2B_I2C_TEMP_BUF_SIZE   (64)
#define A2B_I2C_DATA_ARGS       (8)

/*======================= L O C A L  P R O T O T Y P E S  =========*/
#if defined(A2B_FEATURE_TRACE) || defined(A2B_FEATURE_SEQ_CHART)
static a2b_Char* a2b_i2cFormatString(a2b_Char* buf,
    a2b_UInt32  bufLen, const a2b_Char* fmt, void**  args,
    a2b_UInt16  numArgs, const a2b_Byte* data, a2b_UInt16 nBytes);
#endif  /* A2B_FEATURE_TRACE || A2B_FEATURE_SEQ_CHART */
static a2b_HResult a2b_i2cRead(a2b_StackContext* ctx,
    a2b_UInt16 addr, a2b_UInt16 nRead, a2b_Byte* rBuf);
static a2b_HResult a2b_i2cWrite(a2b_StackContext* ctx,
    a2b_UInt16 addr, a2b_UInt16 nWrite, const a2b_Byte* wBuf);
static a2b_HResult a2b_i2cWriteRead(a2b_StackContext*  ctx,
    a2b_UInt16 addr, a2b_UInt16 nWrite, const a2b_Byte* wBuf,
    a2b_UInt16 nRead, a2b_Byte*  rBuf);
static a2b_HResult a2b_i2cPrepAccess(a2b_StackContext* ctx,
    a2b_I2cCmd  cmd, a2b_Int16 nodeAddr, a2b_UInt16 i2cAddr);
static a2b_HResult a2b_i2cExecuteAccess(struct a2b_StackContext* ctx,
    a2b_I2cCmd cmd, a2b_Int16 nodeAddr, a2b_UInt16 i2cAddr,
    a2b_UInt16 nWrite, void* wBuf, a2b_UInt16 nRead, void* rBuf);
static a2b_HResult
a2b_i2cGenericWriteRead
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  i2cAddr,
    a2b_UInt16                  nWrite,
    void*                       wBuf,
    a2b_UInt16                  nRead,
    void*                       rBuf
    );
static a2b_HResult
a2b_i2cGenericWrite
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  i2cAddr,
    a2b_UInt16                  nWrite,
    void*                       wBuf
    );
/*======================= D A T A  ================================*/

/*==================== C O D E =================================*/

#if defined(A2B_FEATURE_TRACE) || defined(A2B_FEATURE_SEQ_CHART)

/*!****************************************************************************
*  \ingroup         a2bstack_i2c_priv
*  
*  \b               a2b_i2cFormatString
* 
*  This is an internal function for formatting a string that optionally
*  includes the contents of a data buffer. This is used for formatting strings
*  for trace and sequence chart diagnostics and is only available when one of
*  the features are enabled.
* 
*  \param   [in,out]    buf     The character buffer in which the formatted
*                               string will be rendered.
* 
*  \param   [in]        bufLen  The size of 'buf' in characters.
* 
*  \param   [in]        fmt     The sprintf-like format string. The format
*                               string only supports a subset of options
*                               described by the <em>#a2b_vsnprintf</em>
*                               function.
* 
*  \param   [in]        args    An array of void* entries pointing at the
*                               arguments for the format string.
* 
*  \param   [in]        numArgs The number of arguments in the <em>args</em>
* 
*  \param   [in]        data    An array of bytes to be formatted as hex
*                               digits at the end of the formatted string.
*                               At most #A2B_I2C_DATA_ARGS will be formatted.
*                               May be A2B_NULL if no data is available.
* 
*  \param   [in]        nBytes  The number of data bytes in the <em>data</em>
*                               array.
* 
*  \pre     Only available when #A2B_FEATURE_SEQ_CHART or #A2B_FEATURE_TRACE
*           is enabled.
* 
*  \post    The <em>buf</em> hold the A2B_NULL terminated formatted string.
* 
*  \return  A pointer to the <em>buf</em> that was passed in.
* 
******************************************************************************/
static a2b_Char*
a2b_i2cFormatString
    (
    a2b_Char*       buf,
    a2b_UInt32      bufLen,
    const a2b_Char* fmt,
    void**          args,
    a2b_UInt16      numArgs,
    const a2b_Byte* data,
    a2b_UInt16      nBytes
    )
{
    a2b_StringBuffer    sb;
    a2b_UInt16          idx;
    a2b_UInt16          nDataArgs;
    void*               argVec[1];

    a2b_stringBufferInit(&sb, buf, bufLen);

    (void)a2b_vsnprintfStringBuffer(&sb, fmt, args, numArgs);

    nDataArgs = A2B_MIN(nBytes, A2B_I2C_DATA_ARGS);
    for ( idx = 0; idx < nDataArgs; ++idx )
    {
        argVec[0] = (void*)&data[idx];
        (void)a2b_vsnprintfStringBuffer(&sb, "%02bX", argVec, A2B_ARRAY_SIZE(argVec));
        if ( idx + 1 < nDataArgs )
        {
            (void)a2b_vsnprintfStringBuffer(&sb, " ", A2B_NULL, 0);
        }
    }

    if ( nBytes > nDataArgs )
    {
        (void)a2b_vsnprintfStringBuffer(&sb, " ...", A2B_NULL, 0);
    }

    return buf;

} /* a2b_i2cFormatString */

#endif  /* A2B_FEATURE_TRACE || A2B_FEATURE_SEQ_CHART */


/*!****************************************************************************
*  \ingroup         a2bstack_i2c_priv
*  
*  \b               a2b_i2cRead
* 
*  Reads bytes from the I2C device. This is a synchronous call and will block
*  until the operation is complete.
* 
*  \param   [in]        ctx     The stack context associated with the read.
* 
*  \param   [in]        addr    The 7-bit I2C address.
* 
*  \param   [in]        nRead   The number of bytes to read from the device.
* 
*  \param   [in,out]    rBuf    A buffer in which to write the results of the
*                               read.
* 
*  \pre     None
* 
*  \post    The read buffer holds the contents of the read on success.
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
static a2b_HResult
a2b_i2cRead
    (
    a2b_StackContext*   ctx,
    a2b_UInt16          addr,
    a2b_UInt16          nRead,
    a2b_Byte*           rBuf
    )
{

    a2b_HResult result = ctx->stk->pal.i2cRead(ctx->stk->i2cHnd, addr,
                                                nRead, rBuf);

#if defined(A2B_FEATURE_TRACE) || defined(A2B_FEATURE_SEQ_CHART)
    a2b_Char    buf[A2B_I2C_TEMP_BUF_SIZE];
    void*       args[3u];

#ifdef A2B_FEATURE_SEQ_CHART
    a2b_UInt32  callOrigin;
#endif  /* A2B_FEATURE_SEQ_CHART */

#ifdef A2B_FEATURE_TRACE
    a2b_UInt32  trcMask = A2B_TRC_DOM_I2C | A2B_TRC_LVL_TRACE2;
#endif

    args[0U] = &addr;
    args[1U] = &result;
    args[2U] = A2B_NULL;

#ifdef A2B_FEATURE_TRACE
    if ( A2B_SUCCEEDED(result) )
    {
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                    "a2b_i2cRead[0x%02hX] <- ", args, 1, rBuf, nRead);
    }
    else
    {
        trcMask |= A2B_TRC_LVL_ERROR;
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                                "a2b_i2cRead[0x%02hX] Error: 0x%lX",
                                 args, 2, A2B_NULL, 0);
    }
    A2B_TRACE1((ctx, trcMask, "%s", buf));
#endif  /* A2B_FEATURE_TRACE */

#ifdef A2B_FEATURE_SEQ_CHART
    args[0] = &addr;
    args[1] = &nRead;
    args[2] = &result;
    callOrigin = (a2b_UInt32)((a2b_UInt32)(ctx->domain == A2B_DOMAIN_APP) ?
                        A2B_SEQ_CHART_ENTITY_APP :
                        A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                            ctx->ccb.plugin.nodeSig.nodeAddr));

    (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf), "a2b_i2cRead(0x%02hX, %hu)",
                                args, 2, A2B_NULL, 0);
    A2B_SEQ_CHART1((ctx,
                    (a2b_SeqChartEntity)callOrigin,
                    A2B_SEQ_CHART_ENTITY_PLATFORM,
                    A2B_SEQ_CHART_COMM_REQUEST,
                    A2B_SEQ_CHART_LEVEL_I2C,
                    "%s", buf));

    if ( A2B_SUCCEEDED(result) )
    {
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                "a2b_i2cRead(0x%02hX, %hu) <- ", args, 2, rBuf, nRead);
    }
    else
    {
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                            "a2b_i2cRead(0x%02hX, %hu) <- Error 0x%lX",
                             args, 3, A2B_NULL, 0);
    }
    A2B_SEQ_CHART1((ctx,
                    A2B_SEQ_CHART_ENTITY_PLATFORM,
                    (a2b_SeqChartEntity)callOrigin,
                    A2B_SEQ_CHART_COMM_REPLY,
                    A2B_SEQ_CHART_LEVEL_I2C,
                    "%s", buf));
#endif /* A2B_FEATURE_SEQ_CHART */

#endif /* A2B_FEATURE_TRACE || A2B_FEATURE_SEQ_CHART */

    return result;

} /* a2b_i2cRead */


/*!****************************************************************************
*  \ingroup         a2bstack_i2c_priv
*  
*  \b               a2b_i2cWrite
* 
*  Writes bytes to the I2C device. This is an synchronous call and will block
*  until the operation is complete.
* 
*  \param   [in]    ctx     The stack context associated with the write.
* 
*  \param   [in]    addr    The 7-bit I2C address.
* 
*  \param   [in]    nWrite  The number of bytes to write.
* 
*  \param   [in]    wBuf    A buffer containing the data to write. The buffer
*                           is of size 'nWrite' bytes.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
static a2b_HResult
a2b_i2cWrite
    (
    a2b_StackContext*   ctx,
    a2b_UInt16          addr,
    a2b_UInt16          nWrite,
    const a2b_Byte*     wBuf
    )
{
    a2b_HResult result = ctx->stk->pal.i2cWrite(ctx->stk->i2cHnd, addr,
                                                nWrite, wBuf);

#if defined(A2B_FEATURE_TRACE) || defined(A2B_FEATURE_SEQ_CHART)
    a2b_Char    buf[A2B_I2C_TEMP_BUF_SIZE];
    void*       args[3u];

#ifdef A2B_FEATURE_SEQ_CHART
    a2b_UInt32  callOrigin;
#endif  /* A2B_FEATURE_SEQ_CHART */

#ifdef A2B_FEATURE_TRACE
    a2b_UInt32  trcMask = A2B_TRC_DOM_I2C | A2B_TRC_LVL_TRACE2;
#endif

    args[0u] = &addr;
    args[1u] = &result;
    args[2u] = A2B_NULL;

#ifdef A2B_FEATURE_TRACE
    if ( A2B_SUCCEEDED(result) )
    {
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                        "a2b_i2cWrite[0x%02hX] -> ", args, 1, wBuf, nWrite);
    }
    else
    {
        trcMask |= A2B_TRC_LVL_ERROR;
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                                    "a2b_i2cWrite[0x%02hX] Error: 0x%lX",
                                    args, 2, wBuf, nWrite);
    }
    A2B_TRACE1((ctx, trcMask, "%s", buf));
#endif  /* A2B_FEATURE_TRACE */

#ifdef A2B_FEATURE_SEQ_CHART
    args[0] = &addr;
    args[1] = &nWrite;
    args[2] = &result;
    callOrigin = (a2b_UInt32)((a2b_UInt32)(ctx->domain == A2B_DOMAIN_APP) ?
                    A2B_SEQ_CHART_ENTITY_APP :
                    A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                        ctx->ccb.plugin.nodeSig.nodeAddr));

    (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                "a2b_i2cWrite(0x%02hX, %hu) -> ", args, 2, wBuf, nWrite);
    A2B_SEQ_CHART1((ctx,
                    (a2b_SeqChartEntity)callOrigin,
                    A2B_SEQ_CHART_ENTITY_PLATFORM,
                    A2B_SEQ_CHART_COMM_REQUEST,
                    A2B_SEQ_CHART_LEVEL_I2C,
                    "%s", buf));

    if ( A2B_SUCCEEDED(result) )
    {
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                            "a2b_i2cWrite(0x%02hX, %hu) <- Success",
                                args, 2, A2B_NULL, 0);
    }
    else
    {
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                            "a2b_i2cWrite(0x%02hX, %hu) <- Error: 0x%lX",
                                args, 3, A2B_NULL, 0);
    }
    A2B_SEQ_CHART1((ctx,
                    A2B_SEQ_CHART_ENTITY_PLATFORM,
                    (a2b_SeqChartEntity)callOrigin,
                    A2B_SEQ_CHART_COMM_REPLY,
                    A2B_SEQ_CHART_LEVEL_I2C,
                    "%s", buf));
#endif  /* A2B_FEATURE_SEQ_CHART */


#endif /* A2B_FEATURE_TRACE || A2B_FEATURE_SEQ_CHART */

    return result;

} /* a2b_i2cWrite */


/*!****************************************************************************
*  \ingroup         a2bstack_i2c_priv
*  
*  \b               a2b_i2cWriteRead
* 
*  Writes and then reads bytes from the I2C device *without* an I2C stop
*  sequence separating the two operations. Instead a repeated I2C start
*  sequence is used as the operation separator. This is an synchronous call and
*  will block until the operation is complete.
* 
*  \param   [in]    ctx     The stack context associated with the write/read.
* 
*  \param   [in]    addr    The 7-bit I2C address.
* 
*  \param   [in]    nWrite  The number of bytes to write.
* 
*  \param   [in]    wBuf    A buffer containing the data to write. The buffer
*                           is of size 'nWrite' bytes.
* 
*  \param   [in]    nRead   The number of bytes to read from the device.
* 
*  \param   [in]    rBuf    A buffer in which to write the results of the read.
* 
*  \pre     None
* 
*  \post    The read buffer holds the contents of the read on success.
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
static a2b_HResult
a2b_i2cWriteRead
    (
    a2b_StackContext*   ctx,
    a2b_UInt16          addr,
    a2b_UInt16          nWrite,
    const a2b_Byte*     wBuf,
    a2b_UInt16          nRead,
    a2b_Byte*           rBuf
    )
{
    a2b_HResult result = ctx->stk->pal.i2cWriteRead(ctx->stk->i2cHnd, addr,
                                                nWrite, wBuf, nRead, rBuf);

#if defined(A2B_FEATURE_SEQ_CHART) || defined(A2B_FEATURE_TRACE)
    a2b_Char    buf[A2B_I2C_TEMP_BUF_SIZE];
    void*       args[4u];

#if defined(A2B_FEATURE_TRACE)
    a2b_UInt32  trcMask = A2B_TRC_DOM_I2C | A2B_TRC_LVL_TRACE2;
#endif

#ifdef A2B_FEATURE_SEQ_CHART
    a2b_UInt32  callOrigin;
#endif  /* A2B_FEATURE_SEQ_CHART */

    args[0u] = &addr;
    args[1u] = &result;
    args[2u] = A2B_NULL;
    args[3u] = A2B_NULL;

#ifdef A2B_FEATURE_TRACE
    if ( A2B_SUCCEEDED(result) )
    {
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                "a2b_i2cWriteRead[0x%02hX] -> ", args, 1, wBuf, nWrite);
        A2B_TRACE1((ctx, trcMask, "%s", buf));

        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                "a2b_i2cWriteRead[0x%02hX] <- ", args, 1, rBuf, nRead);
        A2B_TRACE1((ctx, trcMask, "%s", buf));
    }
    else
    {
        trcMask |= A2B_TRC_LVL_ERROR;
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                                    "a2b_i2cWriteRead[0x%02hX] Error: 0x%lX",
                                    args, 2, A2B_NULL, 0);
		A2B_TRACE1((ctx, trcMask, "%s", buf));
    }
#endif /* A2B_FEATURE_TRACE */

#ifdef A2B_FEATURE_SEQ_CHART
    args[0] = &addr;
    args[1] = &nWrite;
    args[2] = &nRead;
    args[3] = &result;
    callOrigin = (a2b_UInt32)((a2b_UInt32)(ctx->domain == A2B_DOMAIN_APP) ?
                        A2B_SEQ_CHART_ENTITY_APP :
                        A2B_NODE_ADDR_TO_CHART_PLUGIN_ENTITY(
                            ctx->ccb.plugin.nodeSig.nodeAddr));
    (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                                "a2b_i2cWriteRead(0x%02hX, %hu, %hu) -> ",
                                args, 3, wBuf, nWrite);
    A2B_SEQ_CHART1((ctx,
                    (a2b_SeqChartEntity)callOrigin,
                    A2B_SEQ_CHART_ENTITY_PLATFORM,
                    A2B_SEQ_CHART_COMM_REQUEST,
                    A2B_SEQ_CHART_LEVEL_I2C,
                    "%s", buf));

    if ( A2B_SUCCEEDED(result) )
    {
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                            "a2b_i2cWriteRead(0x%02hX, %hu, %hu) <- ",
                                args, 3, rBuf, nRead);
    }
    else
    {
        (void)a2b_i2cFormatString(buf, A2B_ARRAY_SIZE(buf),
                        "a2b_i2cWriteRead(0x%02hX, %hu, %hu) <- Error: 0x%lX",
                                args, 4, A2B_NULL, 0);
    }
    A2B_SEQ_CHART1((ctx,
                    A2B_SEQ_CHART_ENTITY_PLATFORM,
                    (a2b_SeqChartEntity)callOrigin,
                    A2B_SEQ_CHART_COMM_REPLY,
                    A2B_SEQ_CHART_LEVEL_I2C,
                    "%s", buf));
#endif  /* A2B_FEATURE_SEQ_CHART */

#endif

    return result;

} /* a2b_i2cWriteRead */


/*!****************************************************************************
* \ingroup         a2bstack_i2c_priv
* 
* \b               a2b_i2cPrepAccess
*
* Prepares I2C access to the master node, slave nodes, or peripheral
* devices attached to a slave node. Since all I2C access is tunneled
* through the master node the master must be configured to pass the I2C
* command appropriately. The last access "mode" is cached to avoid
* repeated configuration of AD2410 registers that do not change between
* access to the same entity.
*
* \param   [in]    ctx         The stack context.
*
* \param   [in]    cmd         The I2C command to execute.
*
* \param   [in]    nodeAddr    The destination A2B node address. Only
*                              applicable to slave and peripheral accesses
*                              and ignored otherwise.
*
* \param   [in]    i2cAddr     The peripheral I2C address. Only applicable
*                              to peripheral accesses and ignored
*                              otherwise.
*
* \pre     None
*
* \post    None
*
* \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*          #A2B_FAILED() for success or failure of the operation.
*
******************************************************************************/
static a2b_HResult
a2b_i2cPrepAccess
    (
    a2b_StackContext*   ctx,
    a2b_I2cCmd          cmd,
    a2b_Int16           nodeAddr,
    a2b_UInt16          i2cAddr
    )
{
    a2b_Byte buf[2];
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_I2C,
                                          A2B_EC_INVALID_PARAMETER);
    a2b_UInt16  masterAddr;

    if ( A2B_NULL != ctx )
    {
        masterAddr = ctx->stk->ecb->baseEcb.i2cMasterAddr;

        switch ( cmd )
        {
            case A2B_I2C_CMD_READ_MASTER:
            case A2B_I2C_CMD_WRITE_MASTER:
            case A2B_I2C_CMD_WRITE_READ_MASTER:
                ctx->stk->i2cMode.access = A2B_I2C_ACCESS_MASTER;
                /* No chip or node address change */
                result = A2B_RESULT_SUCCESS;
                break;

            case A2B_I2C_CMD_READ_SLAVE:
            case A2B_I2C_CMD_WRITE_SLAVE:
            case A2B_I2C_CMD_WRITE_READ_SLAVE:
                if ( ((a2b_UInt32)ctx->stk->i2cMode.access == (a2b_UInt32)A2B_I2C_ACCESS_SLAVE)
                      &&  (!ctx->stk->i2cMode.broadcast) &&
					        (nodeAddr == ctx->stk->i2cMode.nodeAddr))
                {
                    /* No chip, broadcast or node address change */
                    result = A2B_RESULT_SUCCESS;
                }
                /* Else we need to set configure NODEADR */
                else
                {
                    buf[0] = A2B_REG_NODEADR;
                    buf[1] = (a2b_UInt8)nodeAddr & A2B_BITM_NODEADR_NODE;

                    result = a2b_i2cWrite(ctx,
                                            masterAddr,
											(a2b_UInt16)A2B_ARRAY_SIZE(buf),
                                            buf);
                    if ( A2B_FAILED(result) )
                    {
                        /* Reset the last access mode so the next time the
                         * cached values won't be assumed.
                         */
                        a2b_stackResetI2cLastMode(&ctx->stk->i2cMode);
                    }
                    else
                    {
                        ctx->stk->i2cMode.access = A2B_I2C_ACCESS_SLAVE;
                        ctx->stk->i2cMode.nodeAddr = nodeAddr;
                        ctx->stk->i2cMode.broadcast = A2B_FALSE;

                    }
                }
                break;

            case A2B_I2C_CMD_WRITE_BROADCAST_SLAVE:
                if ( ((a2b_UInt32)ctx->stk->i2cMode.access == (a2b_UInt32)A2B_I2C_ACCESS_SLAVE)
                        &&  (ctx->stk->i2cMode.broadcast))
                        {
                    /* No chip, broadcast or node address change */
                    result = A2B_RESULT_SUCCESS;
                        }
                /* Else we need to set configure NODEADR */
                        else
                        {
                    buf[0] = A2B_REG_NODEADR;
                    buf[1] = (a2b_UInt8)nodeAddr & A2B_BITM_NODEADR_NODE;
                    buf[1] |= (a2b_Byte)A2B_BITM_NODEADR_BRCST;

                    result = a2b_i2cWrite(ctx,
                                            masterAddr,
                                            (a2b_UInt16)A2B_ARRAY_SIZE(buf),
                                            buf);
                    if ( A2B_FAILED(result) )
                    {
                        /* Reset the last access mode so the next time the
                         * cached values won't be assumed.
                         */
                        a2b_stackResetI2cLastMode(&ctx->stk->i2cMode);
                        }
                    else
                    {
                        ctx->stk->i2cMode.access = A2B_I2C_ACCESS_SLAVE;
                        ctx->stk->i2cMode.nodeAddr = nodeAddr;
                        ctx->stk->i2cMode.broadcast = A2B_TRUE;
                    }
                }
                break;

            case A2B_I2C_CMD_READ_PERIPH:
            case A2B_I2C_CMD_WRITE_PERIPH:
            case A2B_I2C_CMD_WRITE_READ_PERIPH:
                if ( ((a2b_UInt32)ctx->stk->i2cMode.access == (a2b_UInt32)A2B_I2C_ACCESS_PERIPH) &&
                    (nodeAddr == ctx->stk->i2cMode.nodeAddr) &&
                    (i2cAddr == ctx->stk->i2cMode.chipAddr) &&
                    (!ctx->stk->i2cMode.broadcast ))
                {
                    /* No changes to the current settings */
                    result = A2B_RESULT_SUCCESS;
                }
                else
                {
                    buf[0] = A2B_REG_NODEADR;
                    buf[1] = (a2b_UInt8)nodeAddr & A2B_BITM_NODEADR_NODE;

                    result = a2b_i2cWrite(ctx,
                                            masterAddr,
											(a2b_UInt16)A2B_ARRAY_SIZE(buf),
                                            buf);


                    if ( A2B_SUCCEEDED(result) )
                    {
                        buf[0] = A2B_REG_CHIP;
                        buf[1] = (a2b_Byte)i2cAddr;

                        result = a2b_i2cWrite(ctx,
                                            A2B_MAKE_I2C_BUS_ADDR(masterAddr),
											(a2b_UInt16)A2B_ARRAY_SIZE(buf),
                                            buf);

                    }

                    if ( A2B_SUCCEEDED(result) )
                    {
                        buf[0] = A2B_REG_NODEADR;
                        buf[1] = ((a2b_UInt8)nodeAddr & A2B_BITM_NODEADR_NODE) |
                                A2B_BITM_NODEADR_PERI;

                        result = a2b_i2cWrite(ctx,
                                                masterAddr,
												(a2b_UInt16)A2B_ARRAY_SIZE(buf),
                                                buf);
                    }


                    if ( A2B_FAILED(result) )
                    {
                        /* Reset the last access mode so the next time the
                         * cached values won't be assumed.
                         */
                        a2b_stackResetI2cLastMode(&ctx->stk->i2cMode);
                    }
                    /* Else successfully prepared for access */
                    else
                    {
                        /* Cache the last I2C access mode */
                        ctx->stk->i2cMode.access = A2B_I2C_ACCESS_PERIPH;
                        ctx->stk->i2cMode.nodeAddr = nodeAddr;
                        ctx->stk->i2cMode.chipAddr = i2cAddr;
                        ctx->stk->i2cMode.broadcast = A2B_FALSE;
                    }
                }
                break;

            default:
                /* Unknown command */
                result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_I2C,
                                                          A2B_EC_INTERNAL);
                break;
        }
    }

    return result;

} /* a2b_i2cPrepAccess */


/*!****************************************************************************
*  \ingroup         a2bstack_i2c_priv
*  
*  \b               a2b_i2cExecuteAccess
* 
*  Synchronously executes the requested I2C operation on the master node,
*  slave node, or attached peripheral.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   operation.
* 
*  \param   [in]        cmd         The I2C operation to execute.
* 
*  \param   [in]        nodeAddr    The slave node address of a slave node or
*                                   peripheral I2C access. Unused for master
*                                   node accesses.
* 
*  \param   [in]        i2cAddr     The 7-bit I2C address of the peripheral
*                                   device attached to a slave node. Unused
*                                   for slave/master node accesses.
* 
*  \param   [in]        nWrite      The number of bytes to write.
* 
*  \param   [in]        wBuf        A buffer containing the data to write. The
*                                   buffer is of size 'nWrite' bytes.
* 
*  \param   [in]        nRead       The number of bytes to read.
* 
*  \param   [in,out]    rBuf        A buffer in which to write the results of
*                                   a read.
* 
*  \pre     None
* 
*  \post    For an I2C operation involving a read 'rBuf' holds the contents of
*           the read if successful.
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
static a2b_HResult
a2b_i2cExecuteAccess
    (
    struct a2b_StackContext*    ctx,
    a2b_I2cCmd                  cmd,
    a2b_Int16                   nodeAddr,
    a2b_UInt16                  i2cAddr,
    a2b_UInt16                  nWrite,
    void*                       wBuf,
    a2b_UInt16                  nRead,
    void*                       rBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);
    a2b_UInt16 nodeI2cAddr;

    if ( A2B_NULL != ctx )
    {
        /* Prepare the access to the master/slave/peripheral by setting
         * up some configuration registers in the master node.
         */
        result = a2b_i2cPrepAccess(ctx, cmd, nodeAddr,  i2cAddr);

        if ( A2B_SUCCEEDED(result) )
        {
            if ( (cmd == A2B_I2C_CMD_READ_MASTER) ||
                (cmd == A2B_I2C_CMD_WRITE_MASTER) ||
                (cmd == A2B_I2C_CMD_WRITE_READ_MASTER) )
            {
                nodeI2cAddr = ctx->stk->ecb->baseEcb.i2cMasterAddr;
            }
            /* Else must be a slave or peripheral access */
            else
            {
                /* These reads/writes go to the master node's "bus" address */
                nodeI2cAddr = A2B_MAKE_I2C_BUS_ADDR(
                                ctx->stk->ecb->baseEcb.i2cMasterAddr);
            }

            switch ( cmd )
            {
                case A2B_I2C_CMD_READ_MASTER:
                case A2B_I2C_CMD_READ_SLAVE:
                case A2B_I2C_CMD_READ_PERIPH:
                    if ( A2B_NULL == ctx->stk->pal.i2cRead )
                    {
                        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                                 A2B_FAC_I2C, A2B_EC_INTERNAL);
                    }
                    else
                    {

                        result = a2b_i2cRead(ctx,
                                            nodeI2cAddr,
                                            nRead,
                                            rBuf);
                    }
                    break;

                case A2B_I2C_CMD_WRITE_MASTER:
                case A2B_I2C_CMD_WRITE_SLAVE:
                case A2B_I2C_CMD_WRITE_BROADCAST_SLAVE:
                case A2B_I2C_CMD_WRITE_PERIPH:
                    if ( A2B_NULL == ctx->stk->pal.i2cWrite )
                    {
                        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                                  A2B_FAC_I2C,
                                                  A2B_EC_INTERNAL);
                    }
                    else
                    {

                        result = a2b_i2cWrite(ctx,
                                            nodeI2cAddr,
                                            nWrite,
                                            wBuf);
                    }
                    break;

                case A2B_I2C_CMD_WRITE_READ_MASTER:
                case A2B_I2C_CMD_WRITE_READ_SLAVE:
                case A2B_I2C_CMD_WRITE_READ_PERIPH:
                    if ( A2B_NULL == ctx->stk->pal.i2cWriteRead )
                    {
                        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                                  A2B_FAC_I2C,
                                                  A2B_EC_INTERNAL);
                    }
                    else
                    {
                        /* Get pointer to the next free area in the buffer */
                        result = a2b_i2cWriteRead(ctx,
                                                nodeI2cAddr,
                                                nWrite,
                                                wBuf,
                                                nRead,
                                                rBuf);

                    }
                    break;

                default:
                    result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                            A2B_FAC_I2C,
                                            A2B_EC_INVALID_PARAMETER);

                    A2B_TRACE1((ctx, (A2B_TRC_DOM_STACK |
                                      A2B_TRC_LVL_WARN),
                                      "Unknown I2C command: %lu",
                                      &cmd));
                    break;
            }
        }
		else
		{
			A2B_TRACE1((ctx, (A2B_TRC_DOM_STACK |
                    A2B_TRC_LVL_ERROR),
                    "Failed I2C PrepAccess: %lu",
                    &result));
		}
    }

    return result;

} /* a2b_i2cExecuteAccess */


/*!****************************************************************************
*
* \b   a2b_i2cMasterRead
*
* Reads bytes from the master node. Only the master plugin can issue a
* read request. All other slave plugins and applications requesting to
* read the master node will fail. The read operation will continue at the
* last register offset. This is an synchronous call and will block until the
* operation is complete.
*
* \param   [in]        ctx     The stack context associated with the read.
*
* \param   [in]        nRead   The number of bytes to read from the master
*                              node.
*
* \param   [in,out]    rBuf    A buffer in which to write the results of the
*                              read.
*
* \pre     None
*
* \post    The read buffer holds the contents of the read on success.
*
* \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*          #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cMasterRead
    (
    struct a2b_StackContext*    ctx,
    a2b_UInt16                  nRead,
    void*                       rBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != rBuf) || (nRead == 0u)) )
    {
        /* Only the master plugin is allowed to read/write the master node */
        if ( (ctx->domain != A2B_DOMAIN_PLUGIN) ||
                (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                         A2B_I2C_CMD_READ_MASTER,
                                         A2B_NODEADDR_MASTER,
                                         0u,         /* i2cAddr - unused */
                                         0u,         /* nWrite - unused */
                                         A2B_NULL,  /* wBuf  - unused */
                                         nRead,
                                         rBuf);
        }
    }

    return result;

} /* a2b_i2cMasterRead */


/*!****************************************************************************
* 
*  \b   a2b_i2cMasterWrite
* 
*  Writes bytes to the master node. Only the master plugin can issue a
*  write request. All other slave plugins and applications requesting to
*  write the master node will fail. For the initial write, the first
*  byte of the buffer is treated as the AD2410 register offset by the chip.
*  For subsequent write bytes the chip will auto-increment the register
*  offset with each byte written. This is an synchronous call and will block
*  until the operation is complete.
* 
*  \param   [in]    ctx     The stack context associated with the write.
* 
*  \param   [in]    nWrite  The number of bytes to write.
* 
*  \param   [in]    wBuf    A buffer containing the data to write. The buffer
*                           is of size 'nWrite' bytes.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cMasterWrite
    (
    struct a2b_StackContext*    ctx,
    a2b_UInt16                  nWrite,
    void*                       wBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != wBuf) || (nWrite == 0u)) )
    {
        /* Only the master plugin is allowed to read/write the master node */
        if ( (ctx->domain != A2B_DOMAIN_PLUGIN) ||
                (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                         A2B_I2C_CMD_WRITE_MASTER,
                                         A2B_NODEADDR_MASTER,
                                         0u,         /* i2cAddr - unused */
                                         nWrite,
                                         wBuf,
                                         0u,         /* nRead - unused */
                                         A2B_NULL); /* rBuf - unused */
        }
    }

    return result;

} /* a2b_i2cMasterWrite */


/*!****************************************************************************
* 
*  \b   a2b_i2cMasterWriteRead
* 
*  Writes and then reads bytes from the master node *without* an I2C stop
*  sequence separating the two operations. Instead a repeated I2C start
*  sequence is used as the operation separator. Only the master plugin can
*  issue a write/read request. All other slave plugins and applications
*  requesting to write/read the master node will fail. For the initial write,
*  the first byte of the buffer is treated as the AD2410 register offset by
*  the chip. For subsequent write bytes the chip will auto-increment the
*  register offset with each byte written. The read operation will continue
*  at the last register offset. This is an synchronous call and will block
*  until the operation is complete.
* 
*  \param   [in]    ctx     The stack context associated with the write/read.
* 
*  \param   [in]    nWrite  The number of bytes to write.
* 
*  \param   [in]    wBuf    A buffer containing the data to write. The buffer
*                           is of size 'nWrite' bytes.
* 
*  \param   [in]    nRead   The number of bytes to read from the master node.
* 
*  \param   [in]    rBuf    A buffer in which to write the results of the read.
* 
*  \pre     None
* 
*  \post    The read buffer holds the contents of the read on success.
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cMasterWriteRead
    (
    struct a2b_StackContext*    ctx,
    a2b_UInt16                  nWrite,
    void*                       wBuf,
    a2b_UInt16                  nRead,
    void*                       rBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != wBuf) || (nWrite == 0u)) &&
        ((A2B_NULL != rBuf) || (nRead == 0u)) )
    {
        /* Only the master plugin is allowed to read/write the master node */
        if ( (ctx->domain != A2B_DOMAIN_PLUGIN) ||
                (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                         A2B_I2C_CMD_WRITE_READ_MASTER,
                                         A2B_NODEADDR_MASTER,
                                         0u,         /* i2cAddr - unused */
                                         nWrite,
                                         wBuf,
                                         nRead,
                                         rBuf);
        }
    }

    return result;

} /* a2b_i2cMasterWriteRead */


/*!****************************************************************************
* 
*  \b   a2b_i2cSlaveRead
* 
*  Reads bytes from the slave node. Only the app and master plugin can issue
*  a read request. All other slave plugins requesting to read from a slave
*  node will fail. The read operation will continue at the last register
*  offset. This is an synchronous call and will block until the operation is
*  complete.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   read.
* 
*  \param   [in]        node        The slave node to read.
* 
*  \param   [in]        nRead       The number of bytes to read from the slave
*                                   node.
* 
*  \param   [in,out]    rBuf        A buffer in which to write the results of
*                                   the read.
* 
*  \pre     None
* 
*  \post    On success 'rBuf' holds the data that was read from the slave.
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cSlaveRead
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  nRead,
    void*                       rBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != rBuf) || (nRead == 0u)) )
    {
        /* Only the app and master plugin are allowed to read/write a slave node */
        if ( (ctx->domain != A2B_DOMAIN_APP) &&
            ((ctx->domain != A2B_DOMAIN_PLUGIN) ||
             (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else if ( ((a2b_Bool)(node < 0)) || ((a2b_Bool)(node >= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_I2C,
                                    A2B_EC_INVALID_PARAMETER);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                         A2B_I2C_CMD_READ_SLAVE,
                                         node,
                                         0u,         /* i2cAddr - unused */
                                         0u,         /* nWrite - unused */
                                         A2B_NULL,  /* wBuf  - unused */
                                         nRead,
                                         rBuf);
        }
    }

    return result;

} /* a2b_i2cSlaveRead */


/*!****************************************************************************
* 
*  \b   a2b_i2cSlaveWrite
* 
*  Writes bytes to the slave. Only the app and master plugin can issue a
*  write request. All other slave plugins requesting to write to a slave
*  node will fail. The first byte of the buffer is treated as the AD2410
*  register offset by the chip. For subsequent write bytes the chip will
*  auto-increment the register offset with each byte written. This is an
*  synchronous call and will block until the operation is complete.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   write.
* 
*  \param   [in]        node        The slave node to write.
* 
*  \param   [in]        nWrite      The number of bytes in the 'wBuf' buffer to
*                                   write to the slave.
* 
*  \param   [in]        wBuf        A buffer containing the data to write.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cSlaveWrite
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  nWrite,
    void*                       wBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != wBuf) || (nWrite == 0u)) )
    {
        /* Only the app and master plugin are allowed to read/write a slave node */
        if ( (ctx->domain != A2B_DOMAIN_APP) &&
            ((ctx->domain != A2B_DOMAIN_PLUGIN) ||
             (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else if ( ((a2b_Bool)(node < 0)) || ((a2b_Bool)(node >= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_I2C,
                                    A2B_EC_INVALID_PARAMETER);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                         A2B_I2C_CMD_WRITE_SLAVE,
                                         node,
                                         0u,         /* i2cAddr - unused */
                                         nWrite,
                                         wBuf,
                                         0u,         /* nRead - unused */
                                         A2B_NULL); /* rBuf - unused*/
        }
    }

    return result;

} /* a2b_i2cSlaveWrite */


/*!****************************************************************************
* 
*  \b   a2b_i2cSlaveBroadcastWrite
* 
*  Writes bytes to all the slave nodes. Only the app and master plugin can
*  issue a broadcast write request. All other slave plugins and applications
*  requesting to broadcast write to slave nodes will fail. The first byte of
*  the buffer is treated as the AD2410 register offset by the chip. For
*  subsequently written bytes the chip will auto-increment the register offset
*  with each byte written. This is an synchronous call and will block until the
*  operation is complete.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   write.
* 
*  \param   [in]        nWrite      The number of bytes in the 'wBuf' buffer to
*                                   write to the slave nodes.
* 
*  \param   [in]        wBuf        A buffer containing the data to write.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cSlaveBroadcastWrite
    (
   struct a2b_StackContext* ctx,
   a2b_UInt16               nWrite,
   void*                    wBuf
   )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != wBuf) || (nWrite == 0u)) )
    {
        /* Only the app and master plugin are allowed to read/write a slave node */
        if ( (ctx->domain != A2B_DOMAIN_APP) &&
            ((ctx->domain != A2B_DOMAIN_PLUGIN) ||
             (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                         A2B_I2C_CMD_WRITE_BROADCAST_SLAVE,
                                         0,         /* node - ununsed */
                                         0u,         /* i2cAddr - unused */
                                         nWrite,
                                         wBuf,
                                         0u,         /* nRead - unused */
                                         A2B_NULL); /* rBuf - unused*/
        }
    }

    return result;

} /* a2b_i2cSlaveBroadcastWrite */


/*!****************************************************************************
*
* \b   a2b_i2cSlaveWriteRead
*
* Writes and then reads bytes from the slave node *without* an I2C stop
* sequence separating the two operations. Instead a repeated I2C start
* sequence is used as the operation separator. Only the app and master
* plugin can issue a write request. All other slave plugins requesting
* to write/read a slave node will fail. For the initial write, the first
* byte of the buffer is treated as the AD2410 register offset by the chip.
* For subsequent write bytes the chip will auto-increment the register
* offset with each byte written. The read operation will continue at the
* last register offset. This is an synchronous call and will block until
* the operation is complete.
*
* \param   [in]        ctx         The stack context associated with the
*                                  write/read.
*
* \param   [in]        node        The slave node to write/read.
*
* \param   [in]        nWrite      The number of bytes in the 'wBuf' buffer to
*                                  write to the peripheral.
*
* \param   [in]        wBuf        A buffer containing the data to write.
*
* \param   [in]        nRead       The number of bytes to read from the slave
*                                  node.
*
* \param   [in,out]    rBuf        A buffer in which to write the results of
*                                  the read.
*
* \pre     None
*
* \post    On success 'rBuf' holds the data that was read from the slave.
*
* \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*          #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cSlaveWriteRead
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  nWrite,
    void*                       wBuf,
    a2b_UInt16                  nRead,
    void*                       rBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != wBuf) || (nWrite == 0u)) &&
        ((A2B_NULL != rBuf) || (nRead == 0u)) )
    {
        /* Only the app and master plugin are allowed to read/write a slave node */
        if ( (ctx->domain != A2B_DOMAIN_APP) &&
            ((ctx->domain != A2B_DOMAIN_PLUGIN) ||
             (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else if ( ((a2b_Bool)(node < 0)) || ((a2b_Bool)(node >= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_I2C,
                                    A2B_EC_INVALID_PARAMETER);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                         A2B_I2C_CMD_WRITE_READ_SLAVE,
                                         node,
                                         0u,         /* i2cAddr - unused */
                                         nWrite,
                                         wBuf,
                                         nRead,
                                         rBuf);
        }
    }

    return result;

} /* a2b_i2cSlaveWriteRead */


/*!****************************************************************************
*
* \b   a2b_i2cPeriphRead
*
* Reads bytes from the slave node's peripheral. Only applications, the
* master plugin, or a slave plugin with the same node address can issue an I2C
* read request to a peripheral device attached to the specified node.
* Slave plugins attempting to access peripherals of another node (other than
* their own) will fail. This is a synchronous call and will block until
* the operation is complete.
*
* \param   [in]        ctx     The stack context associated with the read.
*
* \param   [in]        node    The slave node to read.
*
* \param   [in]        i2cAddr The 7-bit I2C address of the peripheral
*                              attached to the slave node's I2C bus.
*
* \param   [in]        nRead   The number of bytes to read from the
*                              peripheral. The 'rBuf' parameter must have
*                              enough space to hold this number of bytes.
*
* \param   [in,out]    rBuf    The buffer in which to write the results of
*                              the read. It's assumed the buffer is sized to
*                              accept 'nRead' bytes of data.
*
* \pre     None
*
* \post    On success 'rBuf' holds the data that was read from the
*          peripheral.
*
* \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*          #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cPeriphRead
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  i2cAddr,
    a2b_UInt16                  nRead,
    void*                       rBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != rBuf) || (nRead == 0u)) )
    {
        if ( ((a2b_Bool)(node < 0)) || ((a2b_Bool)(node >= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_I2C,
                                    A2B_EC_INVALID_PARAMETER);
        }
        /* Only applications, the master plugin, or a slave plugin with
         * the same node address is allowed to access attached peripherals.
         */
        else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
                (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
                (ctx->ccb.plugin.nodeSig.nodeAddr != node) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                          A2B_I2C_CMD_READ_PERIPH,
                                          node,
                                          i2cAddr,
                                          0u,         /* nWrite - unused */
                                          A2B_NULL,  /* wBuf  - unused */
                                          nRead,
                                          rBuf);
        }
    }

    return result;

} /* a2b_i2cPeriphRead */


/*!****************************************************************************
* 
*  \b   a2b_i2cPeriphWrite
* 
*  Writes bytes to the slave node's peripheral. Only applications, the
*  master plugin, or a slave plugin with the same node address can issue an I2C
*  write request to a peripheral device attached to the specified node.
*  Slave plugins attempting to access peripherals of another node (other than
*  their own) will fail. This is a synchronous call and will block until
*  the operation is complete.
* 
*  \param   [in]        ctx     The stack context associated with the write.
* 
*  \param   [in]        node    The slave node to write.
* 
*  \param   [in]        i2cAddr The 7-bit I2C address of the peripheral
*                               attached to the slave node's I2C bus.
* 
*  \param   [in]        nWrite  The number of bytes in the 'wBuf' buffer to
*                               write to the peripheral.
* 
*  \param   [in]        wBuf    A buffer containing the data to write. The
*                               amount of data to write is specified by the
*                               'nWrite' parameter.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cPeriphWrite
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  i2cAddr,
    a2b_UInt16                  nWrite,
    void*                       wBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != wBuf) || (nWrite == 0u)) )
    {

	    if((a2b_Bool)(node  == A2B_NODEADDR_MASTER))
    	{
    		return(a2b_i2cGenericWrite(ctx,node,i2cAddr,nWrite,wBuf));
    	}
        if ( ((a2b_Bool)(node < 0)) || ((a2b_Bool)(node >= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_I2C,
                                    A2B_EC_INVALID_PARAMETER);
        }
        /* Only applications, the master plugin, or a slave plugin with
         * the same node address is allowed to access attached peripherals.
         */
        else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
                (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
                (ctx->ccb.plugin.nodeSig.nodeAddr != node) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                          A2B_I2C_CMD_WRITE_PERIPH,
                                          node,
                                          i2cAddr,
                                          nWrite,
                                          wBuf,
                                          0u,            /* nRead - unused */
                                          A2B_NULL);    /* rBuf - unused */
        }
    }

    return result;

} /* a2b_i2cPeriphWrite */


/*!****************************************************************************
* 
*  \b   a2b_i2cPeriphWriteRead
* 
*  Writes and then reads bytes from the slave node's peripheral *without* an
*  I2C stop sequence separating the two operations. Instead a repeated I2C
*  start sequence is used as the operation separator. Only applications, the
*  master plugin, or a slave plugin with the same node address can issue an I2C
*  write/read request to a peripheral device attached to the specified node.
*  Slave plugins attempting to access peripherals of another node (other than
*  their own) will fail. This is a synchronous call and will block until
*  the operation is complete.
* 
*  \param   [in]        ctx     The stack context associated with the
*                               write/read.
* 
*  \param   [in]        node    The slave node to write/read.
* 
*  \param   [in]        i2cAddr The 7-bit I2C address of the peripheral
*                               attached to the slave node's I2C bus.
* 
*  \param   [in]        nWrite  The number of bytes in the 'wBuf' buffer to
*                               write to the peripheral.
* 
*  \param   [in]        wBuf    A buffer containing the data to write. The
*                               amount of data to write is specified by the
*                               'nWrite' parameter.
* 
*  \param   [in]        nRead   The number of bytes to read from the
*                               peripheral. The 'rBuf' parameter must have
*                               enough space to hold this number of bytes.
* 
*  \param   [in,out]    rBuf    The buffer in which to write the results of
*                               the read. It's assumed the buffer is sized to
*                               accept 'nRead' bytes of data.
* 
*  \pre     None
* 
*  \post    On success 'rBuf' holds the data that was read from the
*           peripheral.
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_i2cPeriphWriteRead
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  i2cAddr,
    a2b_UInt16                  nWrite,
    void*                       wBuf,
    a2b_UInt16                  nRead,
    void*                       rBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != rBuf) || (nRead == 0u)) &&
        ((A2B_NULL != wBuf) || (nWrite == 0u)) )
    {
    	if((a2b_Bool)(node  == A2B_NODEADDR_MASTER))
    	{
    		return(a2b_i2cGenericWriteRead(ctx,node,i2cAddr,nWrite,wBuf,nRead,rBuf));
    	}

        if ( ((a2b_Bool)(node < 0)) || ((a2b_Bool)(node >= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_I2C,
                                    A2B_EC_INVALID_PARAMETER);
        }
        /* Only applications, the master plugin, or a slave plugin with
         * the same node address is allowed to access attached peripherals.
         */
        else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
                (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
                (ctx->ccb.plugin.nodeSig.nodeAddr != node) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else
        {
            result = a2b_i2cExecuteAccess(ctx,
                                          A2B_I2C_CMD_WRITE_READ_PERIPH,
                                          node,
                                          i2cAddr,
                                          nWrite,
                                          wBuf,
                                          nRead,
                                          rBuf);
        }
    }

    return result;

} /* a2b_i2cPeriphWriteRead */

/*!****************************************************************************
*
*  \b   a2b_i2cGenericWrite
*
*  Writes bytes to the any I2C device. Only applications, or the
*  master plugin can issue an I2C write request to a peripheral device
*  attached to the specified node.
*  This is a synchronous call and will block until the operation is
*  complete.
*
*  \param   [in]        ctx     The stack context associated with the write.
*
*  \param   [in]        node    The slave node to write.
*
*  \param   [in]        i2cAddr The 7-bit I2C address of the peripheral
*                               attached to the slave node's I2C bus.
*
*  \param   [in]        nWrite  The number of bytes in the 'wBuf' buffer to
*                               write to the peripheral.
*
*  \param   [in]        wBuf    A buffer containing the data to write. The
*                               amount of data to write is specified by the
*                               'nWrite' parameter.
*
*  \pre     None
*
*  \post    None
*
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
static a2b_HResult
a2b_i2cGenericWrite
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  i2cAddr,
    a2b_UInt16                  nWrite,
    void*                       wBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != wBuf) || (nWrite == 0u)) )
    {
		 if ( ((a2b_Bool)(node < A2B_NODEADDR_MASTER)) || ((a2b_Bool)(node >= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_I2C,
                                    A2B_EC_INVALID_PARAMETER);
        }
        /* Only applications, the master plugin is allowed to access attached peripherals. */
        else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
                (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER))
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else
        {
            result = a2b_i2cWrite(ctx, i2cAddr, nWrite, wBuf);
        }
    }

    return result;

} /* a2b_i2cGenericWrite */

/*!****************************************************************************
*
*  \b   a2b_i2cGenericWriteRead
*
*  Writes bytes to the any I2C device. Only applications, or the
*  master plugin can issue an I2C write request to a peripheral device
*  attached to the specified node.
*  This is a synchronous call and will block until the operation is
*  complete.
*
*  \param   [in]        ctx     The stack context associated with the write.
*
*  \param   [in]        node    The slave node to write.
*
*  \param   [in]        i2cAddr The 7-bit I2C address of the peripheral
*                               attached to the slave node's I2C bus.
*
*  \param   [in]        nWrite  The number of bytes in the 'wBuf' buffer to
*                               write to the peripheral.
*
*  \param   [in]        wBuf    A buffer containing the data to write. The
*                               amount of data to write is specified by the
*                               'nWrite' parameter.
*
*  \param   [in]        nRead   The number of bytes to read from the
*                               peripheral. The 'rBuf' parameter must have
*                               enough space to hold this number of bytes.
*
*  \param   [in,out]    rBuf    The buffer in which to write the results of
*                               the read. It's assumed the buffer is sized to
*                               accept 'nRead' bytes of data.
*  \pre     None
*
*  \post    None
*
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
*
******************************************************************************/
static a2b_HResult
a2b_i2cGenericWriteRead
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   node,
    a2b_UInt16                  i2cAddr,
    a2b_UInt16                  nWrite,
    void*                       wBuf,
    a2b_UInt16                  nRead,
    void*                       rBuf
    )
{
    a2b_HResult result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_INVALID_PARAMETER);

    if ( (A2B_NULL != ctx) &&
        ((A2B_NULL != rBuf) || (nRead == 0u)) &&
        ((A2B_NULL != wBuf) || (nWrite == 0u)) )
    {
        if ( ((a2b_Bool)(node < A2B_NODEADDR_MASTER)) ||
        		((a2b_Bool)(node >= (a2b_Int16)A2B_CONF_MAX_NUM_SLAVE_NODES)) )
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_I2C,
                                    A2B_EC_INVALID_PARAMETER);
        }
        /* Only applications, the master plugin is allowed to access attached peripherals. */
        else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
                (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER))
        {
            result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_I2C,
                                        A2B_EC_PERMISSION);
        }
        else
        {
            result = a2b_i2cWriteRead(ctx, i2cAddr, nWrite, wBuf, nRead, rBuf);
        }
    }

    return result;

} /* a2b_i2cGenericWrite */


