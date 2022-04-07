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
 * \file:   trace_priv.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Definition of private system trace macros.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \ingroup  a2bstack_trace
 * \defgroup a2bstack_trace_priv        \<Private\> 
 * \private 
 *  
 * This defines the trace API's that are private to the stack.
 *  
 * \{ */
/*============================================================================*/

#ifndef A2B_TRACE_PRIV_H_
#define A2B_TRACE_PRIV_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/conf.h"
#include "a2b/stringbuffer.h"

/*======================= D E F I N E S ===========================*/


/*======================= D A T A T Y P E S =======================*/

/* Forward declarations */
struct a2b_StackContext;

/**
 * The definition of a trace channel.
 */
typedef struct a2b_TraceChannel
{
    a2b_Handle                  hnd;
    a2b_UInt32                  mask;
    a2b_Char                    buf[A2B_CONF_TRACE_BUF_SIZE];
    a2b_StringBuffer            strBuf;
    struct a2b_StackContext*    ctx;
} a2b_TraceChannel;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_BEGIN_DECLS

#ifdef A2B_FEATURE_TRACE

A2B_EXPORT A2B_DSO_LOCAL struct a2b_TraceChannel* a2b_traceAlloc(
                                            struct a2b_StackContext*    ctx,
                                            const a2b_Char*             name);

A2B_EXPORT A2B_DSO_LOCAL void a2b_traceFree(struct a2b_TraceChannel* chan);

#endif /* A2B_FEATURE_TRACE */

A2B_END_DECLS

/*======================= D A T A =================================*/


/** \} -- a2bstack_trace_priv */

#endif /* Guard for A2B_TRACE_PRIV_H_ */
