/*
 * Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
 */

/*!    @file osal_common.h */

#ifndef __OSAL_COMMON_H__
#define __OSAL_COMMON_H__

/*=============  I N C L U D E S   =============*/
#include "adi_osal.h"
#include "osal_misra.h"

/*==============  D E F I N E S  ===============*/

#if defined(__STDC__)
/* C language specific macros and declarations*/

 /* True if running at ISR level. */
#define CALLED_FROM_AN_ISR              (_adi_osal_IsCurrentLevelISR())

/* number of microseconds per second */
#define     USEC_PER_SEC                    (1000000u)


/*=============  E X T E R N A L S  =============*/

/* Local global variables that are shared across files */

/* code */
extern  bool _adi_osal_AcquireGlobalLock( void );
extern  void _adi_osal_ReleaseGlobalLock( void );
extern ADI_OSAL_STATUS _adi_osal_InitInterrupts(void);

#else
/* assembly language specific macros and declarations*/


#endif  /* if !defined(__STDC__) */


#endif /*__OSAL_COMMON_H__ */

/*
**
** EOF:
**
*/
