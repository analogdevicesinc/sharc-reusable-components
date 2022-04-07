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
 * \file:   stringbuffer.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of the string buffer object.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/
#include "a2b/stringbuffer.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!****************************************************************************
*
*  \b              a2b_stringBufferInit
*
*  Initializes the StringBuffer structure.
*
*  \param          [in]    strBuf       The StringBuffer to initialize.
* 
*  \param          [in]    rawBuf       The raw character to be managed by the
*                                       StringBuffer.
* 
*  \param          [in]    size         The size (in characters) of the
*                                       raw buffer.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_stringBufferInit
    (
    a2b_StringBuffer*   strBuf,
    a2b_Char*           rawBuf,
    a2b_UInt32          size
    )
{
    if ( A2B_NULL != strBuf )
    {
        strBuf->buf = rawBuf;
        strBuf->capacity = size;
        strBuf->pos = 0u;
        if ( size > 0u )
        {
            strBuf->buf[0u] = '\0';
        }
    }
} /* a2b_stringBufferInit */


/*!****************************************************************************
*
*  \b              a2b_stringBufferAppend
*
*  Attempts to append the character to the end of the StringBuffer. If there
*  is no room available then the character will *not* be appended. The
*  underlying buffer is always null terminated unless it has a capacity of
*  less than a single character.
*
*  \param          [in]    strBuf   The StringBuffer to append the character to.
* 
*  \param          [in]    ch       The character to append.
*
*  \pre            The StringBuffer 'strBuf' has been initialized.
*
*  \post           None
*
*  \return         Returns A2B_TRUE if the character was appended or A2B_FALSE
*                  otherwise.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Bool
a2b_stringBufferAppend
    (
    a2b_StringBuffer*   strBuf,
    a2b_Char            ch
    )
{
    a2b_Bool written = A2B_FALSE;

    if ( A2B_NULL != strBuf )
    {
        if( strBuf->capacity > strBuf->pos + 1u)
        {
            strBuf->buf[strBuf->pos] = ch;
            strBuf->pos++;
            strBuf->buf[strBuf->pos] = '\0';
            written = A2B_TRUE;
        }
    }
    return written;

} /* a2b_stringBufferAppend */


/*!****************************************************************************
*
*  \b              a2b_stringBufferLength
*
*  Returns the length of the null terminated buffer (excluding) the null
*  terminator.
*
*  \param          [in]    strBuf       The StringBuffer to return the length.
*
*  \pre            The StringBuffer 'strBuf' has already been initialized.
*
*  \post           None
*
*  \return         Returns the length of the null terminated string buffer.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_stringBufferLength
    (
    const a2b_StringBuffer* strBuf
    )
{
    a2b_UInt32 length = 0u;
    if ( A2B_NULL != strBuf )
    {
        length = strBuf->pos;
    }

    return length;

} /* a2b_stringBufferLength */


/*!****************************************************************************
*
*  \b              a2b_stringBufferClear
*
*  Resets and clears the string buffer.
*
*  \param          [in]    strBuf       The StringBuffer to clear.
*
*  \pre            The StringBuffer 'strBuf' has already been initialized.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_PUBLIC void
a2b_stringBufferClear
    (
    a2b_StringBuffer*   strBuf
    )
{
    if ( A2B_NULL != strBuf )
    {
        strBuf->pos = 0u;
        if ( strBuf->capacity > 0u )
        {
            strBuf->buf[0u] = '\0';
        }
    }
} /* a2b_stringBufferClear */


/*!****************************************************************************
*
*  \b              a2b_stringBufferAppendWithFill
*
*  Puts (or appends) the source string into the StringBuffer with the
*  correct width and zero fill rules being realized.
*
*  \param          [in]    width        The required width of the source. If 
*                                       the source string is shorter than the 
*                                       width then the space will be optionally
*                                       padded with spaces or zeros.
* 
*  \param          [in]    zeroFill     Whether not to fill with zeros (0)
*                                       or spaces.
* 
*  \param          [in]    dest         The StringBuffer where the source is
*                                       copied into.
* 
*  \param          [in]    src          The null terminated string to copy
*                                       into dest.
*
*  \pre            The StringBuffer 'strBuf' has already been initialized.
*
*  \post           None
*
*  \return         The number of characters that would have been written into 
*                  the destination StringBuffer if enough space were available.
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_UInt32
a2b_stringBufferAppendWithFill
    (
    a2b_UInt32          width,
    a2b_Bool            zeroFill,
    a2b_StringBuffer*   dest,
    const a2b_Char*     src
    )
{
    a2b_Char fill = (a2b_Char)(zeroFill ? '0' : ' ');
    const a2b_Char* p = src;
    a2b_UInt32 nWritten = 0u;

    /* Determine how much 'fill' is needed */
    while ( ('\0' != *p) && (width > 0u) )
    {
        p++;
        width--;
    }

    while ( width > 0u )
    {
        (void)a2b_stringBufferAppend(dest, fill);
        nWritten++;
        width--;
    }

    p = src;
    while ( '\0' != *p )
    {
        (void)a2b_stringBufferAppend(dest, *p);
        nWritten++;
        p++;
    }

    return nWritten;

} /* a2b_stringBufferAppendWithFill */
