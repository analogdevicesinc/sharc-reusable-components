/*
 * This file was borrowed from the eLua project:
 *
 * https://github.com/elua/elua/blob/master/LICENSE
 *
 * See NOTICE for the full MIT license.
 *
 */

// Type definitions for desktop platform
#ifndef __TYPE_H__
#define __TYPE_H__

#include <stdint.h>

#define uint8_t unsigned char
#define int8_t char
#define uint16_t unsigned short
#define int16_t short

// signed and unsigned 8, 16 and 32 bit types
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

typedef u32 timer_data_type;

#endif
