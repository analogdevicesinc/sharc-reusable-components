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
 * \file:   macros.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Definition of common globals macros.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_macros        Global Macro Definitions
 *  
 * Definition of common globals macros.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_MACROS_H_
#define A2B_MACROS_H_

/*======================= I N C L U D E S =========================*/

/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_macro_defs    Types/Defs
 *  
 * The various defines and data types used globally.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*======================= D E F I N E S ===========================*/

#if defined(__STDC__)
#   define STD_C_C89
#   if defined(__STDC_VERSION__)
#       define STD_C_C90
#   endif
#   if (__STDC_VERSION__ >= 199409L)
#       define STD_C_C94
#   endif
#   if (__STDC_VERSION__ >= 199901L)
#       define STD_C_C99
#   endif
#   if (__STDC_VERSION__ >= 201112L)
#       define STD_C_C11
#   endif
#endif

#ifdef __cplusplus
#   define A2B_BEGIN_DECLS extern "C" {
#   define A2B_END_DECLS }
#else
#   define A2B_BEGIN_DECLS
#   define A2B_END_DECLS
#endif


#define A2B_EXPORT extern

#if defined _WIN32 || defined __CYGWIN__ || defined __MINGW64__ || defined __MINGW32__
#   ifdef A2B_BUILD_DSO
#       ifdef __GNUC__
#           define A2B_DSO_PUBLIC __attribute__ ((dllexport)) A2B_EXPORT
#       else
#           define A2B_DSO_PUBLIC __declspec(dllexport) A2B_EXPORT
#       endif
#   else
#       if defined A2B_LINK_STATIC
#           define A2B_DSO_PUBLIC A2B_EXPORT
#       else
#           ifdef __GNUC__
#               define A2B_DSO_PUBLIC __attribute__ ((dllimport)) A2B_EXPORT
#           else
#               define A2B_DSO_PUBLIC __declspec(dllimport) A2B_EXPORT
#           endif
#       endif
#   endif
#   ifdef __GNUC__
#       define A2B_CALL __attribute__ ((__cdecl__))
#   else
#       define A2B_CALL __cdecl
#   endif
#   define A2B_DSO_LOCAL
#else
#   if defined __GNUC__
#       if __GNUC__ >= 4
#           define A2B_DSO_PUBLIC __attribute__ ((visibility ("default"))) A2B_EXPORT
#           define A2B_DSO_LOCAL  __attribute__ ((visibility ("hidden")))
#       else
#           define A2B_DSO_PUBLIC
#           define A2B_DSO_LOCAL
#       endif
#   else
#       define A2B_DSO_PUBLIC
#       define A2B_DSO_LOCAL
#   endif
#   define A2B_CALL
#endif

#define A2B_XSTR(s)   A2B_STR(s)
#define A2B_STR(s)    #s

#ifndef _TESSY_NO_DOWHILE_MACROS_
#define A2B_UNUSED(X) do{ \
	if(X) \
    {	\
	}	\
}while(0)
#else    /* _TESSY_NO_DOWHILE_MACROS_ */
#define A2B_UNUSED(X)
#endif   /* _TESSY_NO_DOWHILE_MACROS_ */

#define A2B_ARRAY_SIZE(X)   (sizeof(X) / sizeof(X[0]))

#define A2B_ASSERT_CONCAT_(a, b) a##b
#define A2B_ASSERT_CONCAT(a, b) A2B_ASSERT_CONCAT_(a, b)
#define A2B_STATIC_ASSERT(expr, msg) \
    typedef char A2B_ASSERT_CONCAT(static_assert_on_line_,__LINE__)[(expr) ? 1 : -1]


#define A2B_MIN(a,b)    (((a) < (b)) ? (a) : (b))
#define A2B_MAX(a,b)    (((a) > (b)) ? (a) : (b))


/*======================= D A T A T Y P E S =======================*/

/** \} -- a2bstack_macros_defs */

/*======================= P U B L I C  P R O T O T Y P E S ========*/

/*======================= D A T A =================================*/

/** \} -- a2bstack_macros */

#endif /* A2B_MACROS_H_ */
