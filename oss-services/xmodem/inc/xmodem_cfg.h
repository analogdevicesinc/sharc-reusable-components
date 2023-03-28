/*!
 * @brief  Example configuration file enabling the umm_malloc heap
 *         allocation module.
 *
 * @file      xmodem_cfg.h
 * @version   1.0.0
 *
*/

#ifndef _XMODEM_CFG_H
#define _XMODEM_CFG_H

/*
 * To use umm_malloc instead of the system heap functions,
 * ensure umm_malloc.h is in the project's include path and
 * set:
 *
 *  #include "umm_malloc.h"
 *
 *  #define XMODEM_MALLOC  umm_malloc
 *  #define XMODEM_FREE    umm_free
 *
 */

/*! @cond */

#define XMODEM_MALLOC            malloc
#define XMODEM_FREE              free

/*! @endcond example-config */

#endif
