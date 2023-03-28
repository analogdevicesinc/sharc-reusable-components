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
 * \file:   memmgr.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Defines the API for the memory management service based on
 *          multiple memory pools. The implementation of the memory management
 *          service.  This memory manager is only available for the stack.
 *          The application and plugins do not have access to this 
 *          implementation.  If the application and/or plugin needs 
 *          its memory managment it must find its own implementation.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_memmgr            Memory Manager Module 
 *  
 * The implementation of the memory management service.
 * This memory manager is only available for the stack.  The 
 * application and plugins do not have access to this 
 * implementation.  If the application and/or plugin needs 
 * its memory managment it must find its own implementation.
 *  
 * \{ */
/** 
 * \defgroup a2bstack_memmgr_priv       \<Private\>
 * \private 
 *
 * This defines the memory manager API's that are private to the stack. 
 *  
 * \{ */
/*============================================================================*/

#ifndef A2B_MEMMGR_H_
#define A2B_MEMMGR_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/ecb.h"
#include "a2b/features.h"
#include "pool.h"

#ifdef A2B_FEATURE_MEMORY_MANAGER

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT A2B_DSO_LOCAL a2b_UInt32 a2b_memMgrGetMinHeapSize(
                                                a2b_Byte* heapStart);
A2B_EXPORT A2B_DSO_LOCAL a2b_HResult a2b_memMgrInit(A2B_ECB* ecb);
A2B_EXPORT A2B_DSO_LOCAL a2b_Handle a2b_memMgrOpen(a2b_Byte* heap,
                                                a2b_UInt32 heapSize);
A2B_EXPORT A2B_DSO_LOCAL void* a2b_memMgrMalloc(a2b_Handle hnd,
                                                a2b_UInt32 size);
A2B_EXPORT A2B_DSO_LOCAL void a2b_memMgrFree(a2b_Handle hnd,
                                            void* p);
A2B_EXPORT A2B_DSO_LOCAL a2b_HResult a2b_memMgrClose(a2b_Handle hnd);
A2B_EXPORT A2B_DSO_LOCAL a2b_HResult a2b_memMgrShutdown(A2B_ECB* ecb);

A2B_END_DECLS

/*======================= D A T A =================================*/

#endif  /* A2B_FEATURE_MEMORY_MANAGER */

/** \} -- a2bstack_memmgr_priv */

/** \} -- a2bstack_memmgr */

#endif /* A2B_MEMMGR_H_ */
