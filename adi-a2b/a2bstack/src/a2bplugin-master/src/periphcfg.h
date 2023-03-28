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
 * \file:   periphcfg.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of EEPROM peripheral config processing.
 *
 *=============================================================================
 */

#ifndef PERIPHCFG_H_
#define PERIPHCFG_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "plugin_priv.h"


/*======================= D E F I N E S ===========================*/


/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/*======================= P U B L I C  P R O T O T Y P E S ========*/

A2B_EXPORT a2b_HResult a2b_periphCfgPreparse( a2b_Plugin*     plugin );

A2B_EXPORT a2b_Bool a2b_periphCfgCompleted( a2b_Plugin* plugin );

A2B_EXPORT a2b_HResult a2b_periphCfgWriteRead( a2b_Plugin*     plugin,
                                               a2b_Int16       nodeAddr,
                                               a2b_UInt16      nWrite,
                                               void*           wBuf,
                                               a2b_UInt16      nRead,
                                               void*           rBuf );

A2B_EXPORT a2b_Bool a2b_periphCfgUsingSync(void);

A2B_EXPORT a2b_Int32 a2b_periphCfgInitProcessing( a2b_Plugin* plugin,
                                                  a2b_Int16   nodeAddr );

A2B_EXPORT a2b_Int32 a2b_periphCfgStartProcessing( struct a2b_Msg* msg );

A2B_EXPORT a2b_Int32 a2b_periphCfgProcessing( a2b_Plugin* plugin,
                                              a2b_Int16 nodeAddr );
void a2b_onPeripheralDelayTimeout(struct a2b_Timer *timer,
										a2b_Handle userData);

A2B_END_DECLS

/*======================= M A C R O S =============================*/


/*======================= D A T A =================================*/


#endif /* end of PERIPHCFG_H_ */
