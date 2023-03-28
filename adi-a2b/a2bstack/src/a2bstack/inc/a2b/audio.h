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
 * \file:   audio.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The definitions and interfaces supporting A2B stack audio access.
 *
 *=============================================================================
 */

#ifndef A2B_AUDIO_H_
#define A2B_AUDIO_H_

/*============================================================================*/
/** 
 * \defgroup a2bstack_audio         Audio Module
 *  
 * The types and associated public APIs providing access to 
 * some A2B stack audio specific functionality.
 *
 * \{ */
/*============================================================================*/

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/msgtypes.h"

/*======================= D E F I N E S ===========================*/


/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_audioConfig(
                                    struct a2b_StackContext*  ctx,
                                    a2b_TdmSettings*          tdmSettings );

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_audio */

#endif /* end of A2B_AUDIO_H_ */

