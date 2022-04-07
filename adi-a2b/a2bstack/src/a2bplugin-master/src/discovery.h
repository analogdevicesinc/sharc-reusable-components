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
 * \file:   discovery.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This is the definition of a A2B master node discovery processing.
 *
 *=============================================================================
 */

/*! \addtogroup Network_Configuration
* @{
*/

/*! \addtogroup Discovery_and_Node_Configuration
 *  @{
 */

#ifndef A2B_DISCOVERY_H_
#define A2B_DISCOVERY_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "plugin_priv.h"
#ifdef A2B_FEATURE_COMM_CH
#include "adi_a2b_commch_interface.h"
#endif	/* A2B_FEATURE_COMM_CH */

/*======================= D E F I N E S ===========================*/

/** This is the mailbox index used to process peripheral 
  * configuration from the EEPROM during discovery in 
  * parallel to other node discovery. 
  */
#define A2B_PERIPH_MAILBOX      (0)

/** Internally used to indicate a completed execution but with failure */
#define A2B_EXEC_COMPLETE_FAIL  (0xFF)


/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT a2b_Int32 a2b_dscvryStart(a2b_Plugin* plugin,
                                     a2b_Bool    deinitFirst);

A2B_EXPORT a2b_Bool a2b_dscvryNodeDiscovered(a2b_Plugin* plugin);

A2B_EXPORT void a2b_dscvryEnd(a2b_Plugin* plugin, a2b_UInt32 errCode);

#ifdef A2B_FEATURE_EEPROM_PROCESSING
A2B_EXPORT void a2b_dscvryPeripheralProcessingComplete( a2b_Plugin* plugin, a2b_Int16   nodeAddr );
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

#ifdef A2B_FEATURE_COMM_CH
a2b_Bool adi_a2b_MstrPluginCommChStatusCb(void* pPlugin, a2b_CommChMsg *pMsg, A2B_COMMCH_EVENT eEventType, a2b_Int8 nNodeAddr);
a2b_Bool a2b_GetCommChInstIdForSlvNode(a2b_Plugin*  plugin, a2b_Int16 nSlvNodeAddr, a2b_UInt16* nIndexOut);
#endif	/* A2B_FEATURE_COMM_CH */

A2B_END_DECLS

/*======================= M A C R O S =============================*/

/* 
 * Set of convenience macros to make tracing and sequence chart
 * note generation easier and more compact.  
 *
 * NOTE:  The text must be a constant, not a variable.
 */
#ifdef A2B_FEATURE_SEQ_CHART

/*--------------*/
/* ERROR Macros */
/*--------------*/
#define A2B_DSCVRY_ERROR0( ctx, funct, text ) \
	do { \
    A2B_TRACE2 ((ctx, (a2b_UInt32)((a2b_UInt32)A2B_TRC_DOM_PLUGIN | (a2b_UInt32)A2B_TRC_LVL_ERROR), \
                "%s %s(): "text, A2B_MPLUGIN_PLUGIN_NAME, funct )); \
    A2B_SEQ_GENERROR0(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, text ); \
    }while(0)

#define A2B_DSCVRY_ERROR1( ctx, funct, text, var0 ) \
    do { \
    A2B_TRACE3 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR), \
                "%s %s(): "text, \
                A2B_MPLUGIN_PLUGIN_NAME, funct, var0 )); \
    A2B_SEQ_GENERROR1(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, \
                      text, var0 ); \
    }while(0)

#define A2B_DSCVRY_ERROR2( ctx, funct, text, var0, var1 ) \
    do{ \
    A2B_TRACE4 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR), \
                "%s %s(): "text, \
                A2B_MPLUGIN_PLUGIN_NAME, funct, var0, var1 )); \
    A2B_SEQ_GENERROR2(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, \
                      text, var0, var1 ); \
    }while(0)

#define A2B_DSCVRY_ERROR5( ctx, funct, text, var0, var1, var2, var3, var4 ) \
    do{ \
    A2B_TRACE7 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR), \
                "%s %s(): "text, \
                A2B_MPLUGIN_PLUGIN_NAME, funct, var0, var1, var2, \
                var3, var4 )); \
    A2B_SEQ_GENERROR5(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, \
                      text, var0, var1, var2, var3, var4 ); \
    }while(0)

/*-------------*/
/* WARN Macros */
/*-------------*/
#define A2B_DSCVRY_WARN0( ctx, funct, text ) \
    do{ \
    A2B_TRACE2 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_WARN), \
                "%s %s(): "text, A2B_MPLUGIN_PLUGIN_NAME, funct )); \
    A2B_SEQ_GENNOTE0(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, text ); \
    }while(0)

/*--------------*/
/* DEBUG Macros */
/*--------------*/
#define A2B_DSCVRYNOTE_DEBUG1( ctx, funct, text, var0 ) \
    do{ \
    A2B_TRACE3 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_DEBUG), \
                "%s %s(): "text, \
                A2B_MPLUGIN_PLUGIN_NAME, funct, var0 )); \
    A2B_SEQ_GENNOTE1(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, \
                     text, var0 ); \
    }while(0)

#define A2B_DSCVRYNOTE_DEBUG2( ctx, funct, text, var0, var1 ) \
    do{ \
    A2B_TRACE4 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_DEBUG), \
                "%s %s(): "text, \
                A2B_MPLUGIN_PLUGIN_NAME, funct, var0, var1 )); \
    A2B_SEQ_GENNOTE2(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, \
                     text, var0, var1 ); \
    }while(0)

#define A2B_DSCVRYNOTE_DEBUG3( ctx, funct, text, var0, var1, var2 ) \
    do{ \
    A2B_TRACE5 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_DEBUG), \
                "%s %s(): "text, \
                A2B_MPLUGIN_PLUGIN_NAME, funct, var0, var1, var2 )); \
    A2B_SEQ_GENNOTE3(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, \
                     text, var0, var1, var2 ); \
    }while(0)

#define A2B_DSCVRY_RAWDEBUG0( ctx, funct, text ) \
    do{ \
    A2B_TRACE2 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_DEBUG), \
                "%s %s(): "text, A2B_MPLUGIN_PLUGIN_NAME, funct )); \
    A2B_SEQ_RAW0(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, text ); \
    }while(0)

#define A2B_DSCVRY_RAWDEBUG1( ctx, funct, text, var0 ) \
   do{ \
    A2B_TRACE3 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_DEBUG), \
                "%s %s(): "text, A2B_MPLUGIN_PLUGIN_NAME, funct, var0 )); \
    A2B_SEQ_RAW1(ctx,  A2B_SEQ_CHART_LEVEL_DISCOVERY, text, var0 ); \
   }while(0)

/*-------------*/
/* MISC Macros */
/*-------------*/
#define A2B_DSCVRY_SEQEND( ctx ) \
    do{ \
    A2B_TRACE0 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1), \
                "<< end group" )); \
    A2B_SEQ_END( ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY); \
    }while(0)

#define A2B_DSCVRY_SEQGROUP0( ctx, text ) \
    do{ \
    A2B_TRACE0 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1), \
                ">> group "text )); \
    A2B_SEQ_GROUP0( ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY, text); \
    }while(0)

#define A2B_DSCVRY_SEQGROUP1( ctx, text, var0 ) \
    do{ \
    A2B_TRACE1 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1), \
                ">> group "text, var0 )); \
    A2B_SEQ_GROUP1( ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY, text, var0); \
    }while(0)

#define A2B_DSCVRY_SEQGROUP2( ctx, text, var0, var1 ) \
    do{ \
    A2B_TRACE2 ((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1), \
                ">> group "text, var0, var1 )); \
    A2B_SEQ_GROUP2( ctx, A2B_SEQ_CHART_LEVEL_DISCOVERY, text, var0, var1); \
    }while(0)

#define A2B_DSCVRY_SEQEND_COND( ctx, bCond ) \
    do{ \
    if ( bCond ) \
        { A2B_DSCVRY_SEQEND( ctx ); }\
    }while(0)

#define A2B_DSCVRY_SEQGROUP0_COND( ctx, bCond, text ) \
    do{ \
    if ( !(bCond) ) \
    { \
        (bCond) = A2B_TRUE; \
        A2B_DSCVRY_SEQGROUP0( ctx, text ); \
    } \
    }while(0)

#else /* !A2B_FEATURE_SEQ_CHART */
#ifdef _TESSY_NO_DOWHILE_MACROS_
/*--------------*/
/* ERROR Macros */
/*--------------*/
#define A2B_DSCVRY_ERROR0( ctx, funct, text )
#define A2B_DSCVRY_ERROR1( ctx, funct, text, var0 )
#define A2B_DSCVRY_ERROR2( ctx, funct, text, var0, var1 )
#define A2B_DSCVRY_ERROR5( ctx, funct, text, var0, var1, var2, var3, var4 )

/*-------------*/
/* WARN Macros */
/*-------------*/
#define A2B_DSCVRY_WARN0( ctx, funct, text )

/*--------------*/
/* DEBUG Macros */
/*--------------*/
#define A2B_DSCVRYNOTE_DEBUG1( ctx, funct, text, var0 )
#define A2B_DSCVRYNOTE_DEBUG2( ctx, funct, text, var0, var1 )
#define A2B_DSCVRYNOTE_DEBUG3( ctx, funct, text, var0, var1, var2 )
#define A2B_DSCVRY_RAWDEBUG0( ctx, funct, text )
#define A2B_DSCVRY_RAWDEBUG1( ctx, funct, text, var0 )

/*-------------*/
/* MISC Macros */
/*-------------*/
#define A2B_DSCVRY_SEQEND( ctx )
#define A2B_DSCVRY_SEQGROUP0( ctx, text )
#define A2B_DSCVRY_SEQGROUP1( ctx, text, var0 )
#define A2B_DSCVRY_SEQGROUP2( ctx, text, var0, var1 )

#define A2B_DSCVRY_SEQEND_COND( ctx, bCond )
#define A2B_DSCVRY_SEQGROUP0_COND( ctx, bCond, text )

#else /* _TESSY_NO_DOWHILE_MACROS_ */


/*--------------*/
/* ERROR Macros */
/*--------------*/
#define A2B_DSCVRY_ERROR0( ctx, funct, text )               do { } while ( 0 )
#define A2B_DSCVRY_ERROR1( ctx, funct, text, var0 )         do { } while ( 0 )
#define A2B_DSCVRY_ERROR2( ctx, funct, text, var0, var1 )   do { } while ( 0 )
#define A2B_DSCVRY_ERROR5( ctx, funct, text, var0, var1, var2, var3, var4 ) do { } while ( 0 )

/*-------------*/
/* WARN Macros */
/*-------------*/
#define A2B_DSCVRY_WARN0( ctx, funct, text ) do { } while ( 0 )

/*--------------*/
/* DEBUG Macros */
/*--------------*/
#define A2B_DSCVRYNOTE_DEBUG1( ctx, funct, text, var0 )       do { } while ( 0 )
#define A2B_DSCVRYNOTE_DEBUG2( ctx, funct, text, var0, var1 ) do { } while ( 0 )
#define A2B_DSCVRYNOTE_DEBUG3( ctx, funct, text, var0, var1, var2 ) do { } while ( 0 )
#define A2B_DSCVRY_RAWDEBUG0( ctx, funct, text )              do { } while ( 0 )
#define A2B_DSCVRY_RAWDEBUG1( ctx, funct, text, var0 )        do { } while ( 0 )

/*-------------*/
/* MISC Macros */
/*-------------*/
#define A2B_DSCVRY_SEQEND( ctx )                        do { } while ( 0 )
#define A2B_DSCVRY_SEQGROUP0( ctx, text )               do { } while ( 0 )
#define A2B_DSCVRY_SEQGROUP1( ctx, text, var0 )         do { } while ( 0 )
#define A2B_DSCVRY_SEQGROUP2( ctx, text, var0, var1 )   do { } while ( 0 )

#define A2B_DSCVRY_SEQEND_COND( ctx, bCond )            do { } while ( 0 )
#define A2B_DSCVRY_SEQGROUP0_COND( ctx, bCond, text )   do { } while ( 0 )

#endif /* _TESSY_NO_DOWHILE_MACROS_ */
#endif

/*======================= D A T A =================================*/


#endif /* A2B_DISCOVERY_H_ */

/**
 @}
*/

/**
 @}
*/
