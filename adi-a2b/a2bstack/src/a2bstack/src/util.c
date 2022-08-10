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
 * \file:   util.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  Implementation of a collection of system utility functions.
 *
 *=============================================================================
 */

/*
 * Portions of the a2b_vsnprintf() related code contained herein was adapted
 * from work done by Kusta Nyholm / SpareTimeLabs. The copyright associated
 * with this code is included below:
 */

/*
 * Copyright (c) 2004,2012 Kustaa Nyholm / SpareTimeLabs
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * Neither the name of the Kustaa Nyholm or SpareTimeLabs nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

/*============================================================================*/
/** 
 * \ingroup  a2bstack_util 
 * \defgroup a2bstack_util_priv             \<Private\> 
 * \private 
 *  
 * This defines the utility API's that are private to the stack. 
 */
/*============================================================================*/

/*======================= I N C L U D E S =========================*/
#include "a2b/util.h"
#include "a2b/stringbuffer.h"

/*======================= D E F I N E S ===========================*/

/** 
 *  Maximum number of characters needed to hold a 32-bit integral
 *  value when converted to a textual (string) representation including
 *  a terminating null. This largest representation would be:
 *      -2147483647 = (11 characters + null = 12)  (32-bit)
 *      -9223372036854775808 (20 characters + null = 21) (64-bit)
 */
#ifdef A2B_FEATURE_64_BIT_INTEGER
#define A2B_MAX_FMT_CONV_BUF_SIZE   (21u * sizeof(a2b_Char))
#else
#define A2B_MAX_FMT_CONV_BUF_SIZE   (12u * sizeof(a2b_Char))
#endif

/** 
 * \{ 
 *  Flags used to indicate the size of the integral values passed
 *  as arguments to the a2b_vsnprintf function.
 */
#define A2B_8BIT_INTEGER    ((a2b_UInt32)1u << (a2b_UInt32)0u)
#define A2B_16BIT_INTEGER   ((a2b_UInt32)1u << (a2b_UInt32)1u)
#define A2B_32BIT_INTEGER   ((a2b_UInt32)1u << (a2b_UInt32)2u)
#define A2B_64BIT_INTEGER   ((a2b_UInt32)1u << (a2b_UInt32)3u)
/** /} */


/*======================= L O C A L  P R O T O T Y P E S  =========*/
static a2b_Int8 a2b_alphaToDigit(a2b_Char    ch);
static a2b_UInt8 a2b_doCrc8(a2b_UInt8   crc_in,
    a2b_UInt8   data);
static a2b_Char a2b_textToUnsigned(a2b_Char ch,
    const a2b_Char**  src, a2b_UInt16 base,
    a2b_UInt32*         retNum);
static void a2b_unsignedToText(a2b_UInt32  num,
    a2b_UInt16  base, a2b_Bool    upperCase,
    a2b_Char*   buf);
#ifdef A2B_FEATURE_64_BIT_INTEGER
static void a2b_unsigned64ToText(a2b_UInt64  num,
    a2b_UInt16  base, a2b_Bool    upperCase,
    a2b_Char*   buf);
static void a2b_signed64ToText(a2b_Int64   num,
    a2b_Char*   buf);
#endif
static void a2b_signedToText(a2b_Int32   num,
    a2b_Char*   buf);
/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/


/*!****************************************************************************
*  \ingroup        a2bstack_util_priv
* 
*  \b              a2b_alphaToDigit
*
*  Returns the integral representation of textual digit.
*
*  \param          [in]    ch       The textual digit to convert to a number.
*
*  \pre            None
*
*  \post           None
*
*  \return         The integral value of the textual digit or -1 if it could not
*                  be converted.
*
******************************************************************************/
static a2b_Int8
a2b_alphaToDigit
    (
    a2b_Char    ch
    )
{
    a2b_Int8 value = -1;

    if ( (ch >= '0') && (ch <= '9') )
    {
        value = (a2b_Int8)(ch - '0');
    }
    else if ( (ch >= 'a') && (ch <= 'f') )
    {
        value = (a2b_Int8)(ch - 'a' + 10);
    }
    else if ( (ch >= 'A') && (ch <= 'F') )
    {
        value = (a2b_Int8)(ch - 'F' + 10);
    }
    else
    {
        /* Completing the control statement */
    }

    return value;
} /* a2b_alphaToDigit */


/*!****************************************************************************
*  \ingroup        a2bstack_util_priv
* 
*  \b              a2b_doCrc8
*
*  Do the actual work to calculate the CRC8 using the ADI algorithm.
*
*  \param          [in]    crc_in   current CRC value
*  \param          [in]    data     single byte of data
*
*  \pre            None
*
*  \post           None
*
*  \return         new CRC value
*
******************************************************************************/
static a2b_UInt8
a2b_doCrc8
    (
    a2b_UInt8   crc_in,
    a2b_UInt8   data
    )
{
    a2b_UInt8 crc_out;
    a2b_UInt8 xor_mask;
    a2b_UInt8 i, j;

    crc_out = crc_in;
    for ( i = 0u; i < 8u; i++ )
    {
        j = 7u - i;
        xor_mask = (a2b_UInt8)((((a2b_UInt32)crc_out & 0x80u) >> (a2b_UInt32)7u) ^ (((a2b_UInt32)data >> (a2b_UInt32)j) & (a2b_UInt32)1u));
        xor_mask = xor_mask | (a2b_UInt8)(((a2b_UInt32)xor_mask << (a2b_UInt32)1u) | ((a2b_UInt32)xor_mask << (a2b_UInt32)2u));
        crc_out  = (a2b_UInt8)((((a2b_UInt32)crc_out << (a2b_UInt32)1u) & (a2b_UInt32)0xffu) ^ (a2b_UInt32)xor_mask);
    }

    return crc_out;
} /* a2b_doCrc8 */


/*!****************************************************************************
*  \ingroup        a2bstack_util_priv
* 
*  \b              a2b_textToUnsigned
*
*  Converts text to the it's corresponding unsigned value.
*
*  \param         [in]     ch       The first textual digit to convert to
*                                   a number.
*                                                                                      
*  \param         [out]    src      Pointer to string containing the remaining
*                                   numbers to convert. Must be null terminated.
*                                                                                      
*  \param         [in]     base     The numerical numbering base of the digits.
*                                                                                      
*  \param         [in,out] retNum   Pointer to an unsigned number that will
*                                   be assigned the converted value.
*
*  \pre            None
*
*  \post           None
*
*  \return         The next character in the src string where decoding stopped.
*
******************************************************************************/
static a2b_Char
a2b_textToUnsigned
    (
    a2b_Char            ch,
    const a2b_Char**    src,
    a2b_UInt16          base,
    a2b_UInt32*         retNum
    )
{
    a2b_UInt32 num = 0u;
    const a2b_Char* ptr = *src;
    a2b_Int8 digit;

    digit = a2b_alphaToDigit(ch);
    while (digit >= 0 )
    {
        if ( digit > (a2b_Int8)base )
        {
            break;
        }
        num = (a2b_UInt32)((num * base) + (a2b_UInt32)digit);
        ++ptr;
        ch = *ptr;
        digit = a2b_alphaToDigit(ch);
    }

    *retNum = num;
    *src = ptr;
    return ch;
} /* a2b_textToUnsigned */


/*!****************************************************************************
*  \ingroup        a2bstack_util_priv
* 
*  \b              a2b_unsignedToText
*
*  Converts the number using the requested base and upper case it if requested.
*  The returned buffer will be null terminated.
*
*  \param         [in]     num          The unsigned number to convert to text.
* 
*  \param         [in]     base         The base of the number (e.g. 10 or 16).
* 
*  \param         [in]     upperCase    If A2B_TRUE, then hexadecimal values 
*                                       in range 'a'-'f' will be upper cased.
* 
*  \param         [in,out] buf          The raw buffer to hold the converted 
*                                       number as a string. Must be large 
*                                       enough to hold any conversion including 
*                                       the terminating null.  This must be
*                                       >= #A2B_MAX_FMT_CONV_BUF_SIZE
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_unsignedToText
    (
    a2b_UInt32  num,
    a2b_UInt16  base,
    a2b_Bool    upperCase,
    a2b_Char*   buf
    )
{
    a2b_Int16   n = 0;
    a2b_UInt32  d = 1u;
    a2b_UInt32  dgt;
    a2b_UInt32 nTempVar;
    a2b_UInt32 nTempDgt;

    while ( num / d >= base )
    {
        d *= base;
    }

    while ( d != 0u )
    {
        dgt = num / d;
        num %= d;
        d /= base;
        if ( n || (dgt > 0u) || (d == 0u) )
        {
        	nTempVar = (a2b_UInt32)(upperCase ? 'A' : 'a') - (a2b_UInt32)10u;
        	nTempDgt = dgt < (a2b_UInt32)10u ? (a2b_UInt32)'0' : nTempVar;
        	nTempVar = dgt + nTempDgt;
            *buf = (a2b_Char)(nTempVar);
            buf++;
            n++;
        }
    }

    /* Make sure the buffer it zero terminated */
    *buf = '\0';
} /* a2b_unsignedToText */


#ifdef A2B_FEATURE_64_BIT_INTEGER

/*!****************************************************************************
*  \ingroup        a2bstack_util_priv
* 
*  \b              a2b_unsigned64ToText
*
*  Converts the number using the requested base and upper case it if requested.
*  The returned buffer will be null terminated.
*
*  \param         [in]     num          The unsigned number to convert to text.
* 
*  \param         [in]     base         The base of the number (e.g. 10 or 16).
* 
*  \param         [in]     upperCase    If A2B_TRUE, then hexadecimal values 
*                                       in range 'a'-'f' will be upper cased.
* 
*  \param         [in,out] buf          The raw buffer to hold the converted 
*                                       number as a string. Must be large 
*                                       enough to hold any conversion including 
*                                       the terminating null.  This must be
*                                       >= #A2B_MAX_FMT_CONV_BUF_SIZE
*
*  \pre            Only available when #A2B_FEATURE_64_BIT_INTEGER is enabled.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_unsigned64ToText
    (
    a2b_UInt64  num,
    a2b_UInt16  base,
    a2b_Bool    upperCase,
    a2b_Char*   buf
    )
{
    a2b_Int16   n = 0;
    a2b_UInt64  d = 1u;
    a2b_UInt64  dgt;
    a2b_UInt64 nTempVar;
    a2b_UInt64 nTempDgt;

    while ( num / d >= base )
    {
        d *= base;
    }

    while ( d != 0u )
    {
        dgt = num / d;
        num %= d;
        d /= base;
        if ( n || (dgt > 0u) || (d == 0u) )
        {
        	nTempVar = (a2b_UInt64)(upperCase ? 'A' : 'a') - (a2b_UInt64)10u;
        	nTempDgt = dgt < (a2b_UInt64)10u ? (a2b_UInt64)'0' : nTempVar;
        	nTempVar = dgt + nTempDgt;

            *buf = (a2b_Char)(nTempVar);
            buf++;
            n++;
        }
    }

    /* Make sure the buffer it zero terminated */
    *buf = '\0';
} /* a2b_unsigned64ToText */


/*!****************************************************************************
*  \ingroup        a2bstack_util_priv
* 
*  \b              a2b_signed64ToText
*
*  Only supports base 10 conversion. The returned buffer will be null
*  terminated.
*
*  \param          [in]    num          The signed number to convert to text.
* 
*  \param         [in,out] buf          The raw buffer to hold the converted 
*                                       number as a string. Must be large 
*                                       enough to hold any conversion including 
*                                       the terminating null.  This must be
*                                       >= #A2B_MAX_FMT_CONV_BUF_SIZE
*
*  \pre            Only available when #A2B_FEATURE_64_BIT_INTEGER is enabled.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_signed64ToText
    (
    a2b_Int64   num,
    a2b_Char*   buf
    )
{
    if ( num < 0 )
    {
        num = -num;
        *buf = '-';
        buf++;
    }

    a2b_unsigned64ToText((a2b_UInt64)num, 10u, A2B_FALSE, buf);
} /* a2b_signed64ToText */

#endif /* A2B_64_BIT_INT_AVAILABLE */


/*!****************************************************************************
*  \ingroup        a2bstack_util_priv
* 
*  \b              a2b_signedToText
*
*  Only supports base 10 conversion. The returned buffer will be null
*  terminated.
*
*  \param         [in]        num   The signed number to convert to text.
* 
*  \param         [in,out]    buf   The raw buffer to hold the converted 
*                                   number as a string. Must be large 
*                                   enough to hold any conversion including 
*                                   the terminating null.  This must be
*                                   >= #A2B_MAX_FMT_CONV_BUF_SIZE
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_signedToText
    (
    a2b_Int32   num,
    a2b_Char*   buf
    )
{
    if ( num < 0 )
    {
        num = -num;
        *buf = '-';
        buf++;
    }

    a2b_unsignedToText((a2b_UInt32)num, 10u, A2B_FALSE, buf);
} /* a2b_signedToText */


/*!****************************************************************************
*
*  \b              a2b_crc8Cont
*
*  Calculate a CRC-8 using the polynomial:
*  \f$ x^8 + x^2 + x + 1 \f$
*
*  Calculation starts with an initial value of 0xFF. Bits are passed
*  through the CRC calculation in the same order in which they appear 
*  on the I2C bus. 
*
*  For example, for the CRC-8 value at address 
*  0x0006, the byte at address 0 is processed first going from MSB 
*  to LSB. After this the byte at address 1 is processed, and so on
*  (through the byte at address 5).
* 
*  \note This routine allows you to start the CRC using #a2b_crc8, then
*        pass the calculated CRC to this routine and continue the CRC
*        with a new buffer.
* 
*  \note This routine can be used instead of #a2b_crc8 BUT the initCrc
*        ADI uses is 0xFF.
*
*  \param          [in]    dataArr      data array
* 
*  \param          [in]    initCrc8     intial CRC8 value
* 
*  \param          [in]    startIdx     starting point of CRC
* 
*  \param          [in]    crcLen       number of bytes to CRC
*
*  \pre            User MUST ensure the (startIdx + crcLen) does not
*                  exceed the size of the dataArr.
*
*  \post           None
*
*  \return         CRC value for the data
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt8
a2b_crc8Cont
    (
    const a2b_UInt8*  dataArr,
    a2b_UInt8         initCrc8,
    a2b_UInt32        startIdx,
    a2b_UInt32        crcLen
    )
{
    a2b_UInt32  i;
    a2b_UInt8   crc8_val = initCrc8;

    if (( dataArr == A2B_NULL ) || ( startIdx > crcLen ))
    {
        return crc8_val;
    }

    for ( i = startIdx; i < (startIdx+crcLen); i++ )
    {
        crc8_val = a2b_doCrc8( crc8_val, dataArr[i] );
    }

    return crc8_val;
} /* a2b_crc8Cont */


/*!****************************************************************************
*
*  \b              a2b_crc8
*
*  Calculate a CRC-8 using the polynomial:
*  \f$ x^8 + x^2 + x + 1 \f$
*
*  Calculation starts with an initial value of 0xFF. Bits are passed
*  through the CRC calculation in the same order in which they appear 
*  on the I2C bus. 
*
*  For example, for the CRC-8 value at address 
*  0x0006, the byte at address 0 is processed first going from MSB 
*  to LSB. After this the byte at address 1 is processed, and so on
*  (through the byte at address 5).
*
*  \param          [in]    dataArr      data array
* 
*  \param          [in]    startIdx     starting point of CRC
* 
*  \param          [in]    crcLen       number of bytes to CRC
*
*  \pre            User MUST ensure the (startIdx + crcLen) does not
*                  exceed the size of the dataArr.
*
*  \post           None
*
*  \return         CRC value for the data
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt8
a2b_crc8
    (
    const a2b_UInt8*  dataArr,
    a2b_UInt32        startIdx,
    a2b_UInt32        crcLen
    )
{
    return a2b_crc8Cont( dataArr, 0xFFu, startIdx, crcLen );
} /* a2b_crc8 */


/*!****************************************************************************
*
*  \b              a2b_memset
*
*  Fills the first 'n' bytes of the memory area pointed to by 's' with the
*  constant byte 'c'.
*
*  \param          [in]    s    Pointer to the area of memory to fill.
* 
*  \param          [in]    c    The constant byte to fill the memory with.
* 
*  \param          [in]    n    The number of bytes to fill with the constant.
*
*  \pre            None
*
*  \post           None
*
*  \return         Pointer to the memory set.
*
******************************************************************************/
A2B_DSO_PUBLIC void*
a2b_memset
    (
    void*       s,
    a2b_Int     c,
    a2b_Size    n
    )
{
    a2b_Byte* ptr = (a2b_Byte*)s;

    if ( A2B_NULL != ptr )
    {
        while ( n > (a2b_Size)0 )
        {
            *ptr = (a2b_Byte)c;
            ptr++;
            n--;
        }
    }

    return s;
} /* a2b_memset */


/*!****************************************************************************
*
*  \b              a2b_memcpy
*
*  Copies 'n' bytes from memory area 'src' to memory area 'dest'. The
*  memory areas must *not* overlap.
*
*  \param          [in]    dest     The destination to move the memory to.
* 
*  \param          [in]    src      Pointer to the source memory to copy.
* 
*  \param          [in]    n        The number of bytes to copy.
*
*  \pre            None
*
*  \post           None
*
*  \return         Pointer to the destination.
*
******************************************************************************/
A2B_DSO_PUBLIC void*
a2b_memcpy
    (
    void*       dest,
    const void* src,
    a2b_Size    n
    )
{
    a2b_Byte* d = (a2b_Byte*)dest;
    a2b_Byte* s = (a2b_Byte*)src;

    if ( (A2B_NULL != dest) && (A2B_NULL != src) )
    {
        while ( n > (a2b_Size)0 )
        {
            *d = *s;
            d++;
            s++;
            n--;
        }
    }
    return dest;
} /* a2b_memcpy */


/*!****************************************************************************
*
*  \b              a2b_strlen
*
*  Calculates the length of the string excluding the terminating null byte.
*
*  \param          [in]    s    The null terminated string to count the length.
*
*  \pre            None
*
*  \post           None
*
*  \return         The number of bytes in string 's'.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Size
a2b_strlen
    (
    const a2b_Char* s
    )
{
    a2b_Size len = 0u;

    if ( A2B_NULL != s )
    {
        while ( s[len] != '\0' )
        {
            len++;
        }
    }
    return len;
} /* a2b_strlen */


/*!****************************************************************************
*
*  \b              a2b_strcpy
*
*  Copies the contents of the source string (including the terminating
*  null ('\0')) to the destination string. The strings may not overlap and
*  the destination buffer must be large enough to hold the string.
*
*  \param          [in]    dest     The destination buffer to receive the
*                                   copied source.
* 
*  \param          [in]    src      The source string to copy.
*
*  \pre            None
*
*  \post           None
*
*  \return         A pointer to the destination string.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Char*
a2b_strcpy
    (
    a2b_Char*       dest,
    const a2b_Char* src
    )
{
    a2b_UInt32 idx;

    if ( (A2B_NULL != dest) && (A2B_NULL != src) )
    {
        for ( idx = 0u; src[idx] != '\0'; ++idx )
        {
            dest[idx] = src[idx];
        }
        dest[idx] = '\0';
    }

    return dest;
} /* a2b_strcpy */


/*!****************************************************************************
*
*  \b              a2b_strncpy
*
*  Copies the contents of the source string (including the terminating
*  null ('\0')) to the destination string except at most 'n' characters are
*  copied. The strings may not overlap and if there is no null byte among the
*  first 'n' bytes of 'src' then 'dest' will *NOT* be null terminated.
*
*  \param          [in]    dest     The destination buffer to receive the
*                                   copied source.
* 
*  \param          [in]    src      The source string to copy.
* 
*  \param          [in]    n        The maximum number of characters to copy.
*
*  \pre            None
*
*  \post           None
*
*  \return         A pointer to the destination string.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Char*
a2b_strncpy
    (
    a2b_Char*       dest,
    const a2b_Char* src,
    a2b_Size        n
    )
{
    a2b_Size idx;

    if ( (A2B_NULL != dest) && (A2B_NULL != src) )
    {
        for ( idx = 0u; (idx < n) && (src[idx] != '\0'); ++idx )
        {
            dest[idx] = src[idx];
        }

        for ( ; idx < n; ++idx )
        {
            dest[idx] = '\0';
        }
    }

    return dest;
} /* a2b_strncpy */


/*!****************************************************************************
*
*  \b              a2b_strrchr
*
*  Returns a pointer to the last occurrence of the character 'c' in
*  string 's'.  If character 'c' is not found in 's' then A2B_NULL is returned.
*
*  \param          [in]    s    The null terminated string to search for 'c'.
* 
*  \param          [in]    c    The last occurrence of 'c' to search for in 's'.
*
*  \pre            None
*
*  \post           None
*
*  \return         The pointer to the last occurrence of 'c' in 's' or A2B_NULL 
*                  if no match is found.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Char*
a2b_strrchr
    (
    const a2b_Char* s,
    a2b_Char        c
    )
{
    a2b_Char* found = A2B_NULL;
    a2b_UInt32 idx = 0u;

    if ( A2B_NULL != s )
    {
        /* Find the null terminator at the end */
        while ( s[idx] != '\0' )
        {
            ++idx;
        }

        /* Now search for the matching character starting from the end
         * moving towards the beginning.
         */
        do
        {
            if ( s[idx] == c )
            {
                found = (a2b_Char*)&s[idx];
                break;
            }
            idx--;
        }
        while ( idx > 0u );

    }
    return found;
} /* a2b_strrchr */


/*!****************************************************************************
*
*  \b              a2b_vsnprintfStringBuffer
*
*  Produces output into the specified buffer using the formatting rules.
* 
*  Similar to the standard C library function vsnprintf() this function
*  produces output into the sized buffer such that at most 'bufSize' characters
*  are written including a terminating null ('\0') to buf. Only a
*  subset of the formatting options are available with this implementation
*  to keep things simple. The supported options include:
* 
*       %[flags][width][length]specifier
* 
*       flags:
*           0       -   Zero fill to the left
* 
*       width:
*           0-N     -   The amount of space to allocate to the argument
* 
*       length:
*           b       -   Pointer to 8-bit integer provided
*           h       -   Pointer to 16-bit integer provided
*           l       -   Pointer to 32-bit integer provided
*           L       -   Pointer to 64-bit integer provided
*           <none>  -   Assume pointer to 32-bit integer provided
* 
*       specifier:
*           c       -   Character
*           u       -   Unsigned integer (base 10)
*           i       -   Signed integer (base 10)
*           d       -   Signed integer (base 10)
*           p       -   A pointer to be shown as a lower-case hex value.
*           P       -   A pointer to be shown as an upper-case hex value.
*           s       -   A null terminated string
*           x       -   Value expressed as hexadecimal (lower-case)
*           X       -   Value expressed as hexadecimal (upper-case)
*           %       -   Outputs '%'
* 
*  \param         [in,out] strBuf   The previously initialized string buffer 
*                                   where the output will be stored including
*                                   the terminating null.
*
*  \param         [in]     fmt      The format string.
* 
*  \param         [in]     args     An array of void* pointers to arguments. 
*                                   It is assumed all arguments are passed in 
*                                   as a pointer to a specified type defined 
*                                   in the format string.
* 
*  \param         [in]     numArgs  The number of arguments in the argument
*                                   list.
*
*  \pre            None
*
*  \post           None
*
*  \return         The number of characters that would've been written into the
*                  buffer had there been room (excluding the terminating null).
*                  A value greater than or equal to the buffer size indicates
*                  that the output has been truncated. The value is negative
*                  if there is an error.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Int
a2b_vsnprintfStringBuffer
    (
    struct a2b_StringBuffer*    strBuf,
    const a2b_Char*             fmt,
    void**                      args,
    a2b_UInt16                  numArgs
    )
{
    a2b_Char convBuf[A2B_MAX_FMT_CONV_BUF_SIZE];
    a2b_Char ch;
    a2b_Int nWritten = -1;
    const a2b_Char* fmtPos = fmt;
    a2b_Bool zeroFill;
    a2b_UInt8 sizeFlag = 0u;
    a2b_UInt32 width;
    a2b_UInt16 argIdx = 0u;
    a2b_UInt32 uValue;
    a2b_Int32 sValue;
    a2b_UInt8 nTemp8Var;
    a2b_UInt16 nTemp16Var;
#ifdef A2B_FEATURE_64_BIT_INTEGER
    a2b_UInt64 uValue64;
    a2b_Int64 sValue64;
#endif

    if ( (A2B_NULL != strBuf) && (A2B_NULL != fmt) )
    {
        nWritten = 0;

        /* Loop while we haven't reached the end of the format string */
        while ( *fmtPos != '\0' )
        {
            ch = *fmtPos;
            fmtPos++;
            if ( ch != '%' )
            {
                (void)a2b_stringBufferAppend(strBuf, ch);
                nWritten++;
            }
            else
            {
                zeroFill = A2B_FALSE;
                sizeFlag = (a2b_UInt8)0;
                width = (a2b_UInt32)0;
                ch = *fmtPos;

                if ( '0' == ch )
                {
                    zeroFill = A2B_TRUE;
                    fmtPos++;
                    ch = *fmtPos;
                }

                if ( (ch >= '0') && (ch <= '9') )
                {
                    ch = a2b_textToUnsigned(ch, &fmtPos, (a2b_UInt16)10, &width);
                }

                if ( ch == 'b' )
                {
                    sizeFlag = (a2b_UInt8)A2B_8BIT_INTEGER;
                    fmtPos++;
                    ch = *fmtPos;
                }

                if ( ch == 'h' )
                {
                    sizeFlag = (a2b_UInt8)A2B_16BIT_INTEGER;
                    fmtPos++;
                    ch = *fmtPos;
                }

                if ( ch == 'l' )
                {
                    sizeFlag = (a2b_UInt8)A2B_32BIT_INTEGER;
                    fmtPos++;
                    ch = *fmtPos;
                }

#ifdef A2B_FEATURE_64_BIT_INTEGER
                if ( ch == 'L' )
                {
                    sizeFlag = (a2b_UInt8)A2B_64BIT_INTEGER;
                    fmtPos++;
                    ch = *fmtPos;
                }
#endif
                switch ( ch )
                {
                    case '\0':
                        /* We've reached the end of the format string */
                        break;

                    case 'u':
                    case 'x':
                    case 'X':
                        if ( numArgs > argIdx )
                        {
                            if ( sizeFlag & (a2b_UInt8)A2B_64BIT_INTEGER )
                            {
#ifdef A2B_FEATURE_64_BIT_INTEGER
                                uValue64 = *((a2b_UInt64*)args[argIdx]);
                                a2b_unsigned64ToText(uValue64,
                                (a2b_UInt16)(ch == 'u' ? 10 : 16), (a2b_Bool)(ch == 'X'), convBuf);
#else
                                convBuf[0] = '\0';
#endif
                            }
                            else
                            {
                                if ( sizeFlag & A2B_8BIT_INTEGER )
                                {
                                	nTemp8Var = *((a2b_UInt8*)args[argIdx]) & (a2b_UInt8)0xFFu;
                                    uValue = (a2b_UInt32)(nTemp8Var);
                                }
                                else if ( sizeFlag & A2B_16BIT_INTEGER )
                                {
                                	nTemp16Var = *((a2b_UInt16*)args[argIdx]) & (a2b_UInt16)0xFFFFu;
                                    uValue = (a2b_UInt32)(nTemp16Var);
                                }
                                else    /* Assume 32 bit integer */
                                {
                                    uValue = *((a2b_UInt32*)args[argIdx]) &
                                        (a2b_UInt32)0xFFFFFFFFu;
                                }
                                a2b_unsignedToText(uValue, (a2b_UInt16)(ch == 'u' ? 10 : 16),
                                		(a2b_Bool)(ch == 'X'), convBuf);
                            }
                            argIdx++;
                            nWritten += (a2b_Int)a2b_stringBufferAppendWithFill(width,
                                                zeroFill, strBuf, convBuf);
                        }
                        fmtPos++;
                        break;

                    case 'i':
                    case 'd':
                        if ( numArgs > argIdx )
                        {
                            if ( sizeFlag & A2B_64BIT_INTEGER )
                            {
#ifdef A2B_FEATURE_64_BIT_INTEGER
                                sValue64 = *((a2b_Int64*)args[argIdx]);
                                a2b_signed64ToText(sValue64, convBuf);
#else
                                convBuf[0] = '\0';
#endif
                            }
                            else
                            {
                                if ( sizeFlag & A2B_8BIT_INTEGER )
                                {
                                    sValue = *((a2b_Int8*)args[argIdx]);
                                }
                                else if ( sizeFlag & A2B_16BIT_INTEGER )
                                {
                                    sValue = *((a2b_Int16*)args[argIdx]);
                                }
                                else    /* Assume 32 bit integer */
                                {
                                    sValue = *((a2b_Int32*)args[argIdx]);
                                }
                                a2b_signedToText(sValue, convBuf);
                            }
                            argIdx++;
                            nWritten += (a2b_Int)a2b_stringBufferAppendWithFill(width,
                                                zeroFill, strBuf, convBuf);
                        }
                        fmtPos++;
                        break;

                    case 'c':
                        if ( numArgs > argIdx )
                        {
                            (void)a2b_stringBufferAppend(strBuf,
                                                   *((a2b_Char*)args[argIdx]));
                            argIdx++;
                            nWritten++;
                        }
                        fmtPos++;
                        break;

                    case 'p':
                    case 'P':
                        if ( numArgs > argIdx )
                        {
                            /* NOTE: This won't work right with pointer sizes
                             * that exceed the size of an appropriate unsigned
                             * integer.
                             */
#ifdef A2B_FEATURE_64_BIT_INTEGER
                            uValue64 = (a2b_UIntPtr)args[argIdx];
                            a2b_unsigned64ToText(uValue64, 16u,
                                                 (a2b_Bool)(ch == 'P'), convBuf);
#else
                            uValue = (a2b_UIntPtr)args[argIdx];
                            a2b_unsignedToText(uValue, 16u, (a2b_Bool)(ch == 'P'), convBuf);
#endif
                            nWritten += (a2b_Int)a2b_stringBufferAppendWithFill(0u,
                                                A2B_FALSE, strBuf, convBuf);
                            argIdx++;
                        }
                        fmtPos++;
                        break;

                    case 's':
                        if ( numArgs > argIdx )
                        {
                            nWritten += (a2b_Int)a2b_stringBufferAppendWithFill(width,
                                            A2B_FALSE, strBuf,
                                            (a2b_Char*)args[argIdx]);
                            argIdx++;
                        }
                        fmtPos++;
                        break;

                    case '%':
                        (void)a2b_stringBufferAppend(strBuf, ch);
                        nWritten++;
                        fmtPos++;
                        break;

                    default:
                        fmtPos++;
                        break;
                }
            }
        }
    }

    return nWritten;
} /* a2b_vsnprintfStringBuffer */


/*!****************************************************************************
*
*  \b              a2b_vsnprintf
*
*  Produces output into the specified buffer using the formatting rules.
* 
*  Similar to the standard C library function vsnprintf() this function
*  produces output into the sized buffer such that at most 'bufSize' characters
*  are written including a terminating null ('\0') to buf. Only a
*  subset of the formatting options are available with this implementation
*  to keep things simple. The supported options include:
* 
*       %[flags][width][length]specifier
* 
*       flags:
*           0       -   Zero fill to the left
* 
*       width:
*           0-N     -   The amount of space to allocate to the argument
* 
*       length:
*           b       -   Pointer to 8-bit integer provided
*           h       -   Pointer to 16-bit integer provided
*           l       -   Pointer to 32-bit integer provided
*           <none>  -   Assume pointer to 32-bit integer provided
* 
*       specifier:
*           c       -   Character
*           u       -   Unsigned integer (base 10)
*           i       -   Signed integer (base 10)
*           d       -   Signed integer (base 10)
*           x       -   Value expressed as hexadecimal (lower-case)
*           X       -   Value expressed as hexadecimal (upper-case)
*           %       -   Outputs '%'
*
*  \param         [in,out] buf      The buffer where the output will be stored 
*                                   including the terminating null.
* 
*  \param         [in]     bufSize  The number of characters that can be
*                                   written into the buffer.
* 
*  \param         [in]     fmt      The format string.
* 
*  \param         [in]     args     An array of void* pointers to arguments. 
*                                   It is assumed all arguments are passed in 
*                                   as a pointer to a specified type defined 
*                                   in the format string.
* 
*  \param         [in]     numArgs  The number of arguments in the argument
*                                   list.
*
*  \pre           None
*
*  \post          None
*
*  \return        The number of characters that would've been written into the
*                 buffer had there been room (excluding the terminating null).
*                 A value greater than or equal to the buffer size indicates
*                 that the output has been truncated. The value is negative
*                 if there is an error.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Int
a2b_vsnprintf
    (
    a2b_Char*       buf,
    a2b_Size        bufSize,
    const a2b_Char* fmt,
    void**          args,
    a2b_UInt16      numArgs
    )
{
    a2b_StringBuffer strBuf;
    a2b_Int nWritten = -1;

    if ( (A2B_NULL != buf) && (bufSize >= sizeof(a2b_Char)) &&
        (A2B_NULL != fmt) )
    {
        a2b_stringBufferInit(&strBuf, buf, bufSize);
        nWritten = a2b_vsnprintfStringBuffer(&strBuf, fmt, args, numArgs);
    }

    return nWritten;
} /* a2b_vsnprintf */
