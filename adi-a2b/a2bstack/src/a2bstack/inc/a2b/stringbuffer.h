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
 * \file:   stringbuffer.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Defines the API for the string buffer object.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_stringbuffer          String Buffer Module
 *  
 * Defines the API for the string buffer object.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_STRINGBUFFER_H_
#define A2B_STRINGBUFFER_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/
A2B_BEGIN_DECLS

/** String buffer object */
typedef struct a2b_StringBuffer
{
    /** Raw buffer holding the data */
    a2b_Char*   buf;

    /** Capacity in *characters* rather than bytes */
    a2b_UInt32  capacity;

    /** Write position within the buffer */
    a2b_UInt32  pos;

} a2b_StringBuffer;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_DSO_PUBLIC void A2B_CALL a2b_stringBufferInit(a2b_StringBuffer* strBuf,
                                                    a2b_Char*       rawBuf,
                                                    a2b_UInt32      size);

A2B_DSO_PUBLIC a2b_Bool A2B_CALL a2b_stringBufferAppend(
                                    a2b_StringBuffer*   strBuf, 
                                    a2b_Char            ch);

A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_stringBufferLength(
                                    const a2b_StringBuffer* strBuf);

A2B_DSO_PUBLIC void A2B_CALL a2b_stringBufferClear(a2b_StringBuffer* strBuf);

A2B_DSO_PUBLIC a2b_UInt32 A2B_CALL a2b_stringBufferAppendWithFill(
                            a2b_UInt32          width,
                            a2b_Bool            zeroFill,
                            a2b_StringBuffer*   dest, 
                            const a2b_Char*     src);

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_stringbuffer */

#endif /* A2B_STRINGBUFFER_H_ */
