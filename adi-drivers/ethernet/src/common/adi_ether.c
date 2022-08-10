/*!
 ******************************************************************************
 *
 * @file:    adi_ether.c
 *
 * @brief:   Ethernet driver interface driver which routes the Ethernet calls to
 *           the underlying physical drivers.
 *
 * @version: $Revision: 25625 $
 *
 * @date:    $Date: 2016-03-18 07:26:22 -0400 (Fri, 18 Mar 2016) $
 * -----------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2016 Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Modified versions of the software must be conspicuously marked as such.
 * - This software is licensed solely and exclusively for use with processors
 *   manufactured by or for Analog Devices, Inc.
 * - This software may not be combined or merged with other code in any manner
 *   that would cause the software to become subject to terms and conditions
 *   which differ from those listed here.
 * - Neither the name of Analog Devices, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 * - The use of this software may or may not infringe the patent rights of one
 *   or more patent holders.  This license does not release you from the
 *   requirement that you obtain separate licenses from these patent holders
 *   to use this software.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF CLAIMS OF INTELLECTUAL
 * PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
#include <stdio.h>

#if !defined(__ADSPSC589_FAMILY__)
#include <adi_types.h>
#else
#include <stdint.h>
#include <stdbool.h>
typedef char char_t;
#endif
#include "adi_ether.h"

/*
 * adi_ether_misra.h suppresses certain MISRA warnings  */
#ifdef _MISRA_RULES
#pragma diag(push)
#include "adi_ether_misra.h"
#endif

/** \addtogroup Ethernet_Driver Ethernet Driver Interface
 *  @{
 */

/* Maximum number of Entries in Handle-EntryPoint Map
 * Only two network interfaces are supported by default
 */
#define MAX_NUM_HANDLE_ENTRYPOINT_MAPS    2U

/* ADI_DRIVER_HANDLE_ENTRYPOINT_MAP structure maintains the mapping between
 * driver entrypoint and its handle.
 */
typedef struct ADI_DRIVER_HANDLE_ENTRYPOINT_MAP
{
    ADI_ETHER_HANDLE         hEtherHandle;  /*!< ethernet driver handle */
    ADI_ETHER_DRIVER_ENTRY  *pEntryPoint;   /*!< driver entry point     */
} ADI_DRIVER_HANDLE_ENTRYPOINT_MAP;

static ADI_DRIVER_HANDLE_ENTRYPOINT_MAP
DriverEntryPointArray[MAX_NUM_HANDLE_ENTRYPOINT_MAPS] =
       {
           { (ADI_ETHER_HANDLE)0, (ADI_ETHER_DRIVER_ENTRY*)0} ,
           { (ADI_ETHER_HANDLE)0, (ADI_ETHER_DRIVER_ENTRY*)0}
       };

/* Number of Driver Entries */
#define NUM_DRIVER_ENTRIES  (sizeof(DriverEntryPointArray)/ sizeof(ADI_DRIVER_ID_ENTRYPOINT_MAP))


/**
* @brief        Opens the Ethernet Device Driver
*
* @details      adi_ether_Open API opens the ethernet device driver and upon success
*               returns handle to it. This handle has to be used with all subsequent
*               driver APIs. Each ethernet driver exports its entrypoint via its header file.
*               Applications should include both adi_ether.h along with the driver
*               specific header file.
*
*               Until applications start the ethernet using adi_ether_EnableMAC
*               communication will not start. Applications can set any driver specific
*               configuration elements before enabling the EMAC.
*
* @param[in]    pEntryPoint     Pointer to the Driver Entry Point
*
* @param[in]    pDeviceInit     Pointer to Initialization Data Structure. The init
*                               structure is used to supply memory to the driver as
*                               well as operation of the cache. The supplied memory is
*                               used for storing the transmit and receive descriptors.
*                               Cache element is used to specify whether data cache
*                               is enable or disabled. If data cache is enabled driver
*                               performs additional functions for the cache-coherency.
*
* @param[in]    pfCallback      Pointer to Ethernet callback Function. This callback
*                               is used by the driver asynchronously to return the
*                               received packets as well as transmitted packets. With
*                               LWIP TCP/IP stack this callback has to be adi_lwip_StackcallBack.
*                               adi_lwip_StackcallBack is defined in LWIP. Applications
*                               that directly use the driver can hookup their own callback
*                               routines.
*
*
* @param[out]   phDevice        Upon success of this API phDevice holds the ethernet device
*                               handle.
*
* @param[in]   pUsrPtr          User pointer returned to callback
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS            if successful
*                  - ADI_ETHER_RESULT_FAILED             upon generic failure
*                  - ADI_ETHER_RESULT_NO_MEMORY          if insufficient amount memory is supplied
*                                                        to the driver
*                  - ADI_ETHER_RESULT_INVALID_PARAM  [D] if invalid parameter is passed. This
*                                                        check is carried only in debug build
*
* @note        Refer driver specific open function for more details.
*
* @sa          adi_ether_Close
*              adi_ether_EnableMAC
*/
ADI_ETHER_RESULT  adi_ether_Open(
                                 ADI_ETHER_DRIVER_ENTRY* const pEntryPoint,
                                 ADI_ETHER_DEV_INIT*     const pDeviceInit,
                                 ADI_ETHER_CALLBACK_FN   const pfCallback,
                                 ADI_ETHER_HANDLE*       const phDevice,
                                 void*                   const pUsrPtr
                                )
{
    ADI_ETHER_RESULT eResult;
    uint32_t nEntrySlot;

#if defined(ADI_DEBUG)
    if ((pEntryPoint == NULL) || (pDeviceInit == NULL)  ||
        (pfCallback == NULL)  || (phDevice ==NULL))
    {
        return (ADI_ETHER_RESULT_INVALID_PARAM);
    }
#endif

    /* Search for an empty slot in the Driver Map */
    for (nEntrySlot = 0U; nEntrySlot < MAX_NUM_HANDLE_ENTRYPOINT_MAPS; nEntrySlot++)
    {
        /* Is the Slot Empty ? */
        if (DriverEntryPointArray[nEntrySlot].hEtherHandle == ((ADI_ETHER_HANDLE)0))
        {
            /* Call the Driver Open Function */
            eResult = pEntryPoint->adi_ether_Open (pEntryPoint, pDeviceInit, pfCallback, phDevice, pUsrPtr);

            /* Return in case of any Error */
            if (eResult != ADI_ETHER_RESULT_SUCCESS)
            {
                return eResult;
            }

            /* Map the Handle to the Entry Point */
            DriverEntryPointArray[nEntrySlot].hEtherHandle = *phDevice;
            DriverEntryPointArray[nEntrySlot].pEntryPoint = pEntryPoint;

            /* Return Success */
            return ADI_ETHER_RESULT_SUCCESS;
        }
    }

    /* No Slots Available and hence return Error */
    return ADI_ETHER_RESULT_FAILED;
}

/*
* @brief        Get the driver entrypoint given the driver handle
*
* @details      The function searches the entry table for handle and returns`
*               the entry point corresponding to the given handle.In case of failure it returns
*               NULL. This function is not exported and used only by the ad_ether_xxx api's
*               This function will not validate the handle, it is expected that the
*               handle validation is performed by the actual API.
*
* @param[in]    pEtherHandle    Pointer to handler.
*
* @return       Entry of the handle on success
*               NULL upon failure
*
*/
static ADI_ETHER_DRIVER_ENTRY* GetEntryFromHandle(ADI_ETHER_HANDLE pEtherHandle)
{
    uint32_t i;

    for(i = 0U ; i< MAX_NUM_HANDLE_ENTRYPOINT_MAPS ; i++ )
    {
       if(pEtherHandle == DriverEntryPointArray[i].hEtherHandle)
       {
               return(DriverEntryPointArray[i].pEntryPoint);
       }
    }

    return(NULL);
}

/**
* @brief        Submits single or list of buffers for receiving ethernet packets
*
* @details      The function supplies single or the given linked list of buffer(s) to the
*               underlying ethernet device driver. These buffers are used for receiving
*               ethernet packets.Once buffers are supplied to the driver
*               applications should not access the buffer or buffer elements until buffer
*               is returned via the callback. Once buffer is submitted to he driver,driver
*               owns it. Ethernet device drivers operate only in asynchronous mode using
*               callbacks, so adi_ether_Read never blocks and returns immediately whether
*               data is available or not.
*
*               By submitting the list of buffers improves the throughput of the system.
*               Typically these buffers will be bound with a descriptor and put in DMA queue.
*               As ethernet receives packets it consumes a buffer and DMA fills the incoming data
*               in to the buffer.
*
*               Drivers operating in conjunction with LWIP TCP/IP stack use adi_lwip_StackcallBack
*               to return the received packets.
*
*               For DMA based systems flush/invalidating the cached buffer regions is handled
*               by the ethernet device drivers. Once buffer is submitted to the driver, accessing
*               driver owned buffer data or elements of buffer by the application may lead to
*               invalid results because of cache coherency.
*
*               pBuffer->Data format:
*
*               | Length | Ethernet Header |  IP etc ...
*               <-2byte->|<- 14 bytes    ->|
*
*               The first two bytes of the Data field holds the length of the packet including
*               the two bytes.
*
*               For applications using LWIP TCP/IP stack, the low level wrapper will configure
*               the buffer for the ethernet device driver. Applications simply use BSD socket
*               APIs to send and receive data using sockets.
*
* @param[in]    hDevice         Handle to Ethernet Device
*
* @param[in]    pBuffer         Pointer to buffer linked list to be supplied to driver
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS           if successful
*                  - ADI_ETHER_RESULT_FAILED            upon failure
*                  - ADI_ETHER_RESULT_NULL_BUFFER       if buffer pointer is NULL
*                  - ADI_ETHER_RESULT_INVALID_PARAM [D] upon parameter error
*
* @note         Refer driver specific Read function for more details
*
* @sa           adi_ether_Write
*/
ADI_ETHER_RESULT  adi_ether_Read (
                                  ADI_ETHER_HANDLE  const hDevice,
                                  ADI_ETHER_BUFFER *pBuffer
                                 )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return(pDriverEntry->adi_ether_Read(hDevice,pBuffer));
}

/**
* @brief        Submits single or list of buffers for transmission over ethernet
*
* @details      Applications use adi_ether_Write function to transmit ethernet packets over
*               network. This API will call the underlying driver specific API via the entry
*               point. adi_ether_Write operates completely asynchronously, so this call will
*               immediately return. Return this call does not imply successful transmission
*               of submitted buffer but return of this buffer via callback ensures the successful
*               transmission of the data. Once buffer is submitted via this API, application
*               should not use this buffer until it is returned via callback.
*
*               Using LWIP TCP/IP stack, transmitted buffers are returned via adi_lwip_StackcallBack
*               handler. This callback handler is part of LWIP subsystem and should not be
*               defined by the application. A single transmitted buffer or list of transmitted
*               buffers may be returned by the driver.
*
*               For data cache enabled systems ethernet device driver will perform flush/invalidate
*               functions for the buffered cache regions. This will ensure cache-coherency.
*
*               Applications that are using the ethernet driver directly must set the first
*               two bytes to the length of the entire frame including the two byte length field.
*
*               pBuffer->Data format:
*
*               | Length | Ethernet Header |  IP etc ...
*
*               <-2byte->|<- 14 bytes    ->|
*
* @param[in]    hDevice         Handle to Ethernet Device
*
* @param[in]    pBuffer         Pointer to buffer linked list containing the data to
*                               to be written to.
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS           if successful
*                  - ADI_ETHER_RESULT_FAILED            upon failure
*                  - ADI_ETHER_RESULT_NULL_BUFFER       if buffer list is a NULL pointer
*                  - ADI_ETHER_RESULT_INVALID_PARAM [D] upon parameter error
*
* @note         Refer driver specific Write function for more details
*
* @sa           adi_ether_Read
*/
ADI_ETHER_RESULT  adi_ether_Write(
                                      ADI_ETHER_HANDLE  const hDevice,
                                      ADI_ETHER_BUFFER *pBuffer
                                      )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }
    return(pDriverEntry->adi_ether_Write(hDevice,pBuffer));
}

/**
* @brief        Close the ethernet device driver
*
* @details      This function close the ethernet device driver. Any further operations
*               using the handle will result in failure.
*
*               Close will disables the ethernet MAC and shuts down any running DMAs.
*               Buffers that are in transient state will not be returned to the application.
*
* @param[in]    hDevice         Handle to Ethernet Device
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS           if successful
*                  - ADI_ETHER_RESULT_FAILED            upon failure
*                  - ADI_ETHER_RESULT_INVALID_PARAM [D] upon parameter error
*
* @note         Refer to driver specific Close for more details
*
* @sa           adi_ether_Open
*/
ADI_ETHER_RESULT  adi_ether_Close (ADI_ETHER_HANDLE hDevice)
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);
    uint32_t i;

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    /* make the slot available */
    for(i = 0U ; i< MAX_NUM_HANDLE_ENTRYPOINT_MAPS ; i++ )
    {
       if(hDevice == DriverEntryPointArray[i].hEtherHandle)
       {
            DriverEntryPointArray[i].hEtherHandle = (ADI_ETHER_HANDLE)0;
            break;
       }
    }

    return(pDriverEntry->adi_ether_Close(hDevice));
}

/**
* @brief        Return the Link Status
*
* @details      adi_ether_GetLinkStatus is used to get the current link status. If link is
*               up it returns true. If link is down it will return false. Applications should
*               wait until link is up before performing any network related operations.
*
*               This API is used after enabling MAC via adi_ether_EnableMAC
*
* @param[in]    hDevice         Handle to Ethernet Device
*
* @return       Link Status
*                  - true if the link is up
*                  - false if the link is down or on error.
*/
bool  adi_ether_GetLinkStatus(ADI_ETHER_HANDLE hDevice)
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return (false);
    }

    return(pDriverEntry->adi_ether_GetLinkStatus(hDevice));
}

/**
* @brief        Add multicast group
*
* @details      Add the given multicast group address and enables appropriate bins.
*               From the given multicast group IP address multicast MAC address is
*               derived. Once a multicast group is added, underlying controller enables
*               the multicast functionality so that MAC will process the multicast frames.
*
*               Applications using lwIP subsystem will not directly access this API but this
*               API gets called from the IGMP protocol.Applications use standard BSD socket
*               interface and ioctlsocket() calls to enable multicast features.
*
* @param[in]    hDevice             Handle to Ethernet Device
*
* @param[in]    MultiCastGroupAddr  Multicast Group Address
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED  upon failure
*                  - ADI_ETHER_RESULT_INVALID_PARAM [D] upon parameter error
*
*
* @note         Refer to driver specific Multicast functions for more details
*
* @sa            adi_ether_DelMulticastFilter
*/
ADI_ETHER_RESULT  adi_ether_AddMulticastFilter(
                                               ADI_ETHER_HANDLE  hDevice,
                                               const uint32_t    MultiCastGroupAddr
                                                    )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }
    return(pDriverEntry->adi_ether_AddMulticastFilter(hDevice,MultiCastGroupAddr));
}

/**
* @brief        Delete multicast group
*
* @details      adi_ether_DelMulticastFilter removes the multicast MAC address from
*               the multicast hash bins. Multicast MAC address is derived from the multicast
*               group IP address and will be disabled at the driver level. Internet Group
*               Message Protocol (IGMP) uses multicasting.
*
*               Applications using lwIP subsystem will not directly access this API but this
*               API gets called from the IGMP protocol.Applications use standard BSD socket
*               interface and ioctlsocket() calls to enable multicast features.
*
* @param[in]    hDevice             Handle to Ethernet Device
*
* @param[in]    MultiCastGroupAddr  Multicast Group Address
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*                  - ADI_ETHER_RESULT_INVALID_PARAM [D] upon parameter error
*
* @note         Refer to driver specific Multicast functions for more details
*
* @sa           adi_ether_AddMulticastFilter
*/
ADI_ETHER_RESULT  adi_ether_DelMulticastFilter(
                                               ADI_ETHER_HANDLE hDevice,
                                               const uint32_t   MultiCastGroupAddr
                                               )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return(pDriverEntry->adi_ether_DelMulticastFilter(hDevice,MultiCastGroupAddr));
}



/**
* @brief        Get the MAC address of the interface
*
* @details      The function returns the MAC address in pMacAddress. Typically MAC address
*               resides in non-volatile memory. Each ethernet driver implements this function
*               differently depending on the underlying hardware. Typically MAC address is
*               stored in the one of the non-volatile medias (flash, EEPROM, OTP). This API
*               will return the MAC address present in the hardware.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[out]   pMacAddress    Pointer to MAC address variable
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_SUCCESS  upon failure
*                  - ADI_ETHER_RESULT_INVALID_PARAM [D] upon parameter error
*
* @note         Refer to driver specific function for more details
*
* @sa           adi_ether_SetMACAddress
*/
ADI_ETHER_RESULT  adi_ether_GetMACAddress(
                                          ADI_ETHER_HANDLE   hDevice,
                                          uint8_t           *pMacAddress
                                          )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return(pDriverEntry->adi_ether_GetMACAddress(hDevice,pMacAddress));
}

/**
* @brief        Set the MAC address of the interface
*
* @details      adi_ether_SetMACAddress sets the MAC address in the ethernet driver. It
*               allows application use to MAC even non-volatile memory is not configured
*               with a MAC address.This API may not update the MAC address in the non-volatile
*               memory.Refer to the documentation of the underlying ethernet driver for details.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    pMacAddress    Pointer to MAC address
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_SUCCESS  upon failure
*                  - ADI_ETHER_RESULT_INVALID_SEQUENCE if called after MAC is enabled
*                  - ADI_ETHER_RESULT_INVALID_PARAM [D] upon parameter error
*
* @sa           adi_ether_GetMACAddress
*/
ADI_ETHER_RESULT  adi_ether_SetMACAddress (
                                           ADI_ETHER_HANDLE  hDevice,
                                           const uint8_t    *pMacAddress
                                           )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return(pDriverEntry->adi_ether_SetMACAddress(hDevice,pMacAddress));
}

/**
* @brief        Configures and enables MAC and PHY
*
* @details      adi_ether_EnableMAC is a critical function that enables the MAC. This API
*               enables the underlying PHY, triggers the auto-negotiation by default. After
*               auto-negotiation is completed, auto negotiation results are used to configure
*               the MAC. Critical configuration setup includes duplex mode, speed. Applications
*               typically issue this API to enable MAC after configuration setup. Applications
*               must supply receive buffers to the driver using adi_ether_Read before enabling
*               the EMAC.
*
*               Any user specific configuration changes to the driver can be done before issuing
*               this API. This API consumes currently configured entities and enables MAC with
*               those parameters.
*
* @param[in]    hDevice         Handle to Ethernet Device
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*                  - ADI_ETHER_RESULT_INVALID_PARAM [D] upon parameter error
*
* @note         Refer to underlying driver documentation for more details
*
*/
ADI_ETHER_RESULT  adi_ether_EnableMAC (ADI_ETHER_HANDLE hDevice)
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return(pDriverEntry->adi_ether_EnableMAC(hDevice));
}

/**
* @brief        Get the Buffer Prefix
*
* @details      Buffer prefix allows additional memory that driver reserves for its own purpose.
*               lwIP subsystem queries underlying ethernet driver for any additional memory
*               specified via prefix. While processing the packets lwIP subsystem skips the prefix
*               number of bytes.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[out]   pBufferPrefix  Return the buffer prefix in this variable
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*                  - ADI_ETHER_RESULT_INVALID_PARAM [D] upon parameter error
*/
ADI_ETHER_RESULT adi_ether_GetBufferPrefix (
                                            ADI_ETHER_HANDLE hDevice,
                                            uint32_t* const  pBufferPrefix
                                            )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return (pDriverEntry->adi_ether_GetBufferPrefix (hDevice, pBufferPrefix));
}

ADI_ETHER_RESULT adi_ether_SetSrcAddrFilt(ADI_ETHER_HANDLE hDevice,
                                          const uint32_t IpAddr,
                                          const uint8_t IpMaskBits,
                                          const bool invert)
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return (pDriverEntry->adi_ether_SetSrcAddrFilt(hDevice, IpAddr, IpMaskBits, invert));
}

ADI_ETHER_RESULT adi_ether_SetSrcAddrFiltEnable(ADI_ETHER_HANDLE hDevice,
                                                const bool enable)
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return (pDriverEntry->adi_ether_SetSrcAddrFiltEnable(hDevice, enable));
}

ADI_ETHER_RESULT adi_ether_SetDstPortFilt(ADI_ETHER_HANDLE hDevice,
                                          const uint16_t Port,
                                          const bool udp,
                                          const bool invert)
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return (pDriverEntry->adi_ether_SetDstPortFilt(hDevice, Port, udp, invert));
}

ADI_ETHER_RESULT adi_ether_SetDstPortFiltEnable(ADI_ETHER_HANDLE hDevice,
                                                const bool enable)
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    return (pDriverEntry->adi_ether_SetDstPortFiltEnable(hDevice, enable));
}

#ifdef _MISRA_RULES
/* Rule 11.1(Req) : Conversions shall not be performed between a pointer to a function and any type other than an integral type. */
#pragma diag (suppress: misra_rule_11_1: "To support passing callback function to module system")
#endif


#if defined(ADI_ETHER_SUPPORT_PTP)

/**
* @brief        Configure the PTP Module
*
* @details      Configure the PTP module. This module should be configured and enabled
*               to support timestamping of the packets. This also start the clock.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    pConfig        Pointer to the PTP configuration structure.The structure used is device/controller
*                              specific and will be available in the device/controller specific header file.
*
* @param[in]    pfCallback     PPT Callback function.
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
ADI_ETHER_RESULT adi_ether_ptp_Config (
                                        ADI_ETHER_HANDLE        const hDevice,
                                        void*                   const pConfig,
                                        ADI_ETHER_CALLBACK_FN   const pfCallback
                                        )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_PTP,
    									     ADI_ETHER_MODULE_FUNC_PTP_CFG,
    									     (void*)pConfig,
    									     (void*)pfCallback,
    									     NULL
    										 ));

}

/**
* @brief        Enable/Disable the timestamping of the packet.
*
* @details      Enable/Disable timestamping of the packet.
*
* @param[in]    bEnable        'true' Enable and 'false' disables the timestamping.
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
ADI_ETHER_RESULT adi_ether_ptp_Enable (
                                       ADI_ETHER_HANDLE const hDevice,
                                       bool             const bEnable
                                       )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_PTP,
    									     ADI_ETHER_MODULE_FUNC_PTP_EN,
    									     (void*)bEnable,
    									     NULL,
    									     NULL
    										 ));
}

/**
* @brief        Get the current time of the PTP module
*
* @details      Get the current time of the clock running in the PTP module.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    pTime          Pointer to the ADI_ETHER_TIME structure to which the
*                              current time will be written.
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
ADI_ETHER_RESULT adi_ether_ptp_GetCurrentTime (
                                               ADI_ETHER_HANDLE  const hDevice,
                                               ADI_ETHER_TIME*   const pTime
                                               )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_PTP,
    									     ADI_ETHER_MODULE_FUNC_PTP_GET_CUR_TIME,
    									     (void*)pTime,
    									     NULL,
    									     NULL
    										 ));
}

/**
* @brief        Update the input clock frequency.
*
* @details      The input clock frequency may have drifted and hence caused the PTP
*               clock frequency to drift. The input clock frequency can be updated
*               with this API. This causes the driver to adjust the registers so that
*               PTP clock frequency will be corrected.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    nClkFreq       Clock frequency of the input clock.
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
#ifdef LEGACY_EMAC_PTP_UPDATE
ADI_ETHER_RESULT adi_ether_ptp_UpdateClkFreq (
                                              ADI_ETHER_HANDLE  const hDevice,
                                              uint32_t          const nClkFreq
                                              )
#else
ADI_ETHER_RESULT adi_ether_ptp_UpdateClkFreq (
                                              ADI_ETHER_HANDLE  const hDevice,
                                              float32_t *       const nClkFreq
                                              )
#endif
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_PTP,
    									     ADI_ETHER_MODULE_FUNC_PTP_UPDATE_FREQ,
    									     (void*)nClkFreq,
    									     NULL,
    									     NULL
    										 ));
}

/**
* @brief        Apply an offset to the PTP time.
*
* @details      Add/Subtract the given time from the PTP time.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    pTime          Pointer to ADI_ETHER_TIME structure.
*
* @param[in]    bAddTime       'true' to add and 'false' to subtract
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
ADI_ETHER_RESULT adi_ether_ptp_ApplyTimeOffset (
                                                ADI_ETHER_HANDLE const hDevice,
                                                ADI_ETHER_TIME*  const pTime,
                                                bool             const bAddTime
                                                )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_PTP,
    									     ADI_ETHER_MODULE_FUNC_PTP_APPLY_TIME_OFFSET,
    									     (void*)pTime,
    									     (void*)bAddTime,
    									     NULL
    										 ));
}

#endif /* ADI_ETHER_SUPPORT_PTP */


#if defined(ADI_ETHER_SUPPORT_AV)

/**
* @brief        Set the given AV profile.
*
* @details      The AV will be configured for the given profile.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    pAVProfile     Pointer to the AV profile structure. The structure used is device/controller
*                              specific and will be available in the device/controller specific header file.
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
ADI_ETHER_RESULT adi_ether_av_SetProfile (
                                          ADI_ETHER_HANDLE const hDevice,
                                          void*            const pAVProfile
                                          )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_AV,
    									     ADI_ETHER_MODULE_FUNC_AV_SET_PROFILE,
    									     (void*)pAVProfile,
    									     NULL,
    									     NULL
    										 ));
}

#endif /* ADI_ETHER_SUPPORT_AV */

#if defined(ADI_ETHER_SUPPORT_PPS)

/**
* @brief        Configure the PPS
*
* @details      The PPS will be configured for the given configuration.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    nDeviceID      The device ID of PPS. This identifies the if multiple independent
*                              PPS generators are available.
*
* @param[in]    pPPSConfig     Pointer to the PPS configuration structure. This is device/controller specific.
*
* @param[in]    pfCallback     PPS Callback function.
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
ADI_ETHER_RESULT adi_ether_pps_Config (
                                       ADI_ETHER_HANDLE        const hDevice,
                                       uint32_t                const nDeviceID,
                                       void*                   const pPPSConfig,
                                       ADI_ETHER_CALLBACK_FN   const pfCallback
                                       )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_PPS,
    									     ADI_ETHER_MODULE_FUNC_PPS_CFG,
    									     (void*)nDeviceID,
    									     (void*)pPPSConfig,
    									     (void*)pfCallback
    										 ));
}

/**
* @brief        Enable/Disable PPS
*
* @details      Enable/Disabe PPS identified by the nDeviceNum
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    nDeviceID      The device ID of PPS. This identifies the PPS if multiple independent
*                              PPS generators are available.
*
* @param[in]    bEnable        'true' to Enable and 'false' to Disable
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
ADI_ETHER_RESULT adi_ether_pps_Enable (
                                      ADI_ETHER_HANDLE const hDevice,
                                      uint32_t         const nDeviceID,
                                      bool             const bEnable
                                      )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_PPS,
    									     ADI_ETHER_MODULE_FUNC_PPS_EN,
    									     (void*)nDeviceID,
    									     (void*)bEnable,
    									     NULL
    										 ));
}

#endif /* ADI_ETHER_SUPPORT_PPS */

#if defined(ADI_ETHER_SUPPORT_ALARM)

/**
* @brief        Configure the Alarm
*
* @details      The Alarm will be configured for the given target time.
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    nDeviceID      The device ID of Alarm. This identifies the Alarm if multiple independent
*                              alarms are available.
*
* @param[in]    pTime          Pointer to the time at which the alarm is triggered.
*
* @param[in]    pfCallback     Callback function which will be called when alarm is triggered.
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
ADI_ETHER_RESULT adi_ether_alarm_Config (
                                         ADI_ETHER_HANDLE        const hDevice,
                                         uint32_t                const nDeviceID,
                                         ADI_ETHER_TIME*         const pTime,
                                         ADI_ETHER_CALLBACK_FN   const pfCallback
                                         )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_ALARM,
    									     ADI_ETHER_MODULE_FUNC_ALARM_CFG,
    									     (void*)nDeviceID,
    									     (void*)pTime,
    									     (void*)pfCallback
    										 ));
}

/**
* @brief        Enable/Disable Alarm
*
* @details      Enable/Disabe Alarm identified by the nDeviceNum
*
* @param[in]    hDevice        Handle to Ethernet Device
*
* @param[in]    nDeviceNum     The device ID of Alarm. This identifies the Alarm if multiple independent
*                              alarms are available.
*
* @param[in]    bEnable        'true' to Enable and 'false' to Disable
*
* @return       Status
*                  - ADI_ETHER_RESULT_SUCCESS  if successful
*                  - ADI_ETHER_RESULT_FAILED   upon failure
*/
ADI_ETHER_RESULT adi_ether_alarm_Enable (
                                         ADI_ETHER_HANDLE const hDevice,
                                         uint32_t         const nDeviceID,
                                         bool             const bEnable
                                         )
{
    ADI_ETHER_DRIVER_ENTRY *pDriverEntry = GetEntryFromHandle(hDevice);

    if (!pDriverEntry)
    {
        return ADI_ETHER_RESULT_FAILED;
    }

    if (pDriverEntry->adi_ether_ModuleIO == NULL)
    {
    	return ADI_ETHER_RESULT_NOT_SUPPORTED;
    }

    return (pDriverEntry->adi_ether_ModuleIO (
    									     hDevice,
    									     ADI_ETHER_MODULE_ALARM,
    									     ADI_ETHER_MODULE_FUNC_ALARM_EN,
    									     (void*)nDeviceID,
    									     (void*)bEnable,
    									     NULL
    										 ));
}

#endif /* ADI_ETHER_SUPPORT_ALARM */

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif

/*@}*/
