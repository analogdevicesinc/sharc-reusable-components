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
 * \file:   error.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Defines error codes and macros for the A2B stack.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_error             Error Defintions
 *  
 * Error code and macro definitions for the A2B stack.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_ERROR_H_
#define A2B_ERROR_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/ctypes.h"

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

typedef enum {
    /* A2B_SEV_SUCCESS = 0 -- not defined to avoid misuse */
    A2B_SEV_FAILURE = (a2b_UInt32)1u
} a2b_Severity;

typedef enum {
    A2B_FAC_NONE            = (a2b_UInt32)0u,
    A2B_FAC_STACK           = (a2b_UInt32)1u,
    A2B_FAC_PLUGIN          = (a2b_UInt32)2u,
    A2B_FAC_I2C             = (a2b_UInt32)3u,
    A2B_FAC_TIMER           = (a2b_UInt32)4u,
    A2B_FAC_LOG             = (a2b_UInt32)5u,
    A2B_FAC_AUDIO_CONFIG    = (a2b_UInt32)6u,
    A2B_FAC_PLATFORM        = (a2b_UInt32)7u,
    A2B_FAC_MEM_MGR         = (a2b_UInt32)8u,
    A2B_FAC_MSGRTR          = (a2b_UInt32)9u,
    A2B_FAC_INTERRUPT       = (a2b_UInt32)10u,
    A2B_FAC_GPIO            = (a2b_UInt32)11u,
    A2B_FAC_DIAG            = (a2b_UInt32)12u
} a2b_Facility;

typedef enum {
    /** No error - operation a success */
    A2B_EC_OK = (a2b_UInt32)0u,

    /** Invalid parameter or bad argument */
    A2B_EC_INVALID_PARAMETER,

    /** Resource could not be allocated */
    A2B_EC_ALLOC_FAILURE,

    /** Failure decoding network configuration */
    A2B_EC_NETWORK_DECODE_FAILURE,

    /** Invalid state for request */
    A2B_EC_INVALID_STATE,

    /** Resource does not exist */
    A2B_EC_DOES_NOT_EXIST,

    /** Transaction timed out */
    A2B_EC_BUSY,

    /** Permissions error */
    A2B_EC_PERMISSION,

    /** I2C slave address NACK error */
    A2B_EC_I2C_ADDR_NACK,

    /** I2C slave data NACK error */
    A2B_EC_I2C_DATA_NACK,

    /** I2C arbitration lost error */
    A2B_EC_I2C_ARBL,

    /**
     * I2C bus locked. Likely due to clock line of bus being held low by some
     * other device.
     */
    A2B_EC_I2C_BUS_LOCKED,

    /** Some form of I/O error */
    A2B_EC_IO,

    /** Catch-all error condition */
    A2B_EC_INTERNAL,

    /** Resource currently unavailable */
    A2B_EC_RESOURCE_UNAVAIL,

    /** A2B power diagnostics failure */
    A2B_EC_POWER_DIAG_FAILURE,

    /** General A2B discovery failure */
    A2B_EC_DISCOVERY_FAILURE,

    /** A2B discovery failure due to power fault error */
    A2B_EC_DISCOVERY_PWR_FAULT,

    /** An operation was cancelled */
    A2B_EC_CANCELLED,

	/** Custom Node Id Authentication failure */
    A2B_EC_CUSTOM_NODE_ID_AUTH,

	/** Custom Node Id Authentication Timeout
	 * when using communication channel
	 * */
	A2B_EC_CUSTOM_NODE_ID_TIMEOUT

} a2b_StackErrorCode;

A2B_END_DECLS

/*======================= D E F I N E S ===========================*/

#define A2B_MAKE_HRESULT(SEV, FAC, CODE) \
                        ((a2b_HResult)( \
                            (((a2b_UInt32)((a2b_UInt32)(SEV) & 0x1U)) << (a2b_UInt32)31u) | \
                            (((a2b_UInt32)((a2b_UInt32)(1U) & 0x1U)) << (a2b_UInt32)29u) | \
                            (((a2b_UInt32)((a2b_UInt32)(FAC) & 0x7FFU)) << (a2b_UInt32)16u) | \
                            (((a2b_UInt32)(CODE) & 0xFFFFU)) \
                        ))

#define A2B_FAILED(R)         (R != 0u)
#define A2B_SUCCEEDED(R)      (R == 0u)
#define A2B_FACILITY(R)       (((a2b_HResult)(R) >> (a2b_UInt32)16U) & (a2b_UInt32)0x7FFU)
#define A2B_ERR_CODE(R)       ((a2b_HResult)(R) & (a2b_UInt32)0xFFFFU)
#define A2B_SEVERITY(R)       (((a2b_HResult)(R) >> (a2b_UInt32)31U) & (a2b_UInt32)0x1U)
#define A2B_RESULT_SUCCESS    (0u)

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*======================= D A T A =================================*/

/** \} -- a2bstack_error */

#endif /* A2B_ERROR_H_ */
