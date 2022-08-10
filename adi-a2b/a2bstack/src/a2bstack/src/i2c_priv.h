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
 * \file:   i2c_priv.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  These are private definitions for the A2B public I2C API.
 *
 *=============================================================================
 */

#ifndef A2B_I2C_PRIV_H_
#define A2B_I2C_PRIV_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "a2b/conf.h"
#include "a2b/i2c.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/*----------------------------------------------------------------------------*/
/** 
 * \ingroup         a2bstack_i2c_priv 
 *  
 * The fundamental I2C commands supported by the A2B stack.
 */
/*----------------------------------------------------------------------------*/
typedef enum
{
    /** \{
     *  Allowed for the *master* plugin only */
    A2B_I2C_CMD_READ_MASTER,
    A2B_I2C_CMD_WRITE_MASTER,
    A2B_I2C_CMD_WRITE_READ_MASTER,
    A2B_I2C_CMD_READ_SLAVE,
    A2B_I2C_CMD_WRITE_SLAVE,
    A2B_I2C_CMD_WRITE_BROADCAST_SLAVE,
    A2B_I2C_CMD_WRITE_READ_SLAVE,
    /** \} */

    /** \{
     *  Allowed for the master/slave plugins plus application context */
    A2B_I2C_CMD_READ_PERIPH,
    A2B_I2C_CMD_WRITE_PERIPH,
    A2B_I2C_CMD_WRITE_READ_PERIPH
    /** \} */

} a2b_I2cCmd;

A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_I2C_PRIV_H_ */
