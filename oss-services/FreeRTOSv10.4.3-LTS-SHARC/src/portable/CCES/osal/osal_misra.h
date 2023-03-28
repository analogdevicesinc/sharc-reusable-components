/*
 * Copyright (C) 2016-2018 Analog Devices Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
 */

/*! @file osal_misra.h */
 /*! @cond */
#ifndef __ADI_OSAL_MISRA_H__
#define __ADI_OSAL_MISRA_H__


#if defined ( __ICCARM__ )  /* IAR compiler... */

/* assert */
/*#define  ASSERT_ADI_OSAL_MISRA_SUPPRESSIONS  _Pragma ("diag_suppress= Pm001,Pm002,Pm073,Pm088,Pm100,Pm114,Pm120,Pm123,Pm127,Pm128,Pm138,Pm139,Pm140,Pm141,Pm143")*/
/*deassert */
/*#define  REVERT_ADI_OSAL_MISRA_SUPPRESSIONS  _Pragma ("diag_default=  Pm001,Pm002,Pm073,Pm088,Pm100,Pm114,Pm120,Pm123,Pm127,Pm128,Pm138,Pm139,Pm140,Pm141,Pm143")*/
/*assert */

#define  ADI_OSAL_MISRA_SUPPRESS_ALL _Pragma ("diag_suppress=                \
     Pm001,Pm002,Pm003,Pm004,Pm005,Pm006,Pm007,Pm008,Pm009,Pm010,Pm011, \
     Pm012,Pm013,Pm014,Pm015,Pm016,Pm017,Pm018,Pm019,Pm020,Pm021,Pm022, \
     Pm023,Pm024,Pm025,Pm026,Pm027,Pm028,Pm029,Pm030,Pm031,Pm032,Pm033, \
     Pm034,Pm035,Pm036,Pm037,Pm038,Pm039,Pm040,Pm041,Pm042,Pm043,Pm044, \
     Pm045,Pm046,Pm047,Pm048,Pm049,Pm050,Pm051,Pm052,Pm053,Pm054,Pm055, \
     Pm056,Pm057,Pm058,Pm059,Pm060,Pm061,Pm062,Pm063,Pm064,Pm065,Pm066, \
     Pm067,Pm068,Pm069,Pm070,Pm071,Pm072,Pm073,Pm074,Pm075,Pm076,Pm077, \
     Pm078,Pm079,Pm080,Pm081,Pm082,Pm083,Pm084,Pm085,Pm086,Pm087,Pm088, \
     Pm089,Pm090,Pm091,Pm092,Pm093,Pm094,Pm095,Pm096,Pm097,Pm098,Pm099, \
     Pm100,Pm101,Pm102,Pm103,Pm104,Pm105,Pm106,Pm107,Pm108,Pm109,Pm110, \
     Pm111,Pm112,Pm113,Pm114,Pm115,Pm116,Pm117,Pm118,Pm119,Pm120,Pm121, \
     Pm122,Pm123,Pm124,Pm125,Pm126,Pm127,Pm128,Pm129,Pm130,Pm131,Pm132, \
     Pm133,Pm134,Pm135,Pm136,Pm137,Pm138,Pm139,Pm140,Pm141,Pm142,Pm143, \
     Pm144,Pm145,Pm146,Pm147,Pm148,Pm149,Pm150,Pm151,Pm152,Pm153,Pm154, \
     Pm155")

#define  ADI_OSAL_MISRA_RESTORE_ALL _Pragma ("diag_default=                  \
     Pm001,Pm002,Pm003,Pm004,Pm005,Pm006,Pm007,Pm008,Pm009,Pm010,Pm011, \
     Pm012,Pm013,Pm014,Pm015,Pm016,Pm017,Pm018,Pm019,Pm020,Pm021,Pm022, \
     Pm023,Pm024,Pm025,Pm026,Pm027,Pm028,Pm029,Pm030,Pm031,Pm032,Pm033, \
     Pm034,Pm035,Pm036,Pm037,Pm038,Pm039,Pm040,Pm041,Pm042,Pm043,Pm044, \
     Pm045,Pm046,Pm047,Pm048,Pm049,Pm050,Pm051,Pm052,Pm053,Pm054,Pm055, \
     Pm056,Pm057,Pm058,Pm059,Pm060,Pm061,Pm062,Pm063,Pm064,Pm065,Pm066, \
     Pm067,Pm068,Pm069,Pm070,Pm071,Pm072,Pm073,Pm074,Pm075,Pm076,Pm077, \
     Pm078,Pm079,Pm080,Pm081,Pm082,Pm083,Pm084,Pm085,Pm086,Pm087,Pm088, \
     Pm089,Pm090,Pm091,Pm092,Pm093,Pm094,Pm095,Pm096,Pm097,Pm098,Pm099, \
     Pm100,Pm101,Pm102,Pm103,Pm104,Pm105,Pm106,Pm107,Pm108,Pm109,Pm110, \
     Pm111,Pm112,Pm113,Pm114,Pm115,Pm116,Pm117,Pm118,Pm119,Pm120,Pm121, \
     Pm122,Pm123,Pm124,Pm125,Pm126,Pm127,Pm128,Pm129,Pm130,Pm131,Pm132, \
     Pm133,Pm134,Pm135,Pm136,Pm137,Pm138,Pm139,Pm140,Pm141,Pm142,Pm143, \
     Pm144,Pm145,Pm146,Pm147,Pm148,Pm149,Pm150,Pm151,Pm152,Pm153,Pm154, \
     Pm155")

#else
/* assert */
#define  ASSERT_OSAL_ADI_MISRA_SUPPRESSIONS

/* deassert */
#define  REVERT_OSAL_ADI_MISRA_SUPPRESSIONS

#define  ADI_OSAL_MISRA_SUPPRESS_ALL

#define  ADI_OSAL_MISRA_RESTORE_ALL

#endif

#endif /* __ADI_OSAL_MISRA_H__ */
/*! @endcond */
