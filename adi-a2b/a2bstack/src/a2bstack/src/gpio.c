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
 * \file:   gpio.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the A2B stack GPIO API.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/gpio.h"
#include "a2b/util.h"
#include "a2b/trace.h"
#include "a2b/error.h"
#include "a2b/regdefs.h"
#include "a2b/i2c.h"
#include "a2b/msg.h"
#include "a2b/defs.h"
#include "stack_priv.h"
#include "stackctx.h"


/*======================= D E F I N E S ===========================*/

/*
 * We make some optimizations when reading several of the GPIO registers
 * in the slave nodes to minimize the number of I2C transactions. As a result
 * we need to make sure the registers in question are consecutive.
 */
A2B_STATIC_ASSERT((a2b_UInt32)A2B_REG_GPIOOEN - (a2b_UInt32)A2B_REG_GPIODATCLR == (a2b_UInt32)1,
                    "Registers aren't contiguous");
A2B_STATIC_ASSERT((a2b_UInt32)A2B_REG_GPIODATCLR - (a2b_UInt32)A2B_REG_GPIODATSET == (a2b_UInt32)1,
                    "Registers aren't contiguous");
A2B_STATIC_ASSERT((a2b_UInt32)A2B_REG_GPIODATSET - (a2b_UInt32)A2B_REG_GPIODAT == (a2b_UInt32)1,
                    "Registers aren't contiguous");
A2B_STATIC_ASSERT((a2b_UInt32)A2B_REG_GPIOIN - (a2b_UInt32)A2B_REG_GPIOIEN == (a2b_UInt32)1,
                    "Registers aren't contiguous");

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
* 
*  \b   a2b_gpioOutSetAction
* 
*  This sets or clears the configured action for the enabled output GPIO pins
*  in a glitch-free manner. Pins that are asserted (1) in the pinMask will be
*  modified while those that aren't will remain unmodified by this operation.
*  Depending how the output of the GPIO pin is configured this may result in
*  the pin being driven low or high. If the GPIO pin is not configured for
*  output (e.g. not enabled) then this action will have no effect. Slave node
*  GPIO related actions can only be executed after A2B discovery and
*  configuration has occurred. Slave plugins <b>cannot</b> configure GPIO
*  pins that belong to another node. A master plugin or application, however,
*  can configure GPIO pins for any slave node.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   GPIO action.
* 
*  \param   [in]        nodeAddr    The address of the A2B node to access.
* 
*  \param   [in]        pinMask     The GPIO pin mask indicating the output
*                                   pin's output action to change. A "1" in
*                                   the mask indicates which pin's output
*                                   action to modify. A "0" in the mask
*                                   indicates that pin will **not** be
*                                   modified.
* 
*  \param   [in]        actionMask  The pinMask's corresponding action
*                                   (set/clear) to execute. A "1" means to
*                                   drive the output pin high while "0" drives
*                                   it low. Only pins/bits specified in the
*                                   pinMask that are "1" will modified.
* 
*  \pre     A2B discovery has already been conducted successfully if accessing
*           a slave node. It is *assumed* the GPIO pin has been configured as
*           an output pin during discovery.
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_gpioOutSetAction
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt8                   pinMask,
    a2b_UInt8                   actionMask
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte data[3];
    struct a2b_StackContext* mCtx;

    /* Make sure it's a valid node */
    if ( (A2B_NULL == ctx) || (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_GPIO,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to access the GPIO registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_GPIO,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* We need the master plugin context in order to make the
         * necessary I2C call.
         */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_GPIO,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            (void)a2b_memset(data, 0, sizeof(data));

            /* This is the starting address to write to */
            data[0] = A2B_REG_GPIODATSET;

            data[1 /* GPIODATSET */] = pinMask & actionMask;
            data[2 /* GPIODATCLR */] = pinMask ^ (actionMask & pinMask);

            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWrite(mCtx, sizeof(data), data);
            }
            else
            {
                result = a2b_i2cSlaveWrite(mCtx, nodeAddr, sizeof(data), data);
            }
        }
    }

    return result;

} /* a2b_gpioOutSetAction */


/*!****************************************************************************
* 
*  \b   a2b_gpioOutGetLevels
* 
*  This function returns the logic level of the GPIO output pins indicated in
*  the pin mask. The pin mask is created by OR'ing together the a2b_GpioPin
*  values for the pins that should be read. Only the pins asserted in the
*  returned 'enabledMask' (an "OR" of a2b_GpioPin values) are configured and
*  enabled for output (and thus have valid readings). Pins in the 'enabledMask'
*  not asserted are either configured for input or are in a HIGH-Z (high
*  impedance) state. Slave node GPIO related actions can only be executed after
*  A2B discovery and configuration has occurred. Slave plugins <b>cannot</b>
*  access GPIO pins that belong to another node. A master plugin or
*  application, however, can access GPIO pins for any slave node.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   GPIO request.
* 
*  \param   [in]        nodeAddr    The address of the A2B node to access.
* 
*  \param   [in]        pinMask     A logical <b>OR</b> of a2b_GpioPin
*                                   enumerations indicating the pins levels
*                                   to read.
* 
*  \param   [in,out]    levelMask   The mask containing the logic level of the
*                                   GPIO output pins as driven by the chip.
*                                   Only those bit positions asserted
*                                   in the 'enabledMask' are considered valid
*                                   output readings.
* 
*  \param   [in,out]    enabledMask The masking indicating which GPIO pins
*                                   are configured and enabled for output.
*                                   The mask is created by <b>OR</b>'ing
*                                   together a2b_GpioPin enumerated values.
*                                   If a bit position is <b>not</b> asserted
*                                   then the corresponding position and value
*                                   in the returned 'levelMask' is invalid and
*                                   does not provide an accurate reading.
* 
*  \pre     A2B discovery has already been conducted successfully if accessing
*           a slave node. It is *assumed* the GPIO pins <b>OR</b>'ed in the
*           'levelMask' have been configured as output pin(s).
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_gpioOutGetLevels
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt8                   pinMask,
    a2b_UInt8*                  levelMask,
    a2b_UInt8*                  enabledMask
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[1];
    a2b_Byte rBuf[4];
    struct a2b_StackContext* mCtx;

    /* Make sure the node address is valid */
    if ( (A2B_NULL == ctx) || (A2B_NULL == levelMask) ||
        (A2B_NULL == enabledMask) || (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_GPIO,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to access the GPIO registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_GPIO,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_GPIO,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            /* A2B_REG_GPIODAT is the smallest (base) register of the four
             * registers we're going to read. We can subtract that off any
             * successive register to determine it's value in the read buffer.
             */
            wBuf[0] = A2B_REG_GPIODAT;
            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWriteRead(mCtx,
                		(a2b_UInt16)A2B_ARRAY_SIZE(wBuf),
                                            &wBuf[0], (a2b_UInt16)A2B_ARRAY_SIZE(rBuf),
                                            &rBuf[0]);
            }
            else
            {
                result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr,
                		(a2b_UInt16)A2B_ARRAY_SIZE(wBuf),
                                            &wBuf[0], (a2b_UInt16)A2B_ARRAY_SIZE(rBuf),
                                            &rBuf[0]);
            }

            if ( A2B_SUCCEEDED(result) )
            {
                *enabledMask = rBuf[A2B_REG_GPIOOEN - A2B_REG_GPIODAT];
                *levelMask = *enabledMask & pinMask &
                                rBuf[0 /* A2B_REG_GPIODAT */];
            }
        }
    }

    return result;

} /* a2b_gpioOutGetLevels */


/*!****************************************************************************
* 
*  \b   a2b_gpioOutIsEnabled
* 
*  This function returns A2B_TRUE if the specified GPIO output pin is enabled
*  or A2B_FALSE otherwise. Slave node GPIO related actions can only be executed
*  after A2B discovery and configuration has occurred. Slave plugins
*  <b>cannot</b> access GPIO pins that belong to another node. A master plugin
*  or application, however, can access GPIO pins for any slave node.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   GPIO request.
* 
*  \param   [in]        nodeAddr    The address of the A2B node to access.
* 
*  \param   [in]        pin         The GPIO pin to access.
* 
*  \param   [in,out]    enabled     Set to A2B_TRUE if the GPIO output pin
*                                   is enabled or A2B_FALSE otherwise.
* 
*  \pre     A2B discovery has already been conducted successfully if accessing
*           a slave node. It is *assumed* the GPIO pin has been configured as
*           an output pin during discovery.
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_gpioOutIsEnabled
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_GpioPin                 pin,
    a2b_Bool*                   enabled
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[1];
    a2b_Byte rBuf[1];
    struct a2b_StackContext* mCtx;

    /* Make sure the node address is valid */
    if ( (A2B_NULL == ctx) || (A2B_NULL == enabled) ||
        (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_GPIO,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to access the GPIO registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_GPIO,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* Need a master plugin context to do I2C calls */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            /* This should *never* happen */
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_GPIO,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            wBuf[0] = A2B_REG_GPIOOEN;
            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWriteRead(mCtx,
                		(a2b_UInt16)A2B_ARRAY_SIZE(wBuf),
                                                &wBuf[0], (a2b_UInt16)A2B_ARRAY_SIZE(rBuf),
                                                &rBuf[0]);
            }
            else
            {
                result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr,
                		(a2b_UInt16)A2B_ARRAY_SIZE(wBuf),
                                                &wBuf[0], (a2b_UInt16)A2B_ARRAY_SIZE(rBuf),
                                                &rBuf[0]);
            }

            if ( A2B_SUCCEEDED(result) )
            {
                *enabled = (rBuf[0] & (a2b_UInt8)pin) ? A2B_TRUE : A2B_FALSE;
            }
        }
    }

    return result;

} /* a2b_gpioOutIsEnabled */


/*!****************************************************************************
* 
*  \b   a2b_gpioOutSetEnabled
* 
*  This function enables/disables the specified GPIO output pins. If an output
*  pin is enabled then it will drive the specified output action on the pin
*  (e.g. high or low depending on the configuration). The enableMask controls
*  whether the corresponding pin specified in the pinMask will be enabled (1)
*  or disabled (0). Operation is undefined if the same GPIO pin is enabled
*  for both input and output. Slave node GPIO related actions can only be
*  executed after A2B discovery and configuration has occurred. Slave
*  plugins <b>cannot</b> access GPIO pins that belong to another node. A master
*  plugin or application, however, can access GPIO pins for any slave node.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   GPIO request.
* 
*  \param   [in]        nodeAddr    The address of the A2B node to access.
* 
*  \param   [in]        pinMask     The GPIO pin mask indicating which pins
*                                   are being configured. The mask is created
*                                   by <b>OR</b>'ing the a2b_GpioPin
*                                   enumeration values.
* 
*  \param   [in,out]    enableMask  Either enables (1) or disables (0) the
*                                   corresponding pin specified by the pinMask.
* 
*  \pre     A2B discovery has already been conducted successfully if accessing
*           a slave node. It is recommended to avoid enabled the same GPIO pin
*           for both input and output.
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_gpioOutSetEnabled
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt8                   pinMask,
    a2b_UInt8                   enableMask
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[2];
    a2b_Byte rBuf[1];
    a2b_UInt8 bitPos;
    struct a2b_StackContext* mCtx;

    /* Make sure the node address is valid */
    if ( (A2B_NULL == ctx) || (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_GPIO,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to access the GPIO registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_GPIO,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* Need a master plugin context to do I2C calls */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            /* This should *never* happen */
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_GPIO,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            /* Read the current register settings */
            wBuf[0] = A2B_REG_GPIOOEN;
            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWriteRead(mCtx, 1u, &wBuf[0],
                                            (a2b_UInt16)A2B_ARRAY_SIZE(rBuf), &rBuf[0]);
            }
            else
            {
                result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr, 1u,
                                                &wBuf[0], (a2b_UInt16)A2B_ARRAY_SIZE(rBuf),
                                                &rBuf[0]);
            }

            if ( A2B_SUCCEEDED(result) )
            {
                /* Initialize with the current pin configuration */
                wBuf[1] = rBuf[0];

                for ( bitPos = (a2b_UInt8)A2B_GPIO_PIN_0; bitPos & (a2b_UInt8)A2B_GPIO_ALL_PINS_MASK;
                      bitPos <<= 1u )
                {
                    /* If this is a pin that is being modified then ... */
                    if ( pinMask & bitPos )
                    {
                        /* If we're enabling this pin then ... */
                        if ( enableMask & bitPos )
                        {
                            wBuf[1] |= bitPos;
                        }
                        /* Else disabling the pin (clear the bit) */
                        else
                        {
                            wBuf[1] &= (a2b_Byte)~(bitPos);
                        }
                    }
                }

                if ( A2B_NODEADDR_MASTER == nodeAddr )
                {
                    result = a2b_i2cMasterWrite(mCtx, 2u, &wBuf[0]);
                }
                else
                {
                    result = a2b_i2cSlaveWrite(mCtx, nodeAddr, 2u, &wBuf[0]);
                }
            }
        }
    }

    return result;

} /* a2b_gpioOutSetEnabled */


/*!****************************************************************************
* 
*  \b   a2b_gpioInGetLevels
* 
*  This function returns the logic level of the GPIO input pins indicated in
*  the pin mask. The pin mask is created by OR'ing together the a2b_GpioPin
*  values for the pins that should be read. Only the pins asserted in the
*  returned 'enabledMask' (an "OR" of a2b_GpioPin values) are configured and
*  enabled for input (and thus have valid readings). Pins in the 'enabledMask'
*  not asserted are either configured for output or are in a HIGH-Z (high
*  impedance) state. Slave node GPIO related actions can only be executed after
*  A2B discovery and configuration has occurred. Slave plugins <b>cannot</b>
*  access GPIO pins that belong to another node. A master plugin or
*  application, however, can access GPIO pins for any slave node.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   GPIO request.
* 
*  \param   [in]        nodeAddr    The address of the A2B node to access.
* 
*  \param   [in]        pinMask     A logical <b>OR</b> of a2b_GpioPin
*                                   enumerations indicating the pins levels
*                                   to read.
* 
*  \param   [in,out]    levelMask   The mask containing the logic level of the
*                                   GPIO input pins read by the chip.
*                                   Only those bit positions asserted
*                                   in the 'enabledMask' are considered valid
*                                   input readings.
* 
*  \param   [in,out]    enabledMask The masking indicating which GPIO pins
*                                   are configured and enabled for input.
*                                   The mask is created by <b>OR</b>'ing
*                                   together a2b_GpioPin enumerated values.
*                                   If a bit position is <b>not</b> asserted
*                                   then the corresponding position and value
*                                   in the returned 'levelMask' is invalid and
*                                   does not provide an accurate reading.
* 
*  \pre     A2B discovery has already been conducted successfully if accessing
*           a slave node. It is *assumed* the GPIO pins <b>OR</b>'ed in the
*           'levelMask' have been configured as input pin(s).
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_gpioInGetLevels
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt8                   pinMask,
    a2b_UInt8*                  levelMask,
    a2b_UInt8*                  enabledMask
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[1];
    a2b_Byte rBuf[2];
    struct a2b_StackContext* mCtx;

    /* Make sure the node address is valid */
    if ( (A2B_NULL == ctx) || (A2B_NULL == levelMask) ||
        (A2B_NULL == enabledMask) || (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_GPIO,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to access the GPIO registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_GPIO,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* Need a master plugin context to do I2C calls */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            /* This should *never* happen */
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_GPIO,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            /* A2B_REG_GPIOIEN is the smallest (base) register of the two
             * registers we're going to read. We can subtract that off any
             * successive register to determine it's value in the read buffer.
             */
            wBuf[0] = A2B_REG_GPIOIEN;
            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWriteRead(mCtx,
                		(a2b_UInt16)A2B_ARRAY_SIZE(wBuf), &wBuf[0],
						(a2b_UInt16)A2B_ARRAY_SIZE(rBuf), &rBuf[0]);
            }
            else
            {
                result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr,
                		(a2b_UInt16)A2B_ARRAY_SIZE(wBuf), &wBuf[0],
						(a2b_UInt16)A2B_ARRAY_SIZE(rBuf), &rBuf[0]);
            }


            if ( A2B_SUCCEEDED(result) )
            {
                *enabledMask = rBuf[0];
                *levelMask = pinMask & *enabledMask &
                                rBuf[A2B_REG_GPIOIN - A2B_REG_GPIOIEN];
            }
        }
    }

    return result;

} /* a2b_gpioInGetLevels */


/*!****************************************************************************
* 
*  \b   a2b_gpioInIsEnabled
* 
*  This function returns A2B_TRUE if the specified GPIO input pin is enabled
*  or A2B_FALSE otherwise. Slave node GPIO related actions can only be executed
*  after A2B discovery and configuration has occurred. Slave plugins
*  <b>cannot</b> access GPIO pins that belong to another node. A master plugin
*  or application, however, can access GPIO pins for any slave node.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   GPIO request.
* 
*  \param   [in]        nodeAddr    The address of the A2B node to access.
* 
*  \param   [in]        pin         The GPIO pin to access.
* 
*  \param   [in,out]    enabled     Set to A2B_TRUE if the GPIO input pin
*                                   is enabled or A2B_FALSE otherwise.
* 
*  \pre     A2B discovery has already been conducted successfully if accessing
*           a slave node. It is *assumed* the GPIO pin has been configured as
*           an input pin during discovery.
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_gpioInIsEnabled
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_GpioPin                 pin,
    a2b_Bool*                   enabled
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[1];
    a2b_Byte rBuf[1];
    struct a2b_StackContext* mCtx;

    /* Make sure the node address is valid */
    if ( (A2B_NULL == ctx) || (A2B_NULL == enabled) ||
        (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_GPIO,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to access the GPIO registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_GPIO,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* Need a master plugin context to do I2C calls */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            /* This should *never* happen */
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_GPIO,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            wBuf[0] = A2B_REG_GPIOIEN;
            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWriteRead(mCtx,
                		(a2b_UInt16)A2B_ARRAY_SIZE(wBuf), &wBuf[0],
						(a2b_UInt16)A2B_ARRAY_SIZE(rBuf), &rBuf[0]);
            }
            else
            {
                result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr,
                		(a2b_UInt16)A2B_ARRAY_SIZE(wBuf), &wBuf[0],
						(a2b_UInt16)A2B_ARRAY_SIZE(rBuf), &rBuf[0]);
            }

            if ( A2B_SUCCEEDED(result) )
            {
                *enabled = (rBuf[0] & (a2b_UInt8)pin) ? A2B_TRUE : A2B_FALSE;
            }
        }
    }

    return result;

} /* a2b_gpioInIsEnabled */


/*!****************************************************************************
* 
*  \b   a2b_gpioInSetEnabled
* 
*  This function enables/disables the specified GPIO input pins. The enableMask
*  controls whether the corresponding pin specified in the pinMask will be
*  enabled (1) or disabled (0). Operation is undefined if the same GPIO pin is
*  enabled for both input and output. Slave node GPIO related actions can only
*  be executed after A2B discovery and configuration has occurred. Slave
*  plugins <b>cannot</b> access GPIO pins that belong to another node. A master
*  plugin or application, however, can access GPIO pins for any slave node.
* 
*  \param   [in]        ctx         The stack context associated with the
*                                   GPIO request.
* 
*  \param   [in]        nodeAddr    The address of the A2B node to access.
* 
*  \param   [in]        pinMask     The GPIO pin mask indicating which pins
*                                   are being configured. The mask is created
*                                   by <b>OR</b>'ing the a2b_GpioPin
*                                   enumeration values.
* 
*  \param   [in,out]    enableMask  Either enables (1) or disables (0) the
*                                   corresponding pin specified by the pinMask.
* 
*  \pre     A2B discovery has already been conducted successfully if accessing
*           a slave node. It is recommended to avoid enabled the same GPIO pin
*           for both input and output.
* 
*  \post    None
* 
*  \return  A status code that can be checked with the #A2B_SUCCEEDED() or
*           #A2B_FAILED() for success or failure of the request.
* 
******************************************************************************/
A2B_DSO_PUBLIC a2b_HResult
a2b_gpioInSetEnabled
    (
    struct a2b_StackContext*    ctx,
    a2b_Int16                   nodeAddr,
    a2b_UInt8                   pinMask,
    a2b_UInt8                   enableMask
    )
{
    a2b_HResult result = A2B_RESULT_SUCCESS;
    a2b_Byte wBuf[2];
    a2b_Byte rBuf[1];
    a2b_UInt8 bitPos;
    struct a2b_StackContext* mCtx;

    /* Make sure the node address is valid */
    if ( (A2B_NULL == ctx) || (!A2B_VALID_NODEADDR(nodeAddr)) )
    {
        result = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_GPIO,
                                  A2B_EC_INVALID_PARAMETER);
    }
    /* Only applications, the master plugin, or a slave plugin with
     * the same node address is allowed to access the GPIO registers.
     */
    else if ( (ctx->domain == A2B_DOMAIN_PLUGIN) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != A2B_NODEADDR_MASTER) &&
            (ctx->ccb.plugin.nodeSig.nodeAddr != nodeAddr) )
    {
        result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                    A2B_FAC_GPIO,
                                    A2B_EC_PERMISSION);
    }
    else
    {
        /* Need a master plugin context to do I2C calls */
        mCtx = a2b_stackContextFind(ctx, A2B_NODEADDR_MASTER);
        if ( A2B_NULL == mCtx )
        {
            /* This should *never* happen */
            result =  A2B_MAKE_HRESULT(A2B_SEV_FAILURE,
                                        A2B_FAC_GPIO,
                                        A2B_EC_INTERNAL);
        }
        else
        {
            /* Read the current register settings */
            wBuf[0] = A2B_REG_GPIOIEN;
            if ( A2B_NODEADDR_MASTER == nodeAddr )
            {
                result = a2b_i2cMasterWriteRead(mCtx, 1u, &wBuf[0],
                		(a2b_UInt16)A2B_ARRAY_SIZE(rBuf), &rBuf[0]);
            }
            else
            {
                result = a2b_i2cSlaveWriteRead(mCtx, nodeAddr, 1u,
                                            &wBuf[0], (a2b_UInt16)A2B_ARRAY_SIZE(rBuf),
                                            &rBuf[0]);
            }

            if ( A2B_SUCCEEDED(result) )
            {
                /* Initialize with the current pin configuration */
                wBuf[1] = rBuf[0];

                for ( bitPos = (a2b_UInt8)A2B_GPIO_PIN_0; bitPos & A2B_GPIO_ALL_PINS_MASK;
                      bitPos <<= 1u )
                {
                    /* If this is a pin that is being modified then ... */
                    if ( pinMask & bitPos )
                    {
                        /* If we're enabling this pin then ... */
                        if ( enableMask & bitPos )
                        {
                            wBuf[1] |= bitPos;
                        }
                        /* Else disabling the pin (clear the bit) */
                        else
                        {
                            wBuf[1] &= (a2b_Byte)~(bitPos);  /* TODO - chk */
                        }
                    }
                }

                if ( A2B_NODEADDR_MASTER == nodeAddr )
                {
                    result = a2b_i2cMasterWrite(mCtx, 2u, &wBuf[0]);
                }
                else
                {
                    result = a2b_i2cSlaveWrite(mCtx, nodeAddr, 2u, &wBuf[0]);
                }
            }
        }
    }

    return result;

} /* a2b_gpioInSetEnabled */
