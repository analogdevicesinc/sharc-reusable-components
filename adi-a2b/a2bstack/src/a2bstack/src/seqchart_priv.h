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
 * \file:   seqchart_priv.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Definition of private sequence chart macros.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \ingroup  a2bstack_seqchart 
 * \defgroup a2bstack_seqchart_priv         \<Private\> 
 * \private 
 *  
 * This defines the sequence chart API's that are private to the stack. 
 *  
 * \{ */
/*============================================================================*/


#ifndef A2B_SEQ_CHART_PRIV_H_
#define A2B_SEQ_CHART_PRIV_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/


/*======================= D A T A T Y P E S =======================*/

/* Forward declarations */
struct a2b_StackContext;
struct a2b_SeqChartChannel;


/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_BEGIN_DECLS

#ifdef A2B_FEATURE_SEQ_CHART

A2B_EXPORT A2B_DSO_LOCAL struct a2b_SeqChartChannel* a2b_seqChartAlloc(
                                            struct a2b_StackContext*    ctx,
                                            const a2b_Char*             name,
                                            a2b_UInt32                  level);

A2B_EXPORT A2B_DSO_LOCAL void a2b_seqChartFree(
                                            struct a2b_SeqChartChannel* chan);

#endif /* A2B_FEATURE_SEQ_CHART */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_seqchart_priv */

#endif /* Guard for A2B_SEQ_CHART_PRIV_H_ */
