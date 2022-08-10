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
 * \file:   util.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The definition of a collection of common system utility functions.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_util          Utilities Module
 *  
 * The definition of a collection of common system utility functions.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_UTIL_H_
#define A2B_UTIL_H_

/*======================= I N C L U D E S =========================*/
#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward Declarations */
struct a2b_StringBuffer;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_util_crc          CRC Functions
 *  
 * These functions are utility functions for CRC verification.
 *
 * \{ */
/*----------------------------------------------------------------------------*/
A2B_DSO_PUBLIC a2b_UInt8 A2B_CALL a2b_crc8(const a2b_UInt8* dataArr,
                                             a2b_UInt32     startIdx,
                                             a2b_UInt32     crcLen);
A2B_DSO_PUBLIC a2b_UInt8 A2B_CALL a2b_crc8Cont(const a2b_UInt8* dataArr,
                                                 a2b_UInt8      initCrc8,
                                                 a2b_UInt32     startIdx,
                                                 a2b_UInt32     crcLen);
/** \} -- a2bstack_util_crc */

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_util_mem          Memory Functions
 *  
 * These functions are utility functions for memory handling.
 *
 * \{ */
/*----------------------------------------------------------------------------*/
A2B_DSO_PUBLIC void* A2B_CALL a2b_memset(void*      s,
                                       a2b_Int      c,
                                       a2b_Size     n);
A2B_DSO_PUBLIC void* A2B_CALL a2b_memcpy(void*      dest,
                                       const void*  src,
                                       a2b_Size     n);
/** \} -- a2bstack_util_mem */

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_util_str          String Functions
 *  
 * These functions are utility functions for string handling.
 *
 * \{ */
/*----------------------------------------------------------------------------*/
A2B_DSO_PUBLIC a2b_Size A2B_CALL a2b_strlen(const a2b_Char* s);
A2B_DSO_PUBLIC a2b_Char* A2B_CALL a2b_strcpy(a2b_Char*  dest,
                                       const a2b_Char*  src);
A2B_DSO_PUBLIC a2b_Char* A2B_CALL a2b_strncpy(a2b_Char* dest,
                                        const a2b_Char* src,
                                        a2b_Size        n);
A2B_DSO_PUBLIC a2b_Char* A2B_CALL a2b_strrchr(const a2b_Char*   s,
                                                a2b_Char        c);
A2B_DSO_PUBLIC a2b_Int A2B_CALL a2b_vsnprintf(a2b_Char*         buf,
                                              a2b_Size          bufSize,
                                              const a2b_Char*   fmt,
                                              void**            args,
                                              a2b_UInt16        numArgs);
A2B_DSO_PUBLIC a2b_Int A2B_CALL a2b_vsnprintfStringBuffer(
                                        struct a2b_StringBuffer*    strBuf,
                                        const a2b_Char*             fmt,
                                        void**                      args,
                                        a2b_UInt16                  numArgs);
/** \} -- a2bstack_util_str */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_util */

#endif /* A2B_UTIL_H_ */
