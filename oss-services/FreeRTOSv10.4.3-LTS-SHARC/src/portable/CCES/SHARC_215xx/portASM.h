/*
 * FreeRTOS Kernel V10.4.3 LTS Patch 2
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

typedef unsigned int UINT32;

/* Saved register set structure. */
typedef struct
{
    UINT32 R0;
    UINT32 R1;
    UINT32 R2;
    UINT32 R3;
    UINT32 R4;
    UINT32 R5;
    UINT32 R6;
    UINT32 R7;
    UINT32 R8;
    UINT32 R9;
    UINT32 R10;
    UINT32 R11;
    UINT32 R12;
    UINT32 R13;
    UINT32 R14;
    UINT32 R15;
    UINT32 S0;
    UINT32 S1;
    UINT32 S2;
    UINT32 S3;
    UINT32 S4;
    UINT32 S5;
    UINT32 S6;
    UINT32 S7;
    UINT32 S8;
    UINT32 S9;
    UINT32 S10;
    UINT32 S11;
    UINT32 S12;
    UINT32 S13;
    UINT32 S14;
    UINT32 S15;

    UINT32 R0_3;
    UINT32 S0_3;
    UINT32 R4_7;
    UINT32 S4_7;
    UINT32 R8_11;
    UINT32 S8_11;
    UINT32 R12_15;
    UINT32 S12_15;

    UINT32 MR0F;
	UINT32 MR1F;
	UINT32 MR2F;
	UINT32 MR0B;
	UINT32 MR1B;
	UINT32 MR2B;

	UINT32 MS0F;
	UINT32 MS1F;
	UINT32 MS2F;
	UINT32 MS0B;
	UINT32 MS1B;
	UINT32 MS2B;

	UINT32 I0;
	UINT32 I1;
	UINT32 I2;
	UINT32 I3;
    UINT32 I4;
	UINT32 I5;
    UINT32 I6;
  //    UINT32 I7; I7 (SP) is stored in the task control block, so we don't need to store it here
	UINT32 I8;
	UINT32 I9;
	UINT32 I10;
	UINT32 I11;
    UINT32 I13;
    UINT32 I12;
	UINT32 I14;
	UINT32 I15;

	UINT32 M0;
	UINT32 M1;
	UINT32 M2;
	UINT32 M3;
    UINT32 M4;
    UINT32 M5;
    UINT32 M6;
    UINT32 M7;
	UINT32 M8;
	UINT32 M9;
	UINT32 M10;
	UINT32 M11;
    UINT32 M12;
    UINT32 M13;
    UINT32 M14;
    UINT32 M15;

	UINT32 B0;
	UINT32 B1;
	UINT32 B2;
	UINT32 B3;
    UINT32 B4;
	UINT32 B5;
    UINT32 B6;
    UINT32 B7;
	UINT32 B8;
	UINT32 B9;
	UINT32 B10;
	UINT32 B11;
    UINT32 B12;
    UINT32 B13;
	UINT32 B14;
	UINT32 B15;

    UINT32 L15;
    UINT32 L14;
    UINT32 L13;
    UINT32 L12;
    UINT32 L11;
    UINT32 L10;
    UINT32 L9;
    UINT32 L8;
    UINT32 L7;
    UINT32 L6;
    UINT32 L5;
    UINT32 L4;
    UINT32 L3;
    UINT32 L2;
    UINT32 L1;
    UINT32 L0;

    UINT32 BitFIFO_1;
	UINT32 BitFIFO_0;
	UINT32 BitFIFOWRP;

	UINT32 ASTATy;
    UINT32 ASTATx;
    UINT32 STKYy;
    UINT32 STKYx;
    UINT32 MODE1;
    UINT32 USTAT4;
    UINT32 USTAT3;
    UINT32 USTAT2;
    UINT32 USTAT1;

    UINT32 PX2;
    UINT32 PX1;
 //   UINT32 RTI;                 /* Saved by the RTL on interrupt entry - Sequence must not be changed */
} ContextRecord;


/* This is a (fake) structure that is only used to emulate a saved
 * context that does not have any hardware stack usage.  Only used
 * when initialising a task. */
typedef struct
{
	UINT32 LOOP_STACK_COUNT;
	UINT32 LCNTR;
	UINT32 PC_STACK_COUNT;
	UINT32 PC_STACK;
} StackRecord;



