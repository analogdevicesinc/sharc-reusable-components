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
 * \file:   seqchart.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Definition of sequence chart macros.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_seqchart          Sequence Chart Module
 *  
 * Definition of sequence chart functions and supporting macros.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_SEQ_CHART_H_
#define A2B_SEQ_CHART_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/stack.h"
#include "a2b/seqchartctl.h"
#include "a2b/stringbuffer.h"

/*============================================================================*/
/** 
 * \defgroup a2bstack_seqchart_macros   Macros
 *  
 * Definition of sequence chart macros.
 *
 * \{ */
/*============================================================================*/

/*======================= D E F I N E S ===========================*/

/*----------------------------------------------------------------------------*/
/** 
 * \name    Sequence Controls 
 *  
 * This is a simple cover macro to make checking for a level 
 * being enabled easier/cleaner. 
 */
/*----------------------------------------------------------------------------*/
#ifdef A2B_FEATURE_SEQ_CHART
#   define A2B_SEQ_CHART_ENABLED(ctx, level) \
        a2b_seqChartIsEnabled ( ctx, \
                                A2B_SEQ_CHART_ENTITY_APP, \
                                A2B_SEQ_CHART_ENTITY_APP, \
                                A2B_SEQ_CHART_COMM_REQUEST, \
                                level )
#else
#   define A2B_SEQ_CHART_ENABLED(ctx, level) A2B_FALSE
#endif

/*----------------------------------------------------------------------------*/
/** 
 * \name    Basic Sequence Req/Resp
 * 
 * C89 compatible sequence chart routines. Calls to this macro/function should
 * be written as follows:
 * 
 * \code
 * A2B_SEQ_CHART#((ctx,
 *               A2B_SEQ_CHART_ENTITY_APP,
 *               A2B_SEQ_CHART_ENTITY_STACK,
 *               A2B_SEQ_CHART_COMM_REQUEST,
 *               A2B_SEQ_CHART_LEVEL_3,
 *               fmt, ...))
 * \endcode 
 *  
 * This invocation demonstrates a *request* from the "App" to the "Stack" at
 * level 3 with time-stamping.
 */
/*----------------------------------------------------------------------------*/
#ifdef A2B_FEATURE_SEQ_CHART
#   define A2B_SEQ_CHART0(X) \
        do { \
            if ( a2b_seqChartIsEnabled X ) \
            {a2b_seqChart0 X; } \
            }while ( 0 )
#   define A2B_SEQ_CHART1(X) \
        do { \
            if ( a2b_seqChartIsEnabled X ) \
            {a2b_seqChart1 X; }   \
            }while ( 0 )
#   define A2B_SEQ_CHART2(X) \
        do { \
            if ( a2b_seqChartIsEnabled X ) \
            {a2b_seqChart2 X; }   \
            }while ( 0 )
#   define A2B_SEQ_CHART3(X) \
        do { \
            if ( a2b_seqChartIsEnabled X ) \
            {a2b_seqChart3 X; } \
        }while ( 0 )
#   define A2B_SEQ_CHART4(X) \
        do { \
            if ( a2b_seqChartIsEnabled X ) \
            {a2b_seqChart4 X; } \
        }while ( 0 )
#   define A2B_SEQ_CHART5(X) \
        do { \
            if ( a2b_seqChartIsEnabled X ) \
            {a2b_seqChart5 X; } \
        }while ( 0 )
#   define A2B_SEQ_CHART6(X) \
        do { \
            if ( a2b_seqChartIsEnabled X ) \
            {a2b_seqChart6 X; } \
        }while ( 0 )
#   define A2B_SEQ_CHART7(X) \
        do { \
            if ( a2b_seqChartIsEnabled X ) \
            {a2b_seqChart7 X; } \
        }while ( 0 )
#   define A2B_SEQ_CHART8(X) \
        do { \
            if ( a2b_seqChartIsEnabled X ) \
            {a2b_seqChart8 X; } \
        }while ( 0 )
#else
#ifdef _TESSY_NO_DOWHILE_MACROS_
#   define A2B_SEQ_CHART0(X)
#   define A2B_SEQ_CHART1(X)
#   define A2B_SEQ_CHART2(X)
#   define A2B_SEQ_CHART3(X)
#   define A2B_SEQ_CHART4(X)
#   define A2B_SEQ_CHART5(X)
#   define A2B_SEQ_CHART6(X)
#   define A2B_SEQ_CHART7(X)
#   define A2B_SEQ_CHART8(X)
#else /* _TESSY_NO_DOWHILE_MACROS_ */
#   define A2B_SEQ_CHART0(X) do { } while ( 0 )
#   define A2B_SEQ_CHART1(X) do { } while ( 0 )
#   define A2B_SEQ_CHART2(X) do { } while ( 0 )
#   define A2B_SEQ_CHART3(X) do { } while ( 0 )
#   define A2B_SEQ_CHART4(X) do { } while ( 0 )
#   define A2B_SEQ_CHART5(X) do { } while ( 0 )
#   define A2B_SEQ_CHART6(X) do { } while ( 0 )
#   define A2B_SEQ_CHART7(X) do { } while ( 0 )
#   define A2B_SEQ_CHART8(X) do { } while ( 0 )
#endif /* _TESSY_NO_DOWHILE_MACROS_ */
#endif

/*----------------------------------------------------------------------------*/
/** 
 * \name    Raw Text 
 *  
 * C89 compatible sequence chart routines for injecting raw text. 
 *  
 * NOTE: A2B_SEQ_RAW# is limited to #A2B_MAX_SEQ_STR characters.
 */
/*----------------------------------------------------------------------------*/
#ifdef A2B_FEATURE_SEQ_CHART

#define A2B_MAX_SEQ_STR      128

#   define A2B_SEQ_RAW0(ctx, lvl, text) \
        do { \
          (void)a2b_seqChartInject(ctx, (a2b_UInt32)lvl, (const a2b_Char*)text); } while ( 0 )

#   define A2B_SEQ_RAW1(ctx, lvl, text, var0) \
        do { void* args[1]; a2b_Char seqBuf[A2B_MAX_SEQ_STR]; \
        args[0] = (void*)var0; \
        (void)a2b_vsnprintf(seqBuf, A2B_MAX_SEQ_STR, (const a2b_Char*)text, args, 1); \
        (void)a2b_seqChartInject( ctx, (a2b_UInt32)lvl, (const a2b_Char*)seqBuf ); \
        } while ( 0 )

#   define A2B_SEQ_RAW2(ctx, lvl, text, var0, var1) \
        do { void* args[2]; a2b_Char seqBuf[A2B_MAX_SEQ_STR]; \
        args[0] = (void*)var0; args[1] = (void*)var1; \
        (void)a2b_vsnprintf(seqBuf, A2B_MAX_SEQ_STR, (const a2b_Char*)text, args, 2); \
        (void)a2b_seqChartInject( ctx, (a2b_UInt32)lvl, (const a2b_Char*)&seqBuf[0] ); \
        } while ( 0 )

#   define A2B_SEQ_RAW3(ctx, lvl, text, var0, var1, var2) \
        do { void* args[3]; a2b_Char seqBuf[A2B_MAX_SEQ_STR]; \
        args[0] = (void*)var0; args[1] = (void*)var1; \
        args[2] = (void*)var2; \
        (void)a2b_vsnprintf(seqBuf, A2B_MAX_SEQ_STR, (const a2b_Char*)text, args, 3); \
        (void)a2b_seqChartInject( ctx, (a2b_UInt32)lvl, (const a2b_Char*)seqBuf ); \
        } while ( 0 )

#   define A2B_SEQ_RAW4(ctx, lvl, text, var0, var1, var2, var3) \
        do { void* args[4]; a2b_Char seqBuf[A2B_MAX_SEQ_STR]; \
        args[0] = (void*)var0; args[1] = (void*)var1; \
        args[2] = (void*)var2; args[3] = (void*)var3; \
        (void)a2b_vsnprintf(seqBuf, A2B_MAX_SEQ_STR, (const a2b_Char*)text, args, 4); \
        (void)a2b_seqChartInject( ctx, (a2b_UInt32)lvl, (const a2b_Char*)seqBuf ); \
        } while ( 0 )

#   define A2B_SEQ_RAW5(ctx, lvl, text, var0, var1, var2, var3, var4) \
        do { void* args[5]; a2b_Char seqBuf[A2B_MAX_SEQ_STR]; \
        args[0] = (void*)var0; args[1] = (void*)var1; \
        args[2] = (void*)var2; args[3] = (void*)var3; \
        args[4] = (void*)var4; \
        (void)a2b_vsnprintf(seqBuf, A2B_MAX_SEQ_STR, (const a2b_Char*)text, args, 5); \
        (void)a2b_seqChartInject( ctx, (a2b_UInt32)lvl, (const a2b_Char*)seqBuf ); \
        } while ( 0 )

#else
#ifdef _TESSY_NO_DOWHILE_MACROS_
#   define A2B_SEQ_RAW0(ctx, lvl, text)
#   define A2B_SEQ_RAW1(ctx, lvl, text, var0)
#   define A2B_SEQ_RAW2(ctx, lvl, text, var0, var1)
#   define A2B_SEQ_RAW3(ctx, lvl, text, var0, var1, var2)
#   define A2B_SEQ_RAW4(ctx, lvl, text, var0, var1, var2, var3)
#   define A2B_SEQ_RAW5(ctx, lvl, text, var0, var1, var2, var3, var4)
#else	/* _TESSY_NO_DOWHILE_MACROS_ */
#   define A2B_SEQ_RAW0(ctx, lvl, text) do { } while ( 0 )
#   define A2B_SEQ_RAW1(ctx, lvl, text, var0) do { } while ( 0 )
#   define A2B_SEQ_RAW2(ctx, lvl, text, var0, var1) do { } while ( 0 )
#   define A2B_SEQ_RAW3(ctx, lvl, text, var0, var1, var2) do { } while ( 0 )
#   define A2B_SEQ_RAW4(ctx, lvl, text, var0, var1, var2, var3) do { } while ( 0 )
#   define A2B_SEQ_RAW5(ctx, lvl, text, var0, var1, var2, var3, var4) do { } while ( 0 )
#endif	/* _TESSY_NO_DOWHILE_MACROS_ */
#endif

/*----------------------------------------------------------------------------*/
/** 
 * \name    Generic Notes 
 *  
 * C89 compatible sequence chart routines for injecting generic notes 
 * into the sequence for different. All notes are created to the right 
 * of the "Stack". 
 *  
 * \code 
 * A2B_SEQ_GENNOTE#  - Creates a yellow colored note 
 * A2B_SEQ_GENERROR# - Creates a salmon colored note 
 * \endcode 
 *  
 * \note     A2B_SEQ_GENNOTE# is limited to #A2B_MAX_SEQ_STR characters. 
 * \note     A2B_SEQ_GENERROR# is limited to #A2B_MAX_SEQ_STR characters. 
 */
/*----------------------------------------------------------------------------*/
#ifdef A2B_FEATURE_SEQ_CHART

#   define A2B_SEQ_GENNOTE0(ctx, lvl, text) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform"); \
        A2B_SEQ_RAW0(ctx, lvl, text); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENNOTE1(ctx, lvl, text, var0) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform"); \
        A2B_SEQ_RAW1(ctx, lvl, text, var0); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENNOTE2(ctx, lvl, text, var0, var1) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform"); \
        A2B_SEQ_RAW2(ctx, lvl, text, var0, var1); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENNOTE3(ctx, lvl, text, var0, var1, var2) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform"); \
        A2B_SEQ_RAW3(ctx, lvl, text, var0, var1, var2); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENNOTE4(ctx, lvl, text, var0, var1, var2, var3) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform"); \
        A2B_SEQ_RAW4(ctx, lvl, text, var0, var1, var2, var3); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENNOTE5(ctx, lvl, text, var0, var1, var2, var3, var4) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform"); \
        A2B_SEQ_RAW5(ctx, lvl, text, var0, var1, var2, var3, var4); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )


#   define A2B_SEQ_GENERROR0(ctx, lvl, text) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform #FFAAAA"); \
        A2B_SEQ_RAW0(ctx, lvl, text); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENERROR1(ctx, lvl, text, var0) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform #FFAAAA"); \
        A2B_SEQ_RAW1(ctx, lvl, text, var0); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENERROR2(ctx, lvl, text, var0, var1) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform #FFAAAA"); \
        A2B_SEQ_RAW2(ctx, lvl, text, var0, var1); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENERROR3(ctx, lvl, text, var0, var1, var2) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform #FFAAAA"); \
        A2B_SEQ_RAW3(ctx, lvl, text, var0, var1, var2); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENERROR4(ctx, lvl, text, var0, var1, var2, var3) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform #FFAAAA"); \
        A2B_SEQ_RAW4(ctx, lvl, text, var0, var1, var2, var3); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#   define A2B_SEQ_GENERROR5(ctx, lvl, text, var0, var1, var2, var3, var4) \
        do {         \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx, lvl, "note right of Platform #FFAAAA"); \
        A2B_SEQ_RAW5(ctx, lvl, text, var0, var1, var2, var3, var4); \
        A2B_SEQ_RAW0(ctx, lvl, "end note"); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#else

#ifdef _TESSY_NO_DOWHILE_MACROS_
#   define A2B_SEQ_GENNOTE0(ctx, lvl, text)
#   define A2B_SEQ_GENNOTE1(ctx, lvl, text, var0)
#   define A2B_SEQ_GENNOTE2(ctx, lvl, text, var0, var1)
#   define A2B_SEQ_GENNOTE3(ctx, lvl, text, var0, var1, var2)
#   define A2B_SEQ_GENNOTE4(ctx, lvl, text, var0, var1, var2, var3)
#   define A2B_SEQ_GENNOTE5(ctx, lvl, text, var0, var1, var2, var3, var4)

#   define A2B_SEQ_GENERROR0(ctx, lvl, text)
#   define A2B_SEQ_GENERROR1(ctx, lvl, text, var0)
#   define A2B_SEQ_GENERROR2(ctx, lvl, text, var0, var1)
#   define A2B_SEQ_GENERROR3(ctx, lvl, text, var0, var1, var2)
#   define A2B_SEQ_GENERROR4(ctx, lvl, text, var0, var1, var2, var3)
#   define A2B_SEQ_GENERROR5(ctx, lvl, text, var0, var1, var2, var3, var4)
#else	/* _TESSY_NO_DOWHILE_MACROS_ */
#   define A2B_SEQ_GENNOTE0(ctx, lvl, text) do { } while ( 0 )
#   define A2B_SEQ_GENNOTE1(ctx, lvl, text, var0) do { } while ( 0 )
#   define A2B_SEQ_GENNOTE2(ctx, lvl, text, var0, var1) do { } while ( 0 )
#   define A2B_SEQ_GENNOTE3(ctx, lvl, text, var0, var1, var2) do { } while ( 0 )
#   define A2B_SEQ_GENNOTE4(ctx, lvl, text, var0, var1, var2, var3) do { } while ( 0 )
#   define A2B_SEQ_GENNOTE5(ctx, lvl, text, var0, var1, var2, var3, var4) do { } while ( 0 )

#   define A2B_SEQ_GENERROR0(ctx, lvl, text) do { } while ( 0 )
#   define A2B_SEQ_GENERROR1(ctx, lvl, text, var0) do { } while ( 0 )
#   define A2B_SEQ_GENERROR2(ctx, lvl, text, var0, var1) do { } while ( 0 )
#   define A2B_SEQ_GENERROR3(ctx, lvl, text, var0, var1, var2) do { } while ( 0 )
#   define A2B_SEQ_GENERROR4(ctx, lvl, text, var0, var1, var2, var3) do { } while ( 0 )
#   define A2B_SEQ_GENERROR5(ctx, lvl, text, var0, var1, var2, var3, var4) do { } while ( 0 )
#endif	/* _TESSY_NO_DOWHILE_MACROS_ */

#endif

/*----------------------------------------------------------------------------*/
/** 
 * \name    Grouping 
 *  
 * C89 compatible sequence chart routines for injecting a group and 
 * ending a group.  These macros ensure NO time stamp is given 
 * to these logged messages. 
 *  
 * Usage: 
 *  
 * \code 
 * A2B_SEQ_GROUP0(ctx, A2B_SEQ_CHART_LEVEL_3, "Some new group"); // start the group 
 * ... 
 * A2B_SEQ_END(ctx, A2B_SEQ_CHART_LEVEL_3); // end the group 
 * \endcode 
 *  
 * \note    All text sent to A2B_SEQ_GROUP# MUST be a constant. 
 */
/*----------------------------------------------------------------------------*/
#ifdef A2B_FEATURE_SEQ_CHART

#    define A2B_SEQ_END( ctx, lvl ) \
        do { \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx,  lvl, "end" ); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#    define A2B_SEQ_GROUP0( ctx, lvl, text ) \
        do { \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW0(ctx,  lvl, "group "text ); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#    define A2B_SEQ_GROUP1( ctx, lvl, text, var0 ) \
        do { \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW1(ctx,  lvl, "group "text, var0 ); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#    define A2B_SEQ_GROUP2( ctx, lvl, text, var0, var1 ) \
        do { \
        a2b_UInt32 options = a2b_seqChartGetOptions(ctx); \
        a2b_seqChartSetOptions(ctx, (a2b_UInt32)(options & (a2b_UInt32)(~A2B_SEQ_CHART_OPT_TIMESTAMP))); \
        A2B_SEQ_RAW2(ctx,  lvl, "group "text, var0, var1 ); \
        a2b_seqChartSetOptions(ctx, options); \
        } while ( 0 )

#else

#ifdef _TESSY_NO_DOWHILE_MACROS_
#    define A2B_SEQ_END( ctx )
#    define A2B_SEQ_GROUP0( ctx )
#    define A2B_SEQ_GROUP1( ctx )
#    define A2B_SEQ_GROUP2( ctx )
#else	/* #ifdef _TESSY_NO_DOWHILE_MACROS_ */
#    define A2B_SEQ_END( ctx )        do { } while ( 0 )
#    define A2B_SEQ_GROUP0( ctx )     do { } while ( 0 )
#    define A2B_SEQ_GROUP1( ctx )     do { } while ( 0 )
#    define A2B_SEQ_GROUP2( ctx )     do { } while ( 0 )
#endif	/* #ifdef _TESSY_NO_DOWHILE_MACROS_ */

#endif
/** \} -- needed last for name */

/** \} -- a2bstack_seqchart_macros */


/*======================= D A T A T Y P E S =======================*/

/* Forward declarations */
struct a2b_StackContext;

/**
 * The definition of a sequence chart trace channel.
 */
typedef struct a2b_SeqChartChannel
{
    a2b_Bool            inUse;
    a2b_Handle          hnd;
    a2b_UInt32          levelMask;
    a2b_UInt32          options;
    a2b_Char            buf[A2B_CONF_TRACE_BUF_SIZE];
    a2b_StringBuffer    strBuf;
    struct a2b_StackContext*   ctx;
} a2b_SeqChartChannel;


/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_BEGIN_DECLS

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_seqchart_funct    Functions
 *  
 * These are the main sequence chart functions.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

#ifdef A2B_FEATURE_SEQ_CHART

A2B_DSO_PUBLIC a2b_Bool A2B_CALL a2b_seqChartIsEnabled(
                                struct a2b_StackContext* ctx,
                                a2b_SeqChartEntity src,
                                a2b_SeqChartEntity dest,
                                a2b_SeqChartCommType commType,
                                a2b_UInt32 level, ...);
A2B_DSO_PUBLIC void A2B_CALL a2b_seqChart0(
                            struct a2b_StackContext* ctx,
                            a2b_SeqChartEntity src,
                            a2b_SeqChartEntity dest,
                            a2b_SeqChartCommType commType,
                            a2b_UInt32 level,
                            const a2b_Char* fmt);
A2B_DSO_PUBLIC void A2B_CALL a2b_seqChart1(
                            struct a2b_StackContext* ctx,
                            a2b_SeqChartEntity src,
                            a2b_SeqChartEntity dest,
                            a2b_SeqChartCommType commType,
                            a2b_UInt32 level,
                            const a2b_Char* fmt,
                            void* a1);
A2B_DSO_PUBLIC void A2B_CALL a2b_seqChart2(
                            struct a2b_StackContext* ctx,
                            a2b_SeqChartEntity src,
                            a2b_SeqChartEntity dest,
                            a2b_SeqChartCommType commType,
                            a2b_UInt32 level,
                            const a2b_Char* fmt,
                            void* a1, void* a2);
A2B_DSO_PUBLIC void A2B_CALL a2b_seqChart3(
                            struct a2b_StackContext* ctx,
                            a2b_SeqChartEntity src,
                            a2b_SeqChartEntity dest,
                            a2b_SeqChartCommType commType,
                            a2b_UInt32 level,
                            const a2b_Char* fmt,
                            void* a1, void* a2, void* a3);
A2B_DSO_PUBLIC void A2B_CALL a2b_seqChart4(
                            struct a2b_StackContext* ctx,
                            a2b_SeqChartEntity src,
                            a2b_SeqChartEntity dest,
                            a2b_SeqChartCommType commType,
                            a2b_UInt32 level,
                            const a2b_Char* fmt,
                            void* a1, void* a2, void* a3, void* a4);
A2B_DSO_PUBLIC void A2B_CALL a2b_seqChart5(
                            struct a2b_StackContext* ctx,
                            a2b_SeqChartEntity src,
                            a2b_SeqChartEntity dest,
                            a2b_SeqChartCommType commType,
                            a2b_UInt32 level,
                            const a2b_Char* fmt,
                            void* a1, void* a2, void* a3, void* a4, void* a5);
A2B_DSO_PUBLIC void A2B_CALL a2b_seqChart6(
                            struct a2b_StackContext* ctx,
                            a2b_SeqChartEntity src,
                            a2b_SeqChartEntity dest,
                            a2b_SeqChartCommType commType,
                            a2b_UInt32 level,
                            const a2b_Char* fmt,
                            void* a1, void* a2, void* a3, void* a4, void* a5,
                            void* a6);
A2B_DSO_PUBLIC void A2B_CALL a2b_seqChart7(
                            struct a2b_StackContext* ctx,
                            a2b_SeqChartEntity src,
                            a2b_SeqChartEntity dest,
                            a2b_SeqChartCommType commType,
                            a2b_UInt32 level,
                            const a2b_Char* fmt,
                            void* a1, void* a2, void* a3, void* a4, void* a5,
                            void* a6, void* a7);
A2B_DSO_PUBLIC void A2B_CALL a2b_seqChart8(
                            struct a2b_StackContext* ctx,
                            a2b_SeqChartEntity src,
                            a2b_SeqChartEntity dest,
                            a2b_SeqChartCommType commType,
                            a2b_UInt32 level,
                            const a2b_Char* fmt,
                            void* a1, void* a2, void* a3, void* a4, void* a5,
                            void* a6, void* a7, void* a8);

A2B_DSO_LOCAL a2b_SeqChartChannel* a2b_seqChartAlloc
    (
    struct a2b_StackContext*   ctx,
    const a2b_Char*     name,
    a2b_UInt32          level
    );
A2B_DSO_LOCAL void a2b_seqChartFree(a2b_SeqChartChannel*  chan);

#endif /* A2B_FEATURE_SEQ_CHART */

/** \} -- a2bstack_seqchart_funct */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_seqchart */

#endif /* Guard for A2B_SEQ_CHART_H_ */
