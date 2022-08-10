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
 * \file:   daig.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The definitions and interfaces supporting A2B stack diagnostics.
 *
 *=============================================================================
 */

/*============================================================================*/
/**
 * \defgroup a2bstack_diag          Diagnostics Module
 *  
 * The types and APIs providing raw access to diagnostic registers
 * of the A2B stack.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_DIAG_H_
#define A2B_DIAG_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/


/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward Declarations */
struct a2b_StackContext;


/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_diagWriteReg(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_UInt8 reg,
                                                a2b_UInt8 value);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_diagReadReg(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_UInt8 reg,
                                                a2b_UInt8* value);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_diagReadPrbsErrCnt(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_UInt32* count);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_diagGetRegDump(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_UInt8 regOffset,
                                                a2b_UInt8* regs,
                                                a2b_UInt16* numRegs);

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_diag */

#endif /* A2B_DIAG_H_ */
