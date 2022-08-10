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
 * \file:   diag.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the A2B stack diagnostic API.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \ingroup  a2bstack_diag 
 * \defgroup a2bstack_diag_priv         \<Private\> 
 * \private 
 *  
 * This defines diagnostic API's that are private to the stack.
 */
/*============================================================================*/

/*======================= I N C L U D E S =========================*/

#include "a2b/diag.h"
#include "a2b/util.h"
#include "a2b/trace.h"
#include "a2b/error.h"
#include "a2b/regdefs.h"
#include "a2b/i2c.h"
#include "a2b/defs.h"
#include "stack_priv.h"
#include "stackctx.h"


/*======================= D E F I N E S ===========================*/


/*======================= L O C A L  P R O T O T Y P E S  =========*/
static a2b_Bool a2b_diagIsValidRegister(a2b_Int16 nodeAddr,
    a2b_UInt8   reg, a2b_UInt8   value, a2b_Char    mode);


/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/


/*!****************************************************************************
*  \ingroup     a2bstack_diag_priv
*  
*  \b           a2b_diagIsValidRegister
* 
*  This function validates the diagnostic register to make sure it's an
*  acceptable register and is being accessed correctly based on the mode
*  (read/write) and the node being addressed.
* 
*  \param   [in]        nodeAddr    The address of the node to access. If set
*                                   to #A2B_NODEADDR_NOTUSED then the write
*                                   will be broadcast to all nodes (master and
*                                   slave). ([0..n] or #A2B_NODEADDR_NOTUSED)
* 
*  \param   [in]        reg         The register offset of an AD2410
*                                   diagnostic related register. Valid
*                                   registers include:                      <br>
*                                       #A2B_REG_BECCTL                     <br>
*                                       #A2B_REG_BECNT                      <br>
*                                       #A2B_REG_TESTMODE                   <br>
*                                       #A2B_REG_ERRMGMT                    <br>
*                                       #A2B_REG_I2STEST                    <br>
*                                       #A2B_REG_RAISE                      <br>
*                                       #A2B_REG_GENERR                     <br>
*                                       #A2B_REG_ERRCNT0-3 (read-only)
* 
*  \param   [in]        value       The 8-bit value to write to the register.
*                                   If in read mode then this value is ignored.
* 
*  \param   [in]        mode        Either 'w' for a register write or 'r'
*                                   for a read.
* 
*  \pre     None
* 
*  \post    None
* 
*  \return  Returns A2B_TRUE if the register access is valid or A2B_FALSE
*           otherwise.
* 
******************************************************************************/
static a2b_Bool
a2b_diagIsValidRegister
    (
    a2b_Int16   nodeAddr,
    a2b_UInt8   reg,
    a2b_UInt8   value,
    a2b_Char    mode
    )
{
    a2b_Bool isValid;

    /* First make sure it's a diagnostics related register */
    switch ( reg )
    {
        case A2B_REG_BECCTL:
        case A2B_REG_BECNT:
        case A2B_REG_TESTMODE:
        case A2B_REG_ERRMGMT:
        case A2B_REG_I2STEST:
        case A2B_REG_RAISE:
        case A2B_REG_GENERR:
            isValid = A2B_TRUE;
            break;
		case A2B_REG_VENDOR:
		case A2B_REG_NODE:
        case A2B_REG_ERRCNT0:
        case A2B_REG_ERRCNT1:
        case A2B_REG_ERRCNT2:
        case A2B_REG_ERRCNT3:
            isValid = ( mode == 'r' ) ? A2B_TRUE : A2B_FALSE;
            break;

        default:
            isValid = A2B_FALSE;
            break;
    }

    /* If the register is valid then for 'writes' we need to do extra checks */
    if ( isValid && (mode == 'w') )
    {
        if ( reg == A2B_REG_RAISE )
        {
            switch ( value )
            {
                case A2B_ENUM_INTTYPE_IO0PND:
                case A2B_ENUM_INTTYPE_IO1PND:
                case A2B_ENUM_INTTYPE_IO2PND:
                    if ( (A2B_NODEADDR_MASTER == nodeAddr) ||
                        (A2B_NODEADDR_NOTUSED == nodeAddr) )
                    {
                        /* These values are only applicable to slave nodes */
                        isValid = A2B_FALSE;
                    }
                    break;

                case A2B_ENUM_INTTYPE_DSCDONE:
                case A2B_ENUM_INTTYPE_I2CERR:
                case A2B_ENUM_INTTYPE_ICRCERR:
                case A2B_ENUM_INTTYPE_SLAVE_INTTYPE_ERR:
                case A2B_ENUM_INTTYPE_STANDBY_DONE:
                case A2B_ENUM_INTTYPE_MSTR_RUNNING:
                    if ( A2B_NODEADDR_MASTER != nodeAddr )
                    {
                        isValid = A2B_FALSE;
                    }
                    break;

                default:
                    break;
            }
        }
        else if ( reg == A2B_REG_GENERR )
        {
            if ( (value & A2B_BITM_GENERR_GENICRCERR) &&
                ((A2B_NODEADDR_MASTER == nodeAddr) ||
                (A2B_NODEADDR_NOTUSED == nodeAddr)) )
            {
                /* Can only generate Interrupt Frame CRC Errors for slave
                 * nodes.
                 */
                isValid = A2B_FALSE;
            }
        }
        else
        {
            /* Completing the control statement */
        }
    }

    return isValid;

} /* a2b_diagIsValidRegister */


/*!****************************************************************************
* 
*  \b   a2b_diagWriteReg
* 
*  This function provides a low-level interface for writing a value to a set
*  of diagnostic-related AD2410 registers. Only the application, a master
*  plugin, or slave plugin managing its own node, can write to the diagnostic
*  registers. These register are either written per node (slave plugin only)
*  or broadcast to all nodes. It is assumed access to slave nodes will only
*  occur <b>after</b> network discovery and configuration.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   diagnostic write request.
* 
*  \param   [in]        nodeAddr    The address of the node to access. If set
*                                   to #A2B_NODEADDR_NOTUSED then the write
*                                   will be broadcast to all nodes (master and
*                                   slave). If the register is #A2B_REG_TESTMODE
*                                   the broadcast behavior is the default.
*                                   Slave plugins can only write to nodes they
*                                   are managing.
*                                   ([0..n] or #A2B_NODEADDR_NOTUSED)
* 
*  \param   [in]        reg         The register offset of an AD2410
*                                   diagnostic related register. Valid
*                                   registers include:                      <br>
*                                       #A2B_REG_BECCTL                     <br>
*                                       #A2B_REG_BECNT                      <br>
*                                       #A2B_REG_TESTMODE                   <br>
*                                       #A2B_REG_ERRMGMT                    <br>
*                                       #A2B_REG_I2STEST                    <br>
*                                       #A2B_REG_RAISE                      <br>
*                                       #A2B_REG_GENERR
* 
*  \param   [in]        value       The 8-bit value to write to the register.
*                                   Please refer to a2b/regdefs.h for
*                                   convenient definitions for common bit
*                                   masks.
* 
*  \pre     A2B discovery has already been conducted successfully.
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_diagWriteReg
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt8                   reg,
    a2b_UInt8                   value
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[2];
    struct a2b_StackContext* mCtx;
    a2b_Bool bResult;

    /* If the basic input arguments are incorrect then ... */
    bResult = a2b_diagIsValidRegister(nodeAddr, reg, value, 'w');
    if ((A2B_NULL == ctx) || (!bResult) ||
        ((nodeAddr != A2B_NODEADDR_NOTUSED) &&
        (!A2B_VALID_NODEADDR(nodeAddr))))
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_DIAG,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, master plugin, or slave plugin accessing its own
     * node are allowed to write to diagnostic registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_DIAG,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* Need a master plugin context to do I2C calls */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            /* This should *never* happen */
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_DIAG,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            wBuf[0] = reg;
            wBuf[1] = value;

            /* If this is a broadcast write */
            if ( (A2B_NODEADDR_NOTUSED == nodeAddr) ||
                (reg == A2B_REG_TESTMODE) )
            {
                /* No need to Write master first, broadcast to slave will ensure that registers are written */
                if ( A2B_SUCCEEDED(result) )
                {
                    /* Now broadcast it to all the slave nodes */
                    result = a2b_i2cSlaveBroadcastWrite(mCtx,
                                            (a2b_UInt16)A2B_ARRAY_SIZE(wBuf), &wBuf[0]);
                }
            }
            else if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWrite(mCtx, (a2b_UInt16)A2B_ARRAY_SIZE(wBuf),
                                            &wBuf[0]);
            }
            /* Else a write to a single slave node */
            else
            {
                result = a2b_i2cSlaveWrite(mCtx, nodeAddr,
                		(a2b_UInt16)A2B_ARRAY_SIZE(wBuf), &wBuf[0]);

            }
        }
    }

    return result;

} /* a2b_diagWriteReg */


/*!****************************************************************************
* 
*  \b   a2b_diagReadReg
* 
*  This function provides a low-level interface for reading a value from a set
*  of diagnostic-related AD2410 registers. These registers can be read
*  from *any* domain (e.g. application, master plugin, or slave plugin). For
*  slave plugins, however, read access is restricted to the node it's managing.
*  These registers are addressed per node so it is assumed access to a slave
*  node will only occur <b>after</b> network discovery and configuration.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   diagnostic read request.
* 
*  \param   [in]        nodeAddr    The address of the node to access. [0..n]
* 
*  \param   [in]        reg         The register offset of an AD2410
*                                   diagnostic related register. Valid
*                                   registers include:                      <br>
*                                       #A2B_REG_BECCTL                     <br>
*                                       #A2B_REG_BECNT                      <br>
*                                       #A2B_REG_TESTMODE                   <br>
*                                       #A2B_REG_ERRCNT0                    <br>
*                                       #A2B_REG_ERRCNT1                    <br>
*                                       #A2B_REG_ERRCNT2                    <br>
*                                       #A2B_REG_ERRCNT3                    <br>
*                                       #A2B_REG_ERRMGMT                    <br>
*                                       #A2B_REG_I2STEST                    <br>
*                                       #A2B_REG_RAISE                      <br>
*                                       #A2B_REG_GENERR
* 
*  \param   [in,out]    value       A pointer to an 8-bit variable to hold
*                                   the read register value.
* 
*  \pre     A2B discovery has already been conducted successfully.
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_diagReadReg
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt8                   reg,
    a2b_UInt8*                  value
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[1];
    struct a2b_StackContext* mCtx;
    a2b_Bool bResult;

    /* If the basic input arguments are incorrect then ... */
    bResult = a2b_diagIsValidRegister(nodeAddr, reg, 0u, 'r');
    if ( (A2B_NULL == ctx) || (!bResult) ||
        (A2B_NULL == value) ||
        (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_DIAG,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to read diagnostic registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_DIAG,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* Need a master plugin context to do I2C calls */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            /* This should *never* happen */
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_DIAG,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            wBuf[0] = reg;

            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWriteRead(mCtx, 1u, &wBuf[0u], 1u, value);
            }
            /* Else a write/read to a single slave node */
            else
            {
                result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr,
                                            1u, &wBuf[0u], 1u, value);

            }
        }
    }

    return result;

} /* a2b_diagReadReg */


/*!****************************************************************************
* 
*  \b   a2b_diagReadPrbsErrCnt
* 
*  This is a convenience function to read the #A2B_REG_ERRCNT0-4 registers
*  and combine them into a single 32-bit count. These registers can be read
*  from *any* domain (e.g. application, master plugin, or slave plugin). For
*  slave plugins, however, read access is restricted to the node it's managing.
*  These registers are addressed per node so it is assumed access to a slave
*  node will only occur <b>after</b> network discovery and configuration.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   diagnostic read request.
* 
*  \param   [in]        nodeAddr    The address of the node to access.
* 
*  \param   [in,out]    count       A pointer to a 32-bit variable to hold
*                                   the A2B PRBS error count.
* 
*  \pre     A2B discovery has already been conducted successfully.
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_diagReadPrbsErrCnt
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt32*                 count
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[1u];
    a2b_Byte rBuf[4u];
    struct a2b_StackContext* mCtx;

    /* If the basic input arguments are incorrect then ... */
    if ( (A2B_NULL == ctx) ||
        (A2B_NULL == count) ||
        (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_DIAG,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to read diagnostic registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_DIAG,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* Need a master plugin context to do I2C calls */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            /* This should *never* happen */
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_DIAG,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            wBuf[0] = (a2b_Byte)A2B_REG_ERRCNT0;

            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWriteRead(mCtx, 1u, &wBuf[0], 4u, &rBuf[0u]);
            }
            /* Else write/read from a slave node */
            else
            {
                result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr,
                                            1u, &wBuf[0u], 4u, &rBuf[0u]);

            }

            if ( A2B_SUCCEEDED(result) )
            {
                *count = (a2b_UInt32)rBuf[0u] |
                         ((a2b_UInt32)rBuf[1u] << 8u) |
                         ((a2b_UInt32)rBuf[2u] << 16u) |
                         ((a2b_UInt32)rBuf[3u] << 24u);
            }
        }
    }

    return result;

} /* a2b_diagReadPrbsErrCnt */


/*!****************************************************************************
* 
*  \b   a2b_diagGetRegDump
* 
*  This is a convenience function that can be used to generate a register
*  dump of an A2B node. The values are stored (starting from AD2410 register
*  offset) into the provided buffer up to the specified maximum number
*  of registers. The number of registers actually read is then stored and
*  returned in 'numRegs' variable. A register dump can be
*  made from *any* domain (e.g. application, master plugin, or slave plugin).
*  For slave plugins, however, read access is restricted to the node it's
*  managing. When accessing slave nodes it is assumed network discovery and
*  configuration has already occurred.
*  
*  \attention
*  The read of the Interrupt Type Register (#A2B_REG_INTTYPE) is skipped so
*  that it isn't inadvertently cleared. In its place a zero (0) is written
*  in the register buffer. 
*  
*  \param   [in]        ctx         The stack context associated with the
*                                   register dump request.
* 
*  \param   [in]        nodeAddr    The address of the node to access.
* 
*  \param   [in]        regOffset   The starting register offset (from zero)
*                                   where the dump will begin.
* 
*  \param   [in,out]    regs        A pointer to the buffer to store the
*                                   register dump starting at offset 0 of
*                                   the AD2410.
* 
*  \param   [in,out]    numRegs     On input this should specify the size of
*                                   of the register buffer. On output this
*                                   function will store the number of registers
*                                   actually written to the buffer.
* 
*  \pre     A2B discovery has already been conducted successfully before
*           accessing a slave nodes registers.
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_diagGetRegDump
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt8                   regOffset,
    a2b_UInt8*                  regs,
    a2b_UInt16*                 numRegs
    )
{

    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[1];
    a2b_Byte rBuf[2];
    a2b_UInt8 maxRegOffset;
    a2b_Int16 segLen1;
    a2b_Int16 segLen2;
    a2b_Int16 segOffset2;
    a2b_Int16 regIdx;
    a2b_UInt8 nTempVar;
    struct a2b_StackContext* mCtx;

    /* If the basic input arguments are incorrect then ... */
    if ( (A2B_NULL == ctx) ||
        (A2B_NULL == regs) ||
        (A2B_NULL == numRegs) ||
        (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_DIAG,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to read diagnostic registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_DIAG,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* Need a master plugin context to do I2C calls */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            /* This should *never* happen */
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_DIAG,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            wBuf[0] = A2B_REG_VENDOR;
            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
               result = a2b_i2cMasterWriteRead(mCtx, 1u, &wBuf[0],
                                                       2u, &rBuf[0]);
            }
            else
            {
               result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr, 1u, &wBuf[0],
                                                       2u, &rBuf[0]);
            }

            if ( A2B_SUCCEEDED(result) )
            {
                if ( A2B_IS_AD242X_CHIP(rBuf[0], rBuf[1]) )
                {
                    maxRegOffset = A2B_REG_MBOX1B3;
                }
                else
                {
                    maxRegOffset = A2B_REG_GENERR;
                }

                if ( regOffset > maxRegOffset )
                {
                    result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_DIAG,
                                              A2B_EC_INVALID_PARAMETER);
                }
            }

            if ( A2B_SUCCEEDED(result) )
            {
                /* Figure out how many registers we can actually store */
                *numRegs = A2B_MIN(*numRegs, ((a2b_UInt16)maxRegOffset + (a2b_UInt16)1u - (a2b_UInt16)regOffset));

                /* If there is no room to store any registers */
                if ( 0u == *numRegs )
                {
                    result = A2B_RESULT_SUCCESS;
                }
                else
                {
                    /* Zero out the buffer */
                    (void)a2b_memset(regs, 0, (a2b_Size)*numRegs);

                    /* Figure out how much we need to read *BEFORE* the
                     * A2B_REG_INTTYPE register.
                     */
                    segLen1 = 0;
                    if ( regOffset <= A2B_REG_INTTYPE )
                    {
                    	nTempVar = (a2b_UInt8)A2B_REG_INTTYPE - regOffset;
                        segLen1 = (a2b_Int16)A2B_MIN((a2b_UInt16)nTempVar,
                                             *numRegs);
                    }

                    /* If there is some registers to read before
                     * INTTYPE then ...
                     */
                    if ( segLen1 > 0 )
                    {
                        wBuf[0] = regOffset;
                        if ( A2B_NODEADDR_MASTER == nodeAddr )
                        {
                           result = a2b_i2cMasterWriteRead(mCtx, 1u, &wBuf[0],
                                                       (a2b_UInt16)segLen1, &regs[0]);
                        }
                        /* Else read from a slave node */
                        else
                        {
                           result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr,
                                           1u, &wBuf[0], (a2b_UInt16)segLen1, &regs[0]);

                        }
                    }

                    /* Figure how many registers to read *AFTER* the
                     * A2B_REG_INTTYPE register.
                     */
                    segOffset2 = (a2b_Int16)A2B_REG_INTTYPE + (a2b_Int16)1;
                    if ( regOffset > A2B_REG_INTTYPE )
                    {
                        segOffset2 = (a2b_Int16)regOffset;
                        segLen2 = (a2b_Int16)*numRegs;
                        regIdx = 0;
                    }
                    else
                    {
                        segLen2 = ((a2b_Int16)*numRegs - segLen1 - 1);
                        regIdx = segLen1 + 1;
                    }

                    if ( segLen2 > 0 )
                    {
                        wBuf[0] = (a2b_Byte)segOffset2;
                        if ( A2B_NODEADDR_MASTER == nodeAddr )
                        {
                           result = a2b_i2cMasterWriteRead(mCtx, 1u, &wBuf[0],
                                                   (a2b_UInt16)segLen2, &regs[regIdx]);
                        }
                        /* Else read from a slave node */
                        else
                        {
                           result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr,
                                       1u, &wBuf[0], (a2b_UInt16)segLen2, &regs[regIdx]);

                        }
                    }
                }
            }
        }
    }

    return result;

} /* a2b_diagGetRegDump */

