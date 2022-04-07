/*!
 * @brief  Example configuration file enabling the umm_malloc heap
 *         allocation module.
 *
 * @file      shell_cfg.h
 * @version   1.0.0
 * @sa        shell.h
 *
*/

#ifndef _SHELL_CFG_H
#define _SHELL_CFG_H

/*
 * To use umm_malloc instead of the system heap functions,
 * ensure umm_malloc.h is in the project's include path and
 * set:
 *
 *  #include "umm_malloc.h"
 *
 *  #define SHELL_MALLOC  umm_malloc
 *  #define SHELL_FREE    umm_free
 *
 */

/*! @cond */

#define SHELL_MAX_ARGS          10
#define SHELL_WELCOMEMSG        "Shell %s\n"
#define SHELL_PROMPT            "# "
#define SHELL_ERRMSG            "Invalid command, type 'help' for help\n"
#define SHELL_MAX_LINE_LEN      79
#define SHELL_COLUMNS           80
#define SHELL_LINES             25
#define SHELL_MAX_HISTORIES     50
#define SHELL_MALLOC            malloc
#define SHELL_FREE              free
#define SHELL_STRDUP            shell_strdup
#define SHELL_STRNDUP           shell_strndup

/*! @endcond example-config */

#endif
