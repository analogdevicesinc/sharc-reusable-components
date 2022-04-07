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
 * \file:   pal.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The definitions/prototypes for the Platform Abstraction Layer
 *          (PAL) that provides a platform-specific implementation.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_pal               PAL Module
 *  
 * The definitions/prototypes for the Platform Abstraction Layer
 * (PAL) that provides a platform-specific implementation.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_PAL_H_
#define A2B_PAL_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/ecb.h"
#include "a2b/i2c.h"
#include "a2b/msgtypes.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_PluginApi;

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_memmgr    Memory Mgmt Types/Defs
 * \ingroup  a2bstack_pal_def
 *  
 * PAL memory manager data types and definitions
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef a2b_HResult (A2B_CALL * a2b_MemMgrInitFunc)(A2B_ECB* ecb);
typedef a2b_Handle (A2B_CALL * a2b_MemMgrOpenFunc)(a2b_Byte* heap, a2b_UInt32 heapSize);
typedef void* (A2B_CALL * a2b_MemMgrMallocFunc)(a2b_Handle hnd, a2b_UInt32 size);
typedef void (A2B_CALL * a2b_MemMgrFreeFunc)(a2b_Handle hnd, void* p);
typedef a2b_HResult (A2B_CALL * a2b_MemMgrCloseFunc)(a2b_Handle hnd);
typedef a2b_HResult (A2B_CALL * a2b_MemMgrShutdown)(A2B_ECB* ecb);

/** \} -- a2bstack_pal_memmgr */


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_i2c           I2C Types/Defs 
 * \ingroup  a2bstack_pal_def
 *  
 * PAL I2C data types and definitions
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef a2b_HResult (A2B_CALL * a2b_I2cInitFunc)(A2B_ECB* ecb);
typedef a2b_Handle (A2B_CALL * a2b_I2cOpenFunc)(a2b_I2cAddrFmt fmt,
                               a2b_I2cBusSpeed speed, A2B_ECB* ecb);
typedef a2b_HResult (A2B_CALL * a2b_I2cCloseFunc)(a2b_Handle hnd);
typedef a2b_HResult (A2B_CALL * a2b_I2cReadFunc)(a2b_Handle hnd,
                            a2b_UInt16 addr, a2b_UInt16 nRead, a2b_Byte* rBuf);
typedef a2b_HResult (A2B_CALL * a2b_I2cWriteFunc)(a2b_Handle hnd,
                            a2b_UInt16 addr, a2b_UInt16 nWrite,
                            const a2b_Byte* wBuf);
typedef a2b_HResult (A2B_CALL * a2b_I2cWriteReadFunc)(a2b_Handle hnd,
                                    a2b_UInt16 addr,
                                    a2b_UInt16 nWrite,
                                    const a2b_Byte* wBuf,
                                    a2b_UInt16 nRead,
                                    a2b_Byte* rBuf);
typedef a2b_HResult (A2B_CALL * a2b_I2cShutdownFunc)(A2B_ECB* ecb);

/** \} -- a2bstack_pal_i2c */


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_timer         Timer Types/Defs 
 * \ingroup  a2bstack_pal_def 
 *  
 * PAL timer data types and definitions
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef a2b_HResult (A2B_CALL * a2b_TimerInitFunc)(A2B_ECB* ecb);
/* Must return time in milliseconds */
typedef a2b_UInt32 (A2B_CALL * a2b_TimerGetSysTimeFunc)(void);
typedef a2b_HResult (A2B_CALL * a2b_TimerShutdownFunc)(A2B_ECB* ecb);

/** \} -- a2bstack_pal_timer */


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_log           Log Output Types/Defs 
 * \ingroup  a2bstack_pal_def 
 *  
 * PAL Log output data types and definitions
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef a2b_HResult (A2B_CALL * a2b_LogInitFunc)(A2B_ECB* ecb);
typedef a2b_Handle (A2B_CALL * a2b_LogOpenFunc)(const a2b_Char* url);
typedef a2b_HResult (A2B_CALL * a2b_LogCloseFunc)(a2b_Handle hnd);
typedef a2b_HResult (A2B_CALL * a2b_LogWriteFunc)(a2b_Handle hnd,
                                                    const a2b_Char* msg);
typedef a2b_HResult (A2B_CALL * a2b_LogShutdownFunc)(A2B_ECB* ecb);

/** \} -- a2bstack_pal_log */


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_audio         Audio Configuration Types/Defs 
 * \ingroup  a2bstack_pal_def 
 *  
 * PAL audio configuration data types and definitions
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef a2b_HResult (A2B_CALL * a2b_AudioInitFunc)(A2B_ECB* ecb);
typedef a2b_Handle  (A2B_CALL * a2b_AudioOpenFunc)(void);
typedef a2b_HResult (A2B_CALL * a2b_AudioCloseFunc)(a2b_Handle hnd);
typedef a2b_HResult (A2B_CALL * a2b_AudioConfigFunc)(a2b_Handle hnd,
                                            a2b_TdmSettings* tdmSettings);
typedef a2b_HResult (A2B_CALL * a2b_AudioShutdownFunc)(A2B_ECB* ecb);

/** \} -- a2bstack_pal_audio */


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_plugin        Plugin Types/Defs 
 * \ingroup  a2bstack_pal_def 
 *  
 * PAL plugin data types and definitions. 
 *  
 * These functions are responsible for loading/unloading the slave/master
 * plugins that provide plugin specific behavior. 
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef a2b_HResult (A2B_CALL * a2b_PluginsLoadFunc)(
                                    struct a2b_PluginApi** plugins,
                                    a2b_UInt16* numPlugins, A2B_ECB* ecb);

typedef a2b_HResult (A2B_CALL * a2b_PluginsUnloadFunc)(
                                        struct a2b_PluginApi* plugins,
                                        a2b_UInt16 numPlugins, A2B_ECB* ecb);

/** \} -- a2bstack_pal_plugin */


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_version       Version Types/Defs 
 * \ingroup  a2bstack_pal_def 
 *  
 * PAL version/build data types and definitions. 
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef void (A2B_CALL * a2b_PalGetVersionFunc)(a2b_UInt32* major,
                                    a2b_UInt32* minor,
                                    a2b_UInt32* release);

typedef void (A2B_CALL * a2b_PalGetBuildFunc)(a2b_UInt32* buildNum,
                                    const a2b_Char** const buildDate,
                                    const a2b_Char** const buildOwner,
                                    const a2b_Char** const buildSrcRev,
                                    const a2b_Char** const buildHost);

/** \} -- a2bstack_pal_version */


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_def           PAL Main Defs
 *  
 * Prototypes for Platform Abstraction Layer (PAL).
 *
 * These functions must be implemented when porting the stack from
 * one platform to another. They implement the necessary platform
 * specific services that are abstracted away from the generic
 * A2B stack.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef struct a2b_StackPal
{
    /** \name Memory Manager Implementation
     *  Memory manager prototypes requiring platform implementation.
     *
     * \{ */
    a2b_MemMgrInitFunc      memMgrInit;
    a2b_MemMgrOpenFunc      memMgrOpen;
    a2b_MemMgrMallocFunc    memMgrMalloc;
    a2b_MemMgrFreeFunc      memMgrFree;
    a2b_MemMgrCloseFunc     memMgrClose;
    a2b_MemMgrShutdown      memMgrShutdown;
    /** \} */

    /** \name I2C Implementation
     *  I2C prototypes requiring platform implementation.
     *
     * \{ */
    a2b_I2cInitFunc         i2cInit;
    a2b_I2cOpenFunc         i2cOpen;
    a2b_I2cCloseFunc        i2cClose;
    a2b_I2cReadFunc         i2cRead;
    a2b_I2cWriteFunc        i2cWrite;
    a2b_I2cWriteReadFunc    i2cWriteRead;
    a2b_I2cShutdownFunc     i2cShutdown;
    /** \} */

    /** \name Timer Implementation
     *  Timer prototypes requiring platform implementation.
     *
     * \{ */
    a2b_TimerInitFunc       timerInit;
    a2b_TimerGetSysTimeFunc timerGetSysTime;
    a2b_TimerShutdownFunc   timerShutdown;
    /** \} */

    /** \name Logging Implementation
     *  Logging requiring platform implementation.
     *
     * \{ */
    a2b_LogInitFunc         logInit;
    a2b_LogOpenFunc         logOpen;
    a2b_LogCloseFunc        logClose;
    a2b_LogWriteFunc        logWrite;
    a2b_LogShutdownFunc     logShutdown;
    /** \} */

    /** \name Audio Implementation
     *  Audio settings requiring platform implementation.
     *
     * \{ */
    a2b_AudioInitFunc       audioInit;
    a2b_AudioOpenFunc       audioOpen;
    a2b_AudioCloseFunc      audioClose;
    a2b_AudioConfigFunc     audioConfig;
    a2b_AudioShutdownFunc   audioShutdown;
    /** \} */

    /** \name Platform Implementation
     *  Platform implementation for loading/unloading stack plugins.
     *
     * \{ */
    a2b_PluginsLoadFunc     pluginsLoad;
    a2b_PluginsUnloadFunc   pluginsUnload;
    /** \} */

    /** \name Version/Build Implementation
     *  Platform implementation for returning version and build information.
     *
     * \{ */
    a2b_PalGetVersionFunc   getVersion;
    a2b_PalGetBuildFunc     getBuild;
    /** \} */

} a2b_StackPal;

/** \} -- a2bstack_pal_def */


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_platform      Platform Types/Defs
 *  
 * PAL platform data types and definitions for init/shutdown. 
 *  
 * Platform specific functions used to initialize and shutdown
 * a given platform. These will be call *once* per platform if
 * provided to the #a2b_systemInitialize/Shutdown() functions
 * respectively.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef a2b_HResult (A2B_CALL * a2b_PlatformInitFunc)(A2B_ECB* ecb);
typedef a2b_HResult (A2B_CALL * a2b_PlatformShutdownFunc)(A2B_ECB* ecb);

/** \} -- a2bstack_pal_platform */


/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_pal_pub           Init Function
 *  
 * Function to initialize the platform stack functions to default
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC void A2B_CALL a2b_palInit(a2b_StackPal* pal, A2B_ECB* ecb);

/** \} -- a2bstack_pal_pub */


A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_pal */

#endif /* A2B_PAL_H_ */
