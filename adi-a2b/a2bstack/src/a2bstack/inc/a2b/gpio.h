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
 * \file:   gpio.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The definitions and interfaces supporting A2B stack GPIO access.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_gpio              GPIO Module
 *  
 * The types and associated public APIs providing access to slave node
 * GPIO pins.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_GPIO_H_
#define A2B_GPIO_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_gpio_defs         Types/Defs
 *  
 * The various defines and data types used within the GPIO module.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

/** Define the GPIO high-impedance (Z) value */
#define A2B_GPIO_HIGH_Z     (-1)

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward Declarations */
struct a2b_StackContext;

/**
 * A2B Slave Node GPIO Pin Enumerations
 * These values can be <b>OR</b>'ed together
 * to form a pin mask.
 */
typedef enum
{
	/*!  Enum for GPIO Pin 0  */
    A2B_GPIO_PIN_0 = (1u << 0u),
	/*!  Enum for GPIO Pin 1  */
    A2B_GPIO_PIN_1 = (1u << 1u),
	/*!  Enum for GPIO Pin 2  */
    A2B_GPIO_PIN_2 = (1u << 2u),
	/*!  Enum for GPIO Pin 3  */
    A2B_GPIO_PIN_3 = (1u << 3u),
	/*!  Enum for GPIO Pin 4  */
    A2B_GPIO_PIN_4 = (1u << 4u),
	/*!  Enum for GPIO Pin 5  */
    A2B_GPIO_PIN_5 = (1u << 5u),
	/*!  Enum for GPIO Pin 6  */
    A2B_GPIO_PIN_6 = (1u << 6u),
	/*!  Enum for GPIO Pin 7  */
    A2B_GPIO_PIN_7 = (1u << 7u)
} a2b_GpioPin;

/*!  Macro definition for selecting all pins  */
#define A2B_GPIO_ALL_PINS_MASK  ((a2b_UInt8)(A2B_GPIO_PIN_0) | \
                                (a2b_UInt8)(A2B_GPIO_PIN_1) | \
                                (a2b_UInt8)(A2B_GPIO_PIN_2) | \
                                (a2b_UInt8)(A2B_GPIO_PIN_3) | \
                                (a2b_UInt8)(A2B_GPIO_PIN_4) | \
                                (a2b_UInt8)(A2B_GPIO_PIN_5) | \
                                (a2b_UInt8)(A2B_GPIO_PIN_6) | \
								(a2b_UInt8)(A2B_GPIO_PIN_7) )

/** \} -- a2bstack_gpio_defs */

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_gpio_out      Output Functions
 *  
 * These functions are only output related GPIO functions.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_gpioOutSetAction(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_UInt8 pinMask,
                                                a2b_UInt8 actionMask);
A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_gpioOutGetLevels(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_UInt8 pinMask,
                                                a2b_UInt8* levelMask,
                                                a2b_UInt8* enabledMask);
A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_gpioOutIsEnabled(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_GpioPin pin,
                                                a2b_Bool* enabled);
A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_gpioOutSetEnabled(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_UInt8 pinMask,
                                                a2b_UInt8 enableMask);
/** \} -- a2bstack_gpio_out */

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_gpio_in       Input Functions
 *  
 * These functions are only input related GPIO functions.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_gpioInGetLevels(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_UInt8 pinMask,
                                                a2b_UInt8* levelMask,
                                                a2b_UInt8* enabledMask);
A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_gpioInIsEnabled(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_GpioPin pin,
                                                a2b_Bool* enabled);
A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_gpioInSetEnabled(
                                                struct a2b_StackContext* ctx,
                                                a2b_Int16 nodeAddr,
                                                a2b_UInt8 pinMask,
                                                a2b_UInt8 enableMask);
/** \} -- a2bstack_gpio_in */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_gpio */

#endif /* A2B_GPIO_H_ */

