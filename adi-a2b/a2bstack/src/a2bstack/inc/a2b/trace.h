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
 * \file:   trace.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Definition of system trace macros.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_trace             Trace Module
 *  
 * Definition of system trace macros.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_TRACE_H_
#define A2B_TRACE_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/tracectl.h"

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_trace_defs        Types/Defs
 *  
 * The various defines and data types used within the trace modules.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

/* The trace level is now broken into a domain and a level.  This
 * granularity makes it possible to enable/disable tracing for
 * specific domains to have more targeted tracing.
 */
#define A2B_TRC_LVL_MASK        (0xFFFu)
#define A2B_TRC_DOM_MASK        (0xFFFFF000u)

/*----------------------------------------------------------------------------*/
/** 
 * \name    Basic Trace 
 *  
 * A2B_TRACE#((ctx, LVL, FMT, ...))
 *
 * C89 compatible trace routine. Calls to this macro/function should
 * be written as follows:
 * 
 * \code
 *          A2B_TRACE1((ctx, (A2B_TRC_DOM_SOMETHING | A2B_TRC_LVL_DEBUG),
 *                      "This text is %s", "orange"));
 * \endcode
 */
/*----------------------------------------------------------------------------*/
#ifdef A2B_FEATURE_TRACE
#   if (__STDC_VERSION__ >= 199901L)
#       define A2B_TRACE0(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, __FUNCTION__, __LINE__); \
            a2b_trace0 X; } while ( 0 )
#       define A2B_TRACE1(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, __FUNCTION__, __LINE__); \
            a2b_trace1 X; } while ( 0 )
#       define A2B_TRACE2(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, __FUNCTION__, __LINE__); \
            a2b_trace2 X; } while ( 0 )
#       define A2B_TRACE3(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, __FUNCTION__, __LINE__); \
            a2b_trace3 X; } while ( 0 )
#       define A2B_TRACE4(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, __FUNCTION__, __LINE__); \
            a2b_trace4 X; } while ( 0 )
#       define A2B_TRACE5(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, __FUNCTION__, __LINE__); \
            a2b_trace5 X; } while ( 0 )
#       define A2B_TRACE6(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, __FUNCTION__, __LINE__); \
            a2b_trace6 X; } while ( 0 )
#       define A2B_TRACE7(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, __FUNCTION__, __LINE__); \
            a2b_trace7 X; } while ( 0 )
#       define A2B_TRACE8(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, __FUNCTION__, __LINE__); \
            a2b_trace8 X; } while ( 0 )
#   else
#       define A2B_TRACE0(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, 0, __LINE__); \
            a2b_trace0 X; } while ( 0 )
#       define A2B_TRACE1(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, 0, __LINE__); \
            a2b_trace1 X; } while ( 0 )
#       define A2B_TRACE2(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, 0, __LINE__); \
            a2b_trace2 X; } while ( 0 )
#       define A2B_TRACE3(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, 0, __LINE__); \
            a2b_trace3 X; } while ( 0 )
#       define A2B_TRACE4(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, 0, __LINE__); \
            a2b_trace4 X; } while ( 0 )
#       define A2B_TRACE5(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, 0, __LINE__); \
            a2b_trace5 X; } while ( 0 )
#       define A2B_TRACE6(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, 0, __LINE__); \
            a2b_trace6 X; } while ( 0 )
#       define A2B_TRACE7(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, 0, __LINE__); \
            a2b_trace7 X; } while ( 0 )
#       define A2B_TRACE8(X) \
            do { \
                a2b_tracePrintPrefix(a2b_traceIsEnabled X, __FILE__, 0, __LINE__); \
            a2b_trace8 X; } while ( 0 )
#   endif
#else
#ifdef _TESSY_NO_A2B_FEATURE_TRACE_
#   define A2B_TRACE0(X)
#   define A2B_TRACE1(X)
#   define A2B_TRACE2(X)
#   define A2B_TRACE3(X)
#   define A2B_TRACE4(X)
#   define A2B_TRACE5(X)
#   define A2B_TRACE6(X)
#   define A2B_TRACE7(X)
#   define A2B_TRACE8(X)
#else	/* _TESSY_NO_A2B_FEATURE_TRACE_ */
#   define A2B_TRACE0(X) do { } while ( 0 )
#   define A2B_TRACE1(X) do { } while ( 0 )
#   define A2B_TRACE2(X) do { } while ( 0 )
#   define A2B_TRACE3(X) do { } while ( 0 )
#   define A2B_TRACE4(X) do { } while ( 0 )
#   define A2B_TRACE5(X) do { } while ( 0 )
#   define A2B_TRACE6(X) do { } while ( 0 )
#   define A2B_TRACE7(X) do { } while ( 0 )
#   define A2B_TRACE8(X) do { } while ( 0 )
#endif	/* _TESSY_NO_A2B_FEATURE_TRACE_ */
#endif
/** \} */

/*======================= D A T A T Y P E S =======================*/

/* Forward declarations */
struct a2b_StackContext;

/** \} -- a2bstack_trace_defs */

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_trace_funct       Functions
 *  
 * These functions support the public trace API on an A2B node.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_BEGIN_DECLS

#ifdef A2B_FEATURE_TRACE

A2B_DSO_PUBLIC struct a2b_StackContext* A2B_CALL a2b_traceIsEnabled(
                                        struct a2b_StackContext* ctx,
                                        a2b_UInt32 level, ...);

A2B_DSO_PUBLIC void A2B_CALL a2b_tracePrintPrefix(
                            struct a2b_StackContext* ctx,
                            const a2b_Char*          file, 
                            const a2b_Char*          funcName,
                            a2b_UInt32               line);

A2B_DSO_PUBLIC void A2B_CALL a2b_trace0(
                struct a2b_StackContext* ctx, a2b_UInt32 level,
                const a2b_Char* fmt);
A2B_DSO_PUBLIC void A2B_CALL a2b_trace1(
                struct a2b_StackContext* ctx, a2b_UInt32 level,
                const a2b_Char* fmt, void* a1);
A2B_DSO_PUBLIC void A2B_CALL a2b_trace2(
                struct a2b_StackContext* ctx, a2b_UInt32 level,
                const a2b_Char* fmt, void* a1, void* a2);
A2B_DSO_PUBLIC void A2B_CALL a2b_trace3(
                struct a2b_StackContext* ctx, a2b_UInt32 level,
                const a2b_Char* fmt, void* a1, void* a2, void* a3);
A2B_DSO_PUBLIC void A2B_CALL a2b_trace4(
                struct a2b_StackContext* ctx, a2b_UInt32 level,
                const a2b_Char* fmt, void* a1, void* a2, void* a3, void* a4);
A2B_DSO_PUBLIC void A2B_CALL a2b_trace5(
                struct a2b_StackContext* ctx, a2b_UInt32 level,
                const a2b_Char* fmt, void* a1, void* a2, void* a3, void* a4,
                void* a5);
A2B_DSO_PUBLIC void A2B_CALL a2b_trace6(
                struct a2b_StackContext* ctx, a2b_UInt32 level,
                const a2b_Char* fmt, void* a1, void* a2, void* a3, void* a4,
                void* a5, void* a6);
A2B_DSO_PUBLIC void A2B_CALL a2b_trace7(
                struct a2b_StackContext* ctx, a2b_UInt32 level,
                const a2b_Char* fmt, void* a1, void* a2, void* a3, void* a4,
                void* a5, void* a6, void* a7);
A2B_DSO_PUBLIC void A2B_CALL a2b_trace8(
                struct a2b_StackContext* ctx, a2b_UInt32 level,
                const a2b_Char* fmt, void* a1, void* a2, void* a3, void* a4,
                void* a5, void* a6, void* a7, void* a8);

#endif /* A2B_FEATURE_TRACE */

/** \} -- a2bstack_trace_funct */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_trace */

#endif /* Guard for A2B_TRACE_H_ */
