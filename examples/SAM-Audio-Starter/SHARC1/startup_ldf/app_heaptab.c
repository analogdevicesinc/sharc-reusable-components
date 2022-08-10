/*
** ADSP-SC589 user heap source file generated on Aug 23, 2021 at 14:10:32.
*/
/*
** Copyright (C) 2000-2019 Analog Devices Inc., All Rights Reserved.
**
** This file is generated automatically based upon the options selected
** in the System Configuration utility. Changes to the Heap configuration
** should be made by modifying the appropriate options rather than editing
** this file. To access the System Configuration utility, double-click the
** system.svc file from a navigation view.
**
** Custom additions can be inserted within the user-modifiable sections,
** these are bounded by comments that start with "$VDSG". Only changes
** placed within these sections are preserved when this file is re-generated.
**
** Product      : CrossCore Embedded Studio
** Tool Version : 6.2.1.4
*/

#ifdef _MISRA_RULES
#pragma diag(push)
#pragma diag(suppress:misra_rule_1_1)
#pragma diag(suppress:misra_rule_6_3)
#pragma diag(suppress:misra_rule_8_10)
#pragma diag(suppress:misra_rule_10_1_a)
#pragma diag(suppress:misra_rule_11_3)
#endif /* _MISRA_RULES */

#pragma file_attr("libData=HeapTable")

#include <stdlib.h>
#include <limits.h>

extern "asm" unsigned long ldf_heap_space;
extern "asm" unsigned long ldf_heap_length;

const __heap_desc_t heap_table[2] =
{

  { &ldf_heap_space, (unsigned long) &ldf_heap_length, 0 },

  { (void *)0, (size_t)0, 0 } /* This terminates the table. */
};

#if CHAR_BIT != 8
#  error This source must be compiled with -char-size-8
#endif

#ifdef _MISRA_RULES
#pragma diag(pop)
#endif /* _MISRA_RULES */

