/*****************************************************************************
    Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
*****************************************************************************/

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
