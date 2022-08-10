/* This is an example of a header file for platforms/compilers that do
 * not come with stdint.h/stddef.h/stdbool.h/string.h. To use it, define
 * PB_SYSTEM_HEADER as "pb_syshdr.h", including the quotes, and add the
 * extra folder to your include path.
 *
 * It is very likely that you will need to customize this file to suit
 * your platform. For any compiler that supports C99, this file should
 * not be necessary.
 */

#ifndef _PB_SYSHDR_H_
#define _PB_SYSHDR_H_

#if 1

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#else

#include "a2b/ctypes.h"
#include "a2b/features.h"
#include "a2b/util.h"

#ifdef __ADSP21000__
#include <stddef.h>
#endif

#ifndef _STDINT
/* You will need to modify these to match the word size of your platform. */
typedef a2b_Int8 int8_t;
typedef a2b_UInt8 uint8_t;
typedef a2b_Int16 int16_t;
typedef a2b_UInt16 uint16_t;
typedef a2b_Int32 int32_t;
typedef a2b_UInt32 uint32_t;

#ifndef _STDDEF_H
#ifdef A2B_FEATURE_64_BIT_INTEGER
/* 64-bit quantities should only be needed if the protobufs definitions use
 * them. For A2B this is not the case.
 */
typedef a2b_Int64 int64_t;
typedef a2b_UInt64 uint64_t;
#endif
#endif
#endif

/* stddef.h/stdio.h MUST not be used in any C source for a deliverable.
 * However, when debugging with a printf you will need stdio.h. 
 * However, only defining stdio.h will result in a compiler error 
 * with size_t conflicts.  To resolve the compiler error you need to 
 * declare the following to use printf statements: 
 * #include <stddef.h>
 * #include <stdio.h> 
 * ... 
 * printf("something\n"); 
 */

/* Ifdef stddef.h checks for Linux, QNX, ADI, and Microsoft environments */
#if !defined(_STDDEF_H) && !defined(_STDDEF_H_INCLUDED) && \
    !defined(__STDDEF_DEFINED) && !defined(_MSC_VER)
    #ifdef A2B_FEATURE_64_BIT_INTEGER
    #define PB_SUPPORT_64_BIT_NUMBERS
    typedef uint64_t size_t;
    #else
    typedef uint32_t size_t;
    #endif

    #define offsetof(st, m) ((a2b_UIntPtr)(&((st *)0)->m))
#endif

#ifndef NULL
#define NULL A2B_NULL
#endif


/* The Analog Devices compiler has extensions to support bool types */
#ifndef __ANALOG_EXTENSIONS__
typedef a2b_Bool bool;
#define false A2B_FALSE
#define true A2B_TRUE
#endif

#if !defined _ADI_COMPILER
#if !defined(_STRING_H) && !defined(_MSC_VER)  && !defined(_STRING)
#define strlen a2b_strlen
#define memcpy a2b_memcpy
#define memset a2b_memset
#endif
#endif
#endif

#endif
