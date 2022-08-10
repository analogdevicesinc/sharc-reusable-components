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
 * \file:   i2c.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the I2C interface used by A2B stack application and
 *          plugins.
 *
 *=============================================================================
 */

/*============================================================================*/
/**
 * \defgroup a2bstack_i2c       I2C Module
 *  
 * The types and associated APIs providing public low-level I2C access.
 *  
 * The StackContext provides a mechanism for enforcing access control.
 * This means that only a master plugin can access the master, slave, or
 * peripheral I2C read/write routines. A slave plugin can only read/write to
 * peripherals attached to its node. An "application" context can also
 * read/write to peripherals attached via I2C to <em>any</em> slave node. 
 *  
 * \{ */
/*============================================================================*/

#ifndef A2B_I2C_H_
#define A2B_I2C_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_i2c_defs          Types/Defs
 *  
 * The various defines and data types used within the I2C modules.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_StackContext;

/**
 * Supported I2C address formats
 */
typedef enum
{
    /** 7-Bit I2C address */
    A2B_I2C_ADDR_FMT_7BIT,

    /** 10-Bit I2C address */
    A2B_I2C_ADDR_FMT_10BIT

} a2b_I2cAddrFmt;


/**
 * Supported I2C bus speeds
 */
typedef enum
{
    /** Standard mode */
    A2B_I2C_BUS_SPEED_100KHZ,

    /** Fast mode */
    A2B_I2C_BUS_SPEED_400KHZ

} a2b_I2cBusSpeed;

/** \} -- a2bstack_i2c_defs */

/*===========================================================================
 *
 * Define the *synchronous* I2C API
 *
 * ========================================================================*/

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_i2c_master        Master Node I2C Synchronous Access 
 *  
 * I2C API to synchronously read/write to/from the A2B master node.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cMasterRead(
                                            struct a2b_StackContext* ctx,
                                            a2b_UInt16 nRead,
                                            void* rBuf);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cMasterWrite(
                                            struct a2b_StackContext* ctx,
                                            a2b_UInt16 nWrite,
                                            void* wBuf);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cMasterWriteRead(
                                            struct a2b_StackContext* ctx,
                                            a2b_UInt16 nWrite,
                                            void* wBuf,
                                            a2b_UInt16 nRead,
                                            void* rBuf);
/** \} -- a2bstack_i2c_master */

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_i2c_slave         Slave Node I2C Synchronous Access 
 *  
 * I2C API to synchronously read/write to/from the A2B slave node.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cSlaveRead(
                                            struct a2b_StackContext* ctx,
                                            a2b_Int16 node,
                                            a2b_UInt16 nRead,
                                            void* rBuf);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cSlaveWrite(
                                            struct a2b_StackContext* ctx,
                                            a2b_Int16 node,
                                            a2b_UInt16 nWrite,
                                            void* wBuf);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cSlaveBroadcastWrite(
                                            struct a2b_StackContext* ctx,
                                            a2b_UInt16 nWrite,
                                            void* wBuf);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cSlaveWriteRead(
                                            struct a2b_StackContext* ctx,
                                            a2b_Int16 node,
                                            a2b_UInt16 nWrite,
                                            void* wBuf,
                                            a2b_UInt16 nRead,
                                            void* rBuf);
/** \} -- a2bstack_i2c_slave */

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_i2c_periph        Peripheral Device Synchronous I2C Access 
 *  
 * I2C API to synchronously read/write to/from the A2B node peripheral.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cPeriphRead(
                                            struct a2b_StackContext* ctx,
                                            a2b_Int16 node,
                                            a2b_UInt16 i2cAddr,
                                            a2b_UInt16 nRead,
                                            void* rBuf);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cPeriphWrite(
                                            struct a2b_StackContext* ctx,
                                            a2b_Int16 node,
                                            a2b_UInt16 i2cAddr,
                                            a2b_UInt16 nWrite,
                                            void* wBuf);

A2B_DSO_PUBLIC a2b_HResult A2B_CALL a2b_i2cPeriphWriteRead(
                                            struct a2b_StackContext* ctx,
                                            a2b_Int16 node,
                                            a2b_UInt16 i2cAddr,
                                            a2b_UInt16 nWrite,
                                            void* wBuf,
                                            a2b_UInt16 nRead,
                                            void* rBuf);
/** \} -- a2bstack_i2c_periph */

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_i2c */

#endif /* A2B_I2C_H_ */
