/**
 * Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Simple driver includes. */
#include "simple-drivers/twi_simple.h"

/* A2B Stack includes */
#include "a2b/a2b.h"

/* If compiled statically, include the A2B Master plugin for convenience */
#ifdef A2B_STATIC_PLUGIN
#include "a2bplugin-master/plugin.h"
#endif

/* A2B Application includes */
#include "a2bapp_helpers.h"
#include "a2bapp_pal.h"

/*=============================================================================
 *
 *  Timer PAL
 *
 *=============================================================================
 */
/*!****************************************************************************
*
*  pal_timerInit
*
*  This routine is called to do initialization the timer subsystem
*  during the stack allocation process.
*                                                                       <br><br>
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_timerInit(A2B_ECB * ecb)
{
    A2B_UNUSED(ecb);
    return A2B_RESULT_SUCCESS;
}


/*!****************************************************************************
*
*  pal_timerShutdown
*
*  This routine is called to shutdown the timer subsystem
*  during the stack destroy process.
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_timerShutdown(A2B_ECB *ecb)
{
   A2B_UNUSED(ecb);
   return A2B_RESULT_SUCCESS;
}

/* pal_timerGetSysTime() MUST BE IMPLEMENTED FOR A FUNCTIONAL STACK */
/*!****************************************************************************
*
*  pal_timerGetSysTime
*
*  This routine returns the current "system" time in milliseconds. The
*  underlying system time is platform specific.
*
*  return:         The current system time referenced from some epoch.  For
*                  Linux, time is referenced from the first time this function
*                  is called.  The units are milliseconds.
*
******************************************************************************/
a2b_UInt32 pal_timerGetSysTime(void)
{
    a2b_UInt32 sysTime;

    sysTime = ((a2b_UInt32)xTaskGetTickCount() * 1000u) / configTICK_RATE_HZ;

    return sysTime;
}

/*=============================================================================
 *
 *  Plugin PAL
 *
 *=============================================================================
 */
/* pal_pluginsLoad() MUST BE IMPLEMENTED FOR A FUNCTIONAL STACK */
/*!****************************************************************************
*
*  pal_pluginsLoad
*
*  This routine is called to search and load all available plugins.
*  The list of available plugins are returned and typically used to query
*  for plugins during discovery when nodes are found.
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_pluginsLoad(struct a2b_PluginApi **plugins, a2b_UInt16 *numPlugins, A2B_ECB *ecb )
{
    A2B_UNUSED(ecb);
#ifdef A2B_STATIC_PLUGIN
    static struct a2b_PluginApi palPlugins[1];
#endif

#ifdef A2B_STATIC_PLUGIN
    /* This loads a statically linked Master plugin */
    A2B_MASTER_PLUGIN_INIT(&palPlugins[0]);
    *plugins = palPlugins;
    *numPlugins = 1;
#else
    *plugins = A2B_NULL;
    *numPlugins = 0;
#endif

    return A2B_RESULT_SUCCESS;
}


/* pal_pluginsUnload() MUST BE IMPLEMENTED FOR A FUNCTIONAL STACK */
/*!****************************************************************************
*
*  pal_pluginsUnload
*
*  This routine is called to unload previously loaded plugins from
*  pal_pluginsLoad.
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_pluginsUnload(struct a2b_PluginApi *plugins, a2b_UInt16 numPlugins, A2B_ECB* ecb )
{
    A2B_UNUSED(plugins);
    A2B_UNUSED(ecb);

#ifdef A2B_STATIC_PLUGIN
    /* Do nothing */
#endif

    return A2B_RESULT_SUCCESS;
}


/*=============================================================================
 *
 *  Memory PAL
 *
 *=============================================================================
 */

#if !defined(A2B_FEATURE_MEMORY_MANAGER)
 /*!****************************************************************************
*
*  pal_memMgrInit
*
*  This routine is called to do initialization required by the memory manager
*  service during the stack [private] allocation process.
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_memMgrInit(A2B_ECB *ecb)
{
    A2B_UNUSED( ecb );
    return A2B_RESULT_SUCCESS;
}


/*!****************************************************************************
*
*  pal_memMgrOpen
*
*  This routine opens a memory managed heap located at the specified address
*  and of the specified size. If the A2B stack's heap cannot be opened and
*  managed at the specified location (perhaps because the size is insufficient)
*  then the returned handle will be A2B_NULL. The managed heap will use
*  memory pools to avoid fragmentation within the managed region.
*
*  return:         If non-zero the open was considered sucessfully.  If A2B_NULL
*                  memory initialization must have failed and the stack
*                  allocation will fail.
*
******************************************************************************/
a2b_Handle pal_memMgrOpen(a2b_Byte *heap, a2b_UInt32  heapSize)
{
    A2B_UNUSED( heap );
    A2B_UNUSED( heapSize );

    /* MUST be a non-NULL value -- Dummy value */
    return (a2b_Handle)1;
}


/*!****************************************************************************
*
*  pal_memMgrMalloc
*
*  This routine is called to get/allocate a fixed amount of memory.
*
*  return:         Returns an aligned pointer to memory or A2B_NULL if memory
*                  could not be allocated.
*
******************************************************************************/
void *pal_memMgrMalloc(a2b_Handle  hnd, a2b_UInt32  size)
{
    A2B_UNUSED( hnd );
    A2B_UNUSED( size );
    return A2B_NULL;
}


/*!****************************************************************************
*
*  pal_memMgrFree
*
*  This routine is called to free previously allocated memory.
*
*  return:         None
*
******************************************************************************/
void pal_memMgrFree(a2b_Handle  hnd, void *p)
{
    A2B_UNUSED( hnd );
    A2B_UNUSED( p );
}


/*!****************************************************************************
*
*  pal_memMgrClose
*
*  This routine is called to de-initialization the memory management subsystem
*  during the stack destroy process.  All resources associated with the
*  heap are freed.
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_memMgrClose(a2b_Handle  hnd)
{
    A2B_UNUSED( hnd );
    return A2B_RESULT_SUCCESS;
}


/*!****************************************************************************
*
*  pal_memMgrShutdown
*
*  This routine is called to shutdown the memory manager subsystem
*  during the stack destroy process.  This routine is called immediately
*  after the pal_memMgrClose (assuming the close was successful).
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_memMgrShutdown(A2B_ECB*    ecb)
{
    A2B_UNUSED( ecb );
    return A2B_RESULT_SUCCESS;
}
#endif /* A2B_FEATURE_MEMORY_MANAGER */

/*=============================================================================
 *
 *  Info PAL
 *
 *=============================================================================
 */
/*!****************************************************************************
*
*  pal_infoGetVersion
*
*  return:  None
*
******************************************************************************/
void
pal_infoGetVersion(a2b_UInt32*  major, a2b_UInt32*  minor, a2b_UInt32*  release)
{
    if ( A2B_NULL != major ) {
        *major = A2B_PAL_VER_MAJOR;
    }

    if ( A2B_NULL != minor ) {
        *minor = A2B_PAL_VER_MINOR;
    }

    if ( A2B_NULL != release ) {
        *release = A2B_PAL_VER_RELEASE;
    }
}


/*!****************************************************************************
*
*  pal_infoGetBuild
*
*  return:  None
*
******************************************************************************/
void
pal_infoGetBuild(
   a2b_UInt32*             buildNum,
   const a2b_Char** const  buildDate,
   const a2b_Char** const  buildOwner,
   const a2b_Char** const  buildSrcRev,
   const a2b_Char** const  buildHost
)
{
    if ( A2B_NULL != buildNum ) {
        *buildNum = 1;
    }

    if ( A2B_NULL != buildDate ) {
        *buildDate = __DATE__;
    }

    if ( A2B_NULL != buildOwner ) {
        *buildOwner = A2B_NULL;
    }

    if ( A2B_NULL != buildSrcRev ) {
        *buildSrcRev = A2B_NULL;
    }

    if ( A2B_NULL != buildHost ) {
        *buildHost = A2B_NULL;
    }
}



/*=============================================================================
 *
 *  I2C PAL
 *
 *=============================================================================
 */
/*!****************************************************************************
*
*  pal_i2cInit
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_i2cInit(A2B_ECB *ecb)
{
    A2B_UNUSED(ecb);
    return A2B_RESULT_SUCCESS;
}


/*!****************************************************************************
*
*  pal_i2cOpen
*
*  This routine is called to do post-initialization the I2C subsystem
*  during the stack allocation process.  This routine is called immediately
*  after the pal_i2cInit (assuming the init was successful).
*                                                                       <br><br>
*  return:         If non-zero the open was considered sucessfully.  If A2B_NULL
*                  I2C initialization must have failed and the stack
*                  allocation will fail.  This returned handle will be passed
*                  back into the other PAL I2C (pal_i2cXXX) API calls.
*
******************************************************************************/
a2b_Handle pal_i2cOpen(a2b_I2cAddrFmt  fmt, a2b_I2cBusSpeed speed, A2B_ECB *ecb)
{
    A2B_UNUSED(fmt);
    A2B_UNUSED(speed);
    return (a2b_Handle)(((a2b_helperContext *)ecb->palEcb.usrPtr)->twiHandle);
}


/*!****************************************************************************
*
*  pal_i2cClose
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_i2cClose(a2b_Handle  hnd)
{
    A2B_UNUSED(hnd);
    return A2B_RESULT_SUCCESS;
}


/* pal_i2cRead() MUST BE IMPLEMENTED FOR A FUNCTIONAL STACK */
/*!****************************************************************************
*
*  pal_i2cRead
*
*  This routine reads bytes from an I2C device.
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_i2cRead(a2b_Handle hnd,
   a2b_UInt16 addr, a2b_UInt16  nRead, a2b_Byte*   rBuf)
{
    sTWI *a2bTwiHandle = (sTWI *)hnd;
    TWI_SIMPLE_RESULT result;
    a2b_HResult status = A2B_RESULT_SUCCESS;

    result = twi_read(a2bTwiHandle, addr, rBuf, nRead);
    if (result != TWI_SIMPLE_SUCCESS) {
    status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_I2C,
                              A2B_EC_IO);
    }

    return status;
}


/* pal_i2cWrite() MUST BE IMPLEMENTED FOR A FUNCTIONAL STACK */
/*!****************************************************************************
*
*  pal_i2cWrite
*
*  This routine writes bytes to an I2C device.
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_i2cWrite(a2b_Handle hnd,
   a2b_UInt16 addr, a2b_UInt16 nWrite, const a2b_Byte *wBuf)
{
    sTWI *a2bTwiHandle = (sTWI *)hnd;
    TWI_SIMPLE_RESULT result;
    a2b_HResult status = A2B_RESULT_SUCCESS;

    result = twi_write(a2bTwiHandle, addr, (uint8_t *)wBuf, nWrite);
    if (result != TWI_SIMPLE_SUCCESS) {
    status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_I2C,
                              A2B_EC_IO);
    }

    return status;
}


/* pal_i2cWriteRead() MUST BE IMPLEMENTED FOR A FUNCTIONAL STACK */
/*!****************************************************************************
*
*  pal_i2cWriteRead
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_i2cWriteRead(a2b_Handle hnd,
   a2b_UInt16 addr, a2b_UInt16 nWrite, const a2b_Byte *wBuf,
    a2b_UInt16 nRead, a2b_Byte *rBuf)
{
    sTWI *a2bTwiHandle = (sTWI *)hnd;
    TWI_SIMPLE_RESULT result;
    a2b_HResult status = A2B_RESULT_SUCCESS;

    result = twi_writeRead(a2bTwiHandle, addr,
        (uint8_t *)wBuf, nWrite, rBuf, nRead);
    if (result != TWI_SIMPLE_SUCCESS) {
    status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_I2C,
                              A2B_EC_IO);
    }

    return status;
}


/*!****************************************************************************
*
*  pal_i2cShutdown
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_i2cShutdown(A2B_ECB *ecb)
{
    A2B_UNUSED(ecb);
    return A2B_RESULT_SUCCESS;
}

/*=============================================================================
 *
 *  Audio PAL
 *
 *=============================================================================
 */

/*!****************************************************************************
*
*   pal_audioInit
*
*  This routine is called to do initialization the audio subsystem
*  during the stack allocation process.
*
******************************************************************************/
a2b_HResult pal_audioInit(A2B_ECB *ecb)
{
    A2B_UNUSED(ecb);
    return A2B_RESULT_SUCCESS;
}

/*!****************************************************************************
*
*  pal_audioOpen
*
*  This routine is called to do post-initialization the audio subsystem
*  during the stack allocation process.  This routine is called immediately
*  after the pal_audioInit (assuming the init was successful).
*
******************************************************************************/
a2b_Handle
pal_audioOpen(void)
{
    /* Return non-NULL for success */
    return (a2b_Handle)1;
}


/*!****************************************************************************
*
*  pal_audioClose
*
*  This routine is called to de-initialization the audio subsystem
*  during the stack destroy process.
*
******************************************************************************/
a2b_HResult pal_audioClose(a2b_Handle hnd)
{
    A2B_UNUSED(hnd);
    return A2B_RESULT_SUCCESS;
}


/*!****************************************************************************
*
*  pal_audioConfig
*
*  This routine is called to configure the audio subsystem master node
*  during the discovery process.  This routine is called during the
*  "NetComplete" process after all nodes are discovered and before the master
*  node "NodeComplete" process which fully initializes the master A2B
*  registers and starts the up/downstream flow.
*
*  The application will typically override this routine so it will get the TDM
*  settings of the complete A2B network and can properly setup the audio
*  that is transmitted into the master node to "feed" the network.
*
******************************************************************************/
a2b_HResult pal_audioConfig(a2b_Handle hnd, a2b_TdmSettings *tdmSettings
    )
{
    A2B_UNUSED(hnd);
    A2B_UNUSED(tdmSettings);
    return A2B_RESULT_SUCCESS;
}


/*!****************************************************************************
*
*  pal_audioShutdown
*
*  This routine is called to shutdown the audio subsystem
*  during the stack destroy process.  This routine is called immediately
*  after the pal_audioClose (assuming the close was successful).
*
******************************************************************************/
a2b_HResult pal_audioShutdown(A2B_ECB *ecb)
{
    A2B_UNUSED(ecb);
    return A2B_RESULT_SUCCESS;
}


/*=============================================================================
 *
 *  PAL Log
 *
 *=============================================================================
 */
/*!****************************************************************************
*
*  pal_logInit
*
*  This routine is called to do initialization the log subsystem
*  during the stack allocation process.
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_logInit(A2B_ECB *ecb)
{
   A2B_UNUSED(ecb);
   return A2B_RESULT_SUCCESS;
}


/*!****************************************************************************
*
*  pal_logShutdown
*
*  This routine is called to shutdown the log subsystem
*  during the stack destroy process.  This routine is called immediately
*  after the pal_logClose (assuming the close was successful).
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_logShutdown(A2B_ECB *ecb)
{
   A2B_UNUSED(ecb);
   return A2B_RESULT_SUCCESS;
}


/*!****************************************************************************
*
*  pal_logOpen
*
*  This routine is called to do post-initialization the log subsystem
*  during the stack allocation process.  This routine is called immediately
*  after the pal_logInit (assuming the init was successful).
*
*  return:         If non-zero the open was considered sucessfully.  If A2B_NULL
*                  I2C initialization must have failed and the stack
*                  allocation will fail.  This returned handle will be passed
*                  back into the other PAL log (pal_logXXX) API calls.
*
******************************************************************************/
a2b_Handle pal_logOpen(const a2b_Char* url)
{
   FILE *seq = fopen(url, "w");
   return (a2b_Handle)seq;
}


/*!****************************************************************************
*
*  pal_logClose
*
*  This routine is called to de-initialization the log subsystem
*  during the stack destroy process.
*
*  return:         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_logClose(a2b_Handle  hnd)
{
   fclose((FILE *)hnd);
   return A2B_RESULT_SUCCESS;
}


/*!****************************************************************************
*
*  pal_logWrite
*
*  This routine writes to a log channel.
*
*  return         A status code that can be checked with the A2B_SUCCEEDED()
*                 or A2B_FAILED() for success or failure.
*
******************************************************************************/
a2b_HResult pal_logWrite(a2b_Handle hnd, const a2b_Char* msg)
{
   fputs(msg, (FILE *)hnd); fputs("\n", (FILE *)hnd);
   return A2B_RESULT_SUCCESS;
}


/*=============================================================================
 *
 *  PAL Init
 *
 *=============================================================================
 */

/*!****************************************************************************
*
*  a2b_palInit
*
*  Initializes the Platform Abstraction Layer (PAL) structure [function
*  pointers] with defaults for the given platform. These defaults can be
*  over-ridden as needed if necessary.
*
*  return:         None
*
******************************************************************************/
void a2b_palInit(struct a2b_StackPal *pal, A2B_ECB *ecb)
{
   if ( A2B_NULL != pal )
   {
      /* Do necessary base initialization */
      a2b_memset(pal, 0, sizeof(*pal));
      a2b_memset(ecb, 0, sizeof(*ecb));
      a2b_stackPalInit(pal, ecb);

#ifndef A2B_FEATURE_MEMORY_MANAGER
      pal->memMgrInit      = pal_memMgrInit;
      pal->memMgrOpen      = pal_memMgrOpen;
      pal->memMgrMalloc    = pal_memMgrMalloc;
      pal->memMgrFree      = pal_memMgrFree;
      pal->memMgrClose     = pal_memMgrClose;
      pal->memMgrShutdown  = pal_memMgrShutdown;
#endif

      pal->timerInit       = pal_timerInit;
      pal->timerGetSysTime = pal_timerGetSysTime;
      pal->timerShutdown   = pal_timerShutdown;

      pal->logInit         = pal_logInit;
      pal->logOpen         = pal_logOpen;
      pal->logClose        = pal_logClose;
      pal->logWrite        = pal_logWrite;
      pal->logShutdown     = pal_logShutdown;

      pal->i2cInit         = pal_i2cInit;
      pal->i2cOpen         = pal_i2cOpen;
      pal->i2cClose        = pal_i2cClose;
      pal->i2cRead         = pal_i2cRead;
      pal->i2cWrite        = pal_i2cWrite;
      pal->i2cWriteRead    = pal_i2cWriteRead;
      pal->i2cShutdown     = pal_i2cShutdown;

      pal->audioInit       = pal_audioInit;
      pal->audioOpen       = pal_audioOpen;
      pal->audioClose      = pal_audioClose;
      pal->audioConfig     = pal_audioConfig;
      pal->audioShutdown   = pal_audioShutdown;

      pal->pluginsLoad     = pal_pluginsLoad;
      pal->pluginsUnload   = pal_pluginsUnload;

      pal->getVersion      = pal_infoGetVersion;
      pal->getBuild        = pal_infoGetBuild;

      if ( A2B_NULL != ecb )
      {
      }
   }
}
