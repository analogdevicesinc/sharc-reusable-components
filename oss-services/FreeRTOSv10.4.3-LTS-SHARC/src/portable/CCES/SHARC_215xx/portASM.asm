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

.file_attr ISR;
.file_attr OS_Internals;

#include <platform_include.h>
#include <asm_sprt.h>
#include <sys/anomaly_macros_rtl.h>

.import "portASM.h";

/* Macros to make the context switch code more readable */

/* OFFSETOF returns the offset in bytes, and the stores are performed 
 * in words, so this has to be changed to accomodate the word offset.
 */
#define LOC(X) (OFFSETOF(ContextRecord, X)   / SIZEOF(int))
#define GLOC(X) (OFFSETOF(ContextRecord, X)      / SIZEOF(int))
/* Macro to pack the low 8 bits of 8 40-bit S and R registers into a single pair of 32-bit words in Ra:Rb. */
#define GLOM(Sa,Sb,Sc,Sd,Ra,Rb,Rc,Rd) \
    PX = Ra;  \
    Ra = PX1; \
    PX = Rb;  \
    Rb = PX1; \
    Rb = Rb OR LSHIFT Ra BY -8; \
    PX = Rc;  \
    Rc = PX1; \
    PX = Rd;  \
    Rd = PX1; \
    Rd = Rd OR LSHIFT Rc BY -8; \
    Rb = Rb OR LSHIFT Rd BY -16; \
    PX = Sb; \
    Ra = PX1; \
    PX = Sa;  \
    Rc = Px1; \
    Ra = Ra OR LSHIFT Rc BY -8; \
    PX = Sc;  \
    Rc = PX1; \
    PX = Sd;  \
    Rd = PX1; \
    Rd = Rd OR LSHIFT Rc BY -8; \
    Ra = Ra OR LSHIFT Rd BY -16;

/* Macro to unpack the 4x8-bit fields from Rb and load them into Ra-Rd as the low 8-bits of the 40-bit word. */
/* The remaining 32 bits coming from the context record. */

/* This UNGLOM sequence restores all registers supplied. Sa-d and Ra-d. */
#define UNGLOM(Sa, Sb, Sc, Sd, Ra, Rb, Rc, Rd) \
    PX2 = DM(LOC(Sb), I3);    \
    PX1 = Ra;                  \
    Sb  = PX;                  \
    PX2 = DM(LOC(Sa), I3);    \
    Ra  = LSHIFT Ra by 8;      \
    PX1 = Ra;                  \
    Sa  = PX;                  \
    PX2 = DM(LOC(Sd), I3);    \
    Ra  = LSHIFT Ra by 8;      \
    PX1 = Ra;                  \
    Sd  = PX;                  \
    PX2 = DM(LOC(Sc), I3);    \
    Ra  = LSHIFT Ra by 8;      \
    PX1 = Ra;                  \
    Sc  = PX;                  \
    PX2 = DM(LOC(Rb), I3);    \
    Rc  = LSHIFT Rb BY 8;      \
    PX1 = Rb;                  \
    Rb  = PX;                  \
    PX2 = DM(LOC(Ra), I3);    \
    PX1 = Rc;                  \
    Ra  = PX;                  \
    PX2 = DM(LOC(Rd), I3);    \
    Rc  = LSHIFT Rc BY 8;      \
    PX1 = Rc;                  \
    Rd  = PX;                  \
    PX2 = DM(LOC(Rc), I3);    \
    Rc  = LSHIFT Rc BY 8;      \
    PX1 = Rc;                  \
    Rc  = PX;

#define CONTEXT_RECORD_SIZE   (SIZEOF(ContextRecord)  / SIZEOF(int))

#define ALL_DAG_REGS      (BITM_REGF_MODE1_SRD1H    | \
                           BITM_REGF_MODE1_SRD1L    | \
                           BITM_REGF_MODE1_SRD2H    | \
                           BITM_REGF_MODE1_SRD2L)
                                       
#define ALL_DATA_REGS     (BITM_REGF_MODE1_SRRFH    | \
                           BITM_REGF_MODE1_SRRFL)
                                       
#define ALL_DATA_AND_DAGS (ALL_DAG_REGS | ALL_DATA_REGS)
                                       
#define RUN_TIME_MODE1_CLR (BITM_REGF_MODE1_BR0      | \
                           BITM_REGF_MODE1_BR8      | \
                           BITM_REGF_MODE1_SRCU     | \
                           ALL_DATA_AND_DAGS        | \
                           BITM_REGF_MODE1_ALUSAT   | \
                           BITM_REGF_MODE1_SSE      | \
                           BITM_REGF_MODE1_TRUNCATE | \
                           BITM_REGF_MODE1_PEYEN    | \
                           BITM_REGF_MODE1_BDCST9   | \
                           BITM_REGF_MODE1_BDCST1)

#define RUN_TIME_MODE1_SET (BITM_REGF_MODE1_CBUFEN | BITM_REGF_MODE1_RND32)
#define MODE1_INT_EN_BIT (BITM_REGF_MODE1_IRPTEN)

#define CHARSIZEACCESS (bw)
/* Effect latency of some System Registers may be 2 cycles instead of 1 for
 * External data accesses
 */
/* This anomaly is 09000022 for other processors */
#if WA_15000004
#define WA_15000004_SUPPRESS_ENTER .message/suppress 2547;
#define WA_15000004_SUPPRESS_EXIT  .message/pop 2547;
#define WA_15000004_1NOP NOP;
#define WA_15000004_2NOP NOP; NOP;
#else
#define WA_15000004_SUPPRESS_ENTER
#define WA_15000004_SUPPRESS_EXIT
#define WA_15000004_1NOP
#define WA_15000004_2NOP
#endif

/* Specific Core Stall may not execute as expected between PCSTK load and RTS */
/* This anomaly is 07000009/08000001/09000024 for other processors */
#if WA_15000006
#define WA_15000006_1NOP NOP;
#else
#define WA_15000006_1NOP
#endif

/* Anomaly 20-00-0069 : PCSTK and MODE1STK load do not update if next instruction is L2 or L3 access 
 */
#if WA_20000069
#define WA_20000069_1NOP NOP;
#else /* !WA_20000069 */
#define WA_20000069_1NOP
#endif /* !WA_20000069 */

#if defined(__ADI_GENERATED_DEF_HEADERS__)
#define SIMD_ENABLE_BIT BITM_REGF_MODE1_PEYEN
#define RND32_ENABLE_BIT BITM_REGF_MODE1_RND32
#define LOOP_STACK_EMPTY_BIT BITM_REGF_STKYX_LSEM

#else
#define SIMD_ENABLE_BIT PEYEN
#define RND32_ENABLE_BIT RND32
#define LOOP_STACK_EMPTY_BIT LSEM
#endif

.extern VAR_NAME(pxCurrentTCB);

/*
*********************************************************************************************************
*                                         STATIC VARIABLES
*********************************************************************************************************
*/

.SECTION/DM seg_dmda;

/* Anomaly 36-10-0101 : SEC interrupts fails to latch in IRPTL if incoming SECI interrupt aligns with explicit write to IRPTL register
 *
 * The workaround for this anomaly involves using a system interrupt rather than a core interrupt to
 * trigger rescheduling from task level, so there is extra initialization required to configure the
 * interrupt on the System Event Controller (SEC).
 */
#if WA_20000081
/* Global to hold the core-specific system interrupt ID for rescheduling */
.extern	VAR_NAME(_adi_OSRescheduleIntID);
/* This volaile flag variable is used by tasks, to block their progress after raising the reschedule
 * interrupt until the interrupt has been serviced. This is to compensate for the much greater latency
 * of an SEC interrupt compared with a core interrupt.
 */
.extern	VAR_NAME(_adi_OSWaitingForSched);
#endif /* WA_20000081 */


.SECTION/PM seg_pmco;

.global FUNC_NAME(prvPortStartFirstTask);
#if WA_20000081
/* This is an entrypoint for the reschedule interrupt, which is used as a "dispatched handler" by the
 * adi_rtl low-level interrupt APIs.
 */
.global FUNC_NAME(_adi_SoftIntTaskSw);
#endif /* WA_20000081 */

.extern FUNC_NAME(xPortSFT31Handler);
.extern FUNC_NAME(vTaskSwitchContext);

FUNC_NAME(prvPortStartFirstTask):
	
	/* ToDo: reset the system SP to the base of the system stack,
	 * as it will only be used for interrupts and exceptions from this point
	 * so any space currently being used on the stack will be wasted. */
	 
	// We are currently in the system (foreground) data and DAG regs
		
	I0 = DM(VAR_NAME(pxCurrentTCB)); // Get the location of the current TCB into I0'
	
    // Load the stack pointer for the (possibly new) thread into I3'
    I3 = DM(0, I0); // The first item in pxCurrentTCB is the task top of stack

    /* Restore the loop stack (must be empty at this point) */
    /* Setup some variables to restore the loop stack */
    R0 = DM(I3, M6);   
    R0 = R0 + R0;                                      /* R0 = 2 * R0, move load pointer to stack region */
    IF EQ JUMP (PC, .Initial_Loop_Stack_Done) (DB);  /* if stack count is zero then we're done */
        R0 = R0 - 1;                                   /* pre-decrement count, get stack count into M2 */
        R0 = R0 - 1;                                   /* continue pre-decrement, load ptr += (2 * LOOP_STACK_COUNT)(STALL) */

.Initial_Loop_Stack:                             /* top of loop */
    PUSH LOOP;
    IF NE JUMP (PC, .Initial_Loop_Stack)(DB);   /* R0 == 0 on final iteration */
        R0 = R0 - 1, CURLCNTR = DM(I3, M6);             /* post-decrement count, load CURLCNTR from saved context */
        R0 = R0 - 1, LADDR    = DM(I3, M6);             /* cont. post-decrement count, loadLADDR from saved context */

.Initial_Loop_Stack_Done:

	/* Anomaly workaround for 2137x and 214xx. Can't do direct load from memory to
	 * the Loop Registers. If the context area memory bank is also being written to
	 * by the DMA engine, then the loop register values can get corrupted when they
	 * are being restored.
	 * This is anomaly 09000023/15000005 */
    LCNTR = DM(I3, M6);             /* always restore LCNTR (same in SIMD or SISD) */
        
	/* Restore the PC Stack */
    R0 = DM(I3, M6);                    /* get the saved stack count */
    R0 = PASS R0;                       /* test it for zero */
    IF EQ JUMP (PC, .Initial_PCSTK_Done);  /* if stack count is zero then we're done */

.Initial_PCSTK:                  /* top of loop */
    PUSH PCSTK;                         /* current PC goes on stack here... */
    R0 = R0 - 1, PCSTK = DM(I3, M6);    /* ...and gets replaced here */
    IF NE JUMP (PC, .Initial_PCSTK); /* We can't use the delayslots on this. */

.Initial_PCSTK_Done:

	I11 = I3					; // copy context record address to system I11
	CALL .RestoreTaskContext;


	RTS(DB)						;
		POP STS					;
		NOP						;
		NOP						;
END_LABEL(prvPortStartFirstTask):
	
#if WA_20000081
/* If an SEC interrupt is being used for rescheduling, as a workaround for anomaly 20-00-0081,
 * then the core interrupt SFT3 should never be raised. We provide the entrypoint here to avoid
 * link errors, but the hadler itself is just a loop-stop stub.
 */ 
xPortSFT31Handler:
		JUMP xPortSFT31Handler;
       
/* This is an entrypoint for the reschedule interrupt, which is used as a "dispatched handler" by the
 * adi_rtl low-level interrupt APIs so that it can be invoked via the SEC dispatcher.
 */
FUNC_NAME(_adi_SoftIntTaskSw):

      /* Unlike a "raw" core interrupt, this entrypoint is called from the SEC dispatcher, which has
       * already saved some register to the foreground (i.e.e system) stack. In order to mimic the
       * behaviour of a core interrupt, and hence to minimise the differences between using a core
       * interrupt and an SEC interrupt for rescheduling, we restore those registers here. This puts
       * the register state (apart from PC) back as it was at the point where the SEC dispatcher was
       * entered. *Note* that if the SEC dispatcher (__dispatcher_SEC in SHARC/lib/src/libc_src/int_dispatcher_215XX.asm)
       * is ever modified then this restore code may need to to updated also.
       *
       * The low-level dispatched handler is entered via an indirect jump, implemented with RTS,
       * so we don't have to undo the effects of a C-style function call.
       *
       * Interrupts are still disabled in MODE1 at this point.
       */

      PCSTK = DM(14, I7);
      I12 = I7;                      // copy stack pointer to I12
      B6  = DM(2, I7);               // restore saved regs from stack frame
      I6  = DM(3, I7);
      PX2 = DM(6, I7);
      PX1 = DM(7, I7);
      R12 = PX;
      PX2 = DM(8, I7);
      PX1 = DM(9, I7);
      R8 = PX;
      PX2 = DM(10, I7);
      PX1 = DM(11, I7);
      R4 = PX;
      PX2 = DM(12, I7);
      PX1 = DM(13, I7);
      B7 = DM(4, I7);
      I7 = PM(5, I12);
      I12  = PM(1, I12);
      
/* End of copied dispatcher code */
END_LABEL(_adi_SoftIntTaskSw):
#else
/*
 * xPortSFT31Handler:
 *
 * This is the service routine for the kernel interrupt. It is the handler
 * for all rescheduling activity (pre-emptive and non-preeemptive) in the system.
 */
FUNC_NAME(xPortSFT31Handler):
#endif /* WA_20000081 */

	// This is an interrupt so we are running in the foreground (system) registers
	// and the system stack. The user (task) data & DAG register contents - and the
	// user stack pointer - are untouched in the background registers and we can leave
	// them there until we know that a context switch is actually happening.
	//
	// Since this must be the lowest-priority interrupt in the system no other ISR
	// can currently be using the foreground registers. We are therefore free to trash
	// any registers so long as we leave the C runtime intact (i.e. the fixed registers,
	// the stack and frame pointers, and the DAG length registers).
	//
	// While we're in the reschedule interrupt we can select freely between the foreground
	// and backgound register sets, as long as we don't change the background (task) registers
	// until they have been saved. When we return from the interrupt the pop of the status
	// stack will return the foreground/background selection state to the task's setting (i.e.
	// all background regs selected) and if a nested interrupt occurs then the push of the
	// status stack will force the foreground (system) registers to be selected.  
	
	// We enter the handler with interrupts disabled (MODE1->IRPTEN == 0) but we need to
	// re-enable them as early as possible so as not to impact interrupt latency.
	// Self-nesting on SHARC-XI only applies to the SEC interrupt (ToDo: check this) so
	// we don't need to worry about the reschedule interrupt being re-entered if it is
	// raised by a nested peripheral interrupt. Any nested interrupts that occur should
	// be preserving any registers they touch (except for the 40-bit extensions of the system
	// data regs).
	
#if WA_20000081
	  BIT SET MODE1 MODE1_INT_EN_BIT;	
#else
   	  PCSTK = DM(14, I7);
      I12 = I7;                      // copy stack pointer to I12
      B6  = DM(2, I7);               // restore saved regs from stack frame
      I6  = DM(3, I7);
      PX2 = DM(6, I7);
      PX1 = DM(7, I7);
      R12 = PX;
      PX2 = DM(8, I7);
      PX1 = DM(9, I7);
      R8 = PX;
      PX2 = DM(10, I7);
      PX1 = DM(11, I7);
      R4 = PX;
      PX2 = DM(12, I7);
      PX1 = DM(13, I7);
      B7 = DM(4, I7);
      I7 = PM(5, I12);
      I12  = PM(1, I12);
#endif
	
	// There are a number of registers which are defined as scratch but which are not
	// shadowed (i.e. don't have separate foreground/background instances). These registers
	// may be trashed by the call to the C function vTaskSwitchContext(). These are the
	// STKY, PX and UMASK registers. 
	// However, any C function (such as vTaskSwitchContext) will save and restore any preserved
	// registers that it uses, so we can copy the scratch registers to the preserved registers
	// to save them, without needing to write them out to memory.
	//
	// We don't need to save the ASTAT registers here as they were pushed onto the status stack
	// during interrupt entry.
	//
	R3  = STKYx;
	R5  = STKYy;
	R6  = PX1;
	R7  = PX2;
	R9  = USTAT1;
	R10 = USTAT2;
	R11 = USTAT3;
	R13 = USTAT4;

	R14 = DM(VAR_NAME(pxCurrentTCB)); // R14 is a preserved reg	
	
	// Call the rescheduling function. We don't need to do anything to set
	// up the C runtime as it should always be ready set up in the system regs.
	// Note: this assumes that vTaskSwitchContext() doesn't make any use of
	// hardware looping (as reasonable assumption).
	// ToDo: check whether we need to save & reset the loop registers, e.g. for
	// anomaly workarounds.
	ccall(FUNC_NAME(vTaskSwitchContext));
	
	R15 = DM(VAR_NAME(pxCurrentTCB));
	
	// NOTE: this may be a premature optimisation. It might be the case that the reschedule
	// interrupt is only ever (or in the vast majority of cases) entered when FreeRTOS knows
	// that the task needs to change. If so then we would be better not to bother with this
	// test and to go straight to the context switch.
	// 
	COMP(R14, R15); 	       // Compare the old and new "current TCB" pointers
	IF NE JUMP .ContextSwitch; // if different then context-switch
							   // ToDo: move STKY reloads into delay slots here 
	
	// We're not switching context (task), so we just restore the scratch registers
	// (that we copied into the preserved system regs on entry).
	STKYx  = R3;
	STKYy  = R5;
	PX1    = R6;
	PX2    = R7;
	USTAT1 = R9;
	USTAT2 = R10;
	USTAT3 = R11;
	USTAT4 = R13;
	
#if WA_20000081
	/* Clear the "waiting for reschedule" flag, to release any spinning threads */
	R0 = 0;
	DM(VAR_NAME(_adi_OSWaitingForSched)) = R0;

	/* Globally disable interrupts before ending the interrupt on the SEC, to prevent
	 * the reschedule interrupt from being re-entered, until after RTI, below. 
	 */
	JUMP .intDisable2 (DB);
	BIT CLR MODE1 MODE1_INT_EN_BIT;
	R4 = DM(VAR_NAME(_adi_OSRescheduleIntID));
.intDisable2:
      DM(REG_SEC0_END) = R4;         // tell SEC that the interrupt is handled
#endif

#if WA_20000009
      /* If we return from the interrupt to an idle instruction we might hit
      ** anomaly 20000009. So make sure there aren't memory instructions in
      ** the last five cycles of the interrupt dispatcher.
      */
	  NOP;
	  NOP;
	  NOP;
	  NOP;
	  NOP;
#endif
    // We're done, resume the interrupted task by returning from the SFT31 interrupt.
    // This will restore the task's MODE1 register contents (currently on the status stack) and
    // hence will switch to the background (user) data and DAG registers. We use the delayed
    // form of RTI as insurance against effect delays in popping the status stack.
    RTI (DB);
        NOP;   // ToDo: move the last two USTAT reloads into these slots
        NOP;   // Or should we? MODE1 pop will change to background data regs, but when?
        
.ContextSwitch:
	// We are switching to a different task. R14 contains the address of the outgoing TCB and
	// R15 contains the address of the incoming TCB. Also, the task's STKY, PX and USTAT reg
	// contents are in data regs, as above. 
	
	// Switch to the task (background) DAG registers
	bit set MODE1 ALL_DAG_REGS; // select background (task) DAG regs, still in system data regs
	NOP;                                 // effect delay for MODE1 change

    /* We save task data into a C-like structure with each register having at least one word in that structure (see portASM.h).
     * We shift the task stack pointer downwards by the size of the context structure to allocate memory for the context.
     *
     * Note: if the modify instruction decrements I7 below the stack limit (B7) the CB7 interrupt will be raised.  
     */
    MODIFY(I7, - CONTEXT_RECORD_SIZE) (NW); // use NW because CONTEXT_RECORD_SIZE is in words
	// NOPs needed here?
	
    /* Save all registers that are part of the status stack */
    DM(LOC(MODE1), I7) = MODE1STK; /* save MODE1 from the copy on the status stack */
    DM(LOC(STKYx), I7) = R3;
    DM(LOC(STKYy), I7) = R5;
    WA_15000004_SUPPRESS_EXIT // ToDo: find out what this is for
    DM(LOC(PX1), I7) = R6;
    DM(LOC(PX2), I7) = R7;

	// Save the user status registers
    DM(LOC(USTAT1), I7) = R9;
    DM(LOC(USTAT2), I7) = R10;
    DM(LOC(USTAT3), I7) = R11;
    DM(LOC(USTAT4), I7) = R13;
    
	// Switch to the task (background) data registers
	bit set MODE1 ALL_DATA_REGS; // select background (task) data regs, still in task DAG regs
//	NOP;                                 // effect delay for MODE1 change
	
    /* Save the task DAG2 Low registers (8..11) using task I7 */
    DM(LOC(I8),  I7) = I8;  /* I regs */
    DM(LOC(I9),  I7) = I9;
    DM(LOC(I10), I7) = I10;
    DM(LOC(I11), I7) = I11;
	DM(LOC(L8),  I7) = L8;  /* L regs */
    DM(LOC(L9),  I7) = L9;
    DM(LOC(L10), I7) = L10;
    DM(LOC(L11), I7) = L11;
    DM(LOC(B8),  I7) = B8;  /* B regs */
    DM(LOC(B9),  I7) = B9;
    DM(LOC(B10), I7) = B10;
    DM(LOC(B11), I7) = B11;
    DM(LOC(M8),  I7) = M8;  /* M regs */
    DM(LOC(M9),  I7) = M9;
    DM(LOC(M10), I7) = M10;
    DM(LOC(M11), I7) = M11;
    
    /* Save the TASK DAG2 High (12..15) registers */
    DM(LOC(I12), I7) = I12; /* I regs */
    DM(LOC(I13), I7) = I13;
    DM(LOC(I14), I7) = I14;
    DM(LOC(I15), I7) = I15;
    DM(LOC(L12), I7) = L12; /* L regs */
    DM(LOC(L13), I7) = L13;
    DM(LOC(L14), I7) = L14;
    DM(LOC(L15), I7) = L15;
    DM(LOC(B12), I7) = B12; /* B regs */
    DM(LOC(B13), I7) = B13;
    DM(LOC(B14), I7) = B14;
    DM(LOC(B15), I7) = B15;
    DM(LOC(M12), I7) = M12; /* M regs */
    DM(LOC(M13), I7) = M13;
    DM(LOC(M14), I7) = M14;
    DM(LOC(M15), I7) = M15;

#if WA_15000016 // ToDo: does this anomaly affect 215xx?
.NOCOMPRESS;
#endif
	// The task I12 has been saved, above, so we can use it as a copy of I7
	// for saving DAG1 registers
	I12 = I7;
		
    /* Save the task DAG1 Low registers (0..3) using i12 */
    PM(LOC(I0), I12) = I0;  /* I regs */
    PM(LOC(I1), I12) = I1;
    PM(LOC(I2), I12) = I2;
    PM(LOC(I3), I12) = I3;
    PM(LOC(L0), I12) = L0;  /* L regs */
    PM(LOC(L1), I12) = L1;
    PM(LOC(L2), I12) = L2;
    PM(LOC(L3), I12) = L3;
    PM(LOC(B0), I12) = B0;  /* B regs */
    PM(LOC(B1), I12) = B1;
    PM(LOC(B2), I12) = B2;
    PM(LOC(B3), I12) = B3;
    PM(LOC(M0), I12) = M0;  /* M regs */
    PM(LOC(M1), I12) = M1;
    PM(LOC(M2), I12) = M2;
    PM(LOC(M3), I12) = M3;

    /* Save the TASK DAG1 High (4..7) registers */
    PM(LOC(I4), I12) = I4;  /* I regs */
    PM(LOC(I5), I12) = I5;
    PM(LOC(I6), I12) = I6;
 // PM(LOC(I7), I12) = I7;      /* We do not need to save I7 here, as the stack top and will be saved into the TCB */
    PM(LOC(L4), I12) = L4;  /* L regs */
    PM(LOC(L5), I12) = L5;
    PM(LOC(L6), I12) = L6;
    PM(LOC(L7), I12) = L7;
    PM(LOC(B4), I12) = B4;  /* B regs */
    PM(LOC(B5), I12) = B5;
    PM(LOC(B6), I12) = B6;
    PM(LOC(B7), I12) = B7;
    PM(LOC(M4), I12) = M4;  /* M regs */
    PM(LOC(M5), I12) = M5;
    PM(LOC(M6), I12) = M6;
    PM(LOC(M7), I12) = M7;

#if defined(WA_15000016)
.COMPRESS;
#endif   

	/* Save the task data registers. We already switched to the background (task) data regs above */
		
	/* Save the task (secondary) data registers */
    DM(LOC(R0),  I7) = R0;
    DM(LOC(R1),  I7) = R1;
    DM(LOC(R2),  I7) = R2;
    DM(LOC(R3),  I7) = R3;
    DM(LOC(R4),  I7) = R4;
    DM(LOC(R5),  I7) = R5;
    DM(LOC(R6),  I7) = R6;
    DM(LOC(R7),  I7) = R7;
    DM(LOC(R8),  I7) = R8;
    DM(LOC(R9),  I7) = R9;
    DM(LOC(R10), I7) = R10;
    DM(LOC(R11), I7) = R11;
    DM(LOC(R12), I7) = R12;
    DM(LOC(R13), I7) = R13;
    DM(LOC(R14), I7) = R14;
    DM(LOC(R15), I7) = R15; 
    DM(LOC(S0),  I7) = S0;
    DM(LOC(S1),  I7) = S1;
    DM(LOC(S2),  I7) = S2;
    DM(LOC(S3),  I7) = S3;
    DM(LOC(S4),  I7) = S4;
    DM(LOC(S5),  I7) = S5;
    DM(LOC(S6),  I7) = S6;
    DM(LOC(S7),  I7) = S7;
    DM(LOC(S8),  I7) = S8;
    DM(LOC(S9),  I7) = S9;
    DM(LOC(S10), I7) = S10;
    DM(LOC(S11), I7) = S11;
    DM(LOC(S12), I7) = S12;
    DM(LOC(S13), I7) = S13;
    DM(LOC(S14), I7) = S14;
    DM(LOC(S15), I7) = S15;
        
#if 1
    /* IF RND32 is set then do not save the full 40 bits of the data registers */
    BIT TST MODE1STK BITM_REGF_MODE1_RND32;
    IF TF JUMP .NO_40bit_save;
#endif
    GLOM(S0, S1, S2, S3, R0, R1, R2, R3);      /* Pack and store the low bits of the S and R registers, in batches of 8. */
    DM(GLOC(R0_3), I7) = R0;                    /* R0 = Low 8 bits of the R0 - R3 registers */
    DM(GLOC(S0_3), I7) = R1;                    /* R1 = Low 8 bits of the S0 - S3 registers */

    GLOM(S4, S5, S6, S7, R4, R5, R6, R7);
    DM(GLOC(R4_7), I7) = R4;
    DM(GLOC(S4_7), I7) = R5;

    GLOM(S8, S9, S10, S11, R8, R9, R10, R11);
    DM(GLOC(R8_11), I7) = R8;
    DM(GLOC(S8_11), I7) = R9;

    GLOM(S12, S13, S14, S15, R12, R13, R14, R15);
	DM(GLOC(R12_15), I7) = R12;
	DM(GLOC(S12_15), I7) = R13;

.NO_40bit_save:
    
	// Switch back to the system (foreground) data registers
	bit clr MODE1 ALL_DATA_REGS; // select foreground (system) data regs, still in task DAG regs
//	NOP;                                 // effect delay for MODE1 change
	
    /* Save the multiplier result registers, note that we
     * save both foreground and background sets (unlike
     * the general and DAG registers where we don't save
     * the alternates) and that we both save and restore
     * these with the SRCU bit clear.
     *
     * The register move is being done in SIMD mode so both PEx and PEy
     * registers are being moved at the same time. This means that the
     * instruction R0 = MR0F places MR0F in R0 and MS0F in S0. The store into
     * memory is done in SISD mode because we cannot rely on the memory
     * location being in internal memory and some SHARC processors cannot do
     * SIMD memory accesses to external memory. */

    BIT SET MODE1 SIMD_ENABLE_BIT;    /* Save the multiplier registers. The only way is
                                       to use SIMD because the PEy registers cannot be
                                       used directly */
    NOP;
    R0 = MR0F;
    R1 = MR1F;
    R2 = MR2F;
    R3 = MR0B;
    R4 = MR1B;
    R5 = MR2B;
    BIT CLR MODE1 SIMD_ENABLE_BIT;    /* Back to SISD to do the actual stores to memory */
    NOP;
    WA_15000004_1NOP

    DM(LOC(MS0F), I7)   = S0;
    DM(LOC(MS1F), I7)   = S1;
    DM(LOC(MS2F), I7)   = S2;
    DM(LOC(MS0B), I7)   = S3;
    DM(LOC(MS1B), I7)   = S4;
    DM(LOC(MS2B), I7)   = S5;

    DM(LOC(MR0F), I7)   = R0;
    DM(LOC(MR1F), I7)   = R1;
    DM(LOC(MR2F), I7)   = R2;
    DM(LOC(MR0B), I7)   = R3;
    DM(LOC(MR1B), I7)   = R4;
    DM(LOC(MR2B), I7)   = R5;        
     
    R0 = BFFWRP;                                       /* Save the bit fifo */
    BFFWRP = 64;
    R1 = BITEXT 32;
    R2 = BITEXT 32;
    DM(LOC(BitFIFOWRP), I7) = R0;
    DM(LOC(BitFIFO_0),  I7) = R1;
    DM(LOC(BitFIFO_1),  I7) = R2;

	// Save the top of the status stack. We assume that only one entry in the status stack
	// is in use, since it should only be used by interrupt entry & exit and since the
	// reschedule interrupt should be the lowest-priority interrupt in the system.
	//
	// We have to ensure that MODE1 doesn't change when we pop the status stack, so we
	// replace the stacked MODE1 value with the current value.
	MODE1STK = MODE1; // MODE1STK has already been saved to the context record, above
    POP STS;                // no effect delay (ToDo: check this)
	DM(LOC(ASTATx), I7) = ASTATX;	
	DM(LOC(ASTATy), I7) = ASTATY;	

    /* The remaining entries in the PCSTK are entries that are being used by the task, and
     * should be saved as part of the task context. */
    MODIFY(I7, -1) (NW);
    
    R0 = PCSTKP;
    JUMP (PC, .SaveContext_PCSTK_tst)(DB);          /* safe wrt. SWF */
        R1 = R0;
        R0 = PASS R0;                         
        
.SaveContext_PCSTK:
    R0 = R0 - 1, DM(I7, M7) = PCSTK;                   /* There is *no* automatic pop of the PCSTK with a read */
    POP PCSTK;
.SaveContext_PCSTK_tst:
    IF NE JUMP (PC, .SaveContext_PCSTK);            /* can't use the delay slots on this */

    DM(I7, M7) = R1;  // SAVE THE PCSTKP
     
    R0 = R0 - R0;									   /* Setup to save the Loop stack */
    DM(I7, M7) = LCNTR;                                /* Always save LCNTR */

.SaveContext_SAVE_LOOP_STACK:
    BIT TST STKY LOOP_STACK_EMPTY_BIT;
    IF TF JUMP (PC, .SaveContext_DONE_LOOP_STACK);
    R0 = R0 + 1, DM(I7, M7) = LADDR;                   /* Increment the count & save top of loop address stack */
    JUMP (PC, .SaveContext_SAVE_LOOP_STACK) (DB);      /* safe wrt. SWF */
        DM(I7, M7) = CURLCNTR;                         /* save top of loop counter stack */
        POP LOOP;                                      /* Pop the loop stack */
.SaveContext_DONE_LOOP_STACK:
    DM(0, I7) = R0;                                    /* save the stack count off, safe wrt. SWF */

	 
	// Store the thread stack pointer in the outgoing thread structure
	// We need to be in the system (foreground) data regs for R14
    R0 = I7;                         // copy I7 to R0 so we can save it through I0
    I0 = R14;                        // address of outgoing TCB
    DM(0, I0) = R0; // Save the new top of stack into the first member of the TCB

#if WA_20000081
	/* Clear the "waiting for reschedule" flag, to release any spinning threads */
	R0 = 0;
	DM(VAR_NAME(_adi_OSWaitingForSched)) = R0;
#else /* WA_20000081 */
	WA_20000069_1NOP                        /* workaround for anomaly 20-00-0069 */   
#endif /* !WA_20000081 */
	
/*************************************************
 *
 *  Context restore
 *
 *************************************************/
	// We are currently in the system (foreground) data regs and the task (background) DAG regs
	// Switch back to the system (foreground) data registers
	bit clr MODE1 ALL_DAG_REGS; // select foreground (system) DAG regs
	NOP;                                 // effect delay for MODE1 change
		
	I0 = DM(VAR_NAME(pxCurrentTCB)); // Get the location of the current TCB into I0'
	
    // Load the stack pointer for the (possibly new) thread into I3'
    I3 = DM(0, I0); // The first item in pxCurrentTCB is the task top of stack

    /* Restore the loop stack (must be empty at this point) */
    /* Setup some variables to restore the loop stack */
    R0 = DM(I3, M6);   
    R0 = R0 + R0;                                      /* R0 = 2 * R0, move load pointer to stack region */
    IF EQ JUMP (PC, .Restore_Loop_Stack_Done) (DB);  /* if stack count is zero then we're done */
        R0 = R0 - 1;                                   /* pre-decrement count, get stack count into M2 */
        R0 = R0 - 1;                                   /* continue pre-decrement, load ptr += (2 * LOOP_STACK_COUNT)(STALL) */

.Restore_Loop_Stack:                             /* top of loop */
    PUSH LOOP;
    IF NE JUMP (PC, .Restore_Loop_Stack)(DB);   /* R0 == 0 on final iteration */
        R0 = R0 - 1, CURLCNTR = DM(I3, M6);             /* post-decrement count, load CURLCNTR from saved context */
        R0 = R0 - 1, LADDR    = DM(I3, M6);             /* cont. post-decrement count, loadLADDR from saved context */

.Restore_Loop_Stack_Done:

	/* Anomaly workaround for 2137x and 214xx. Can't do direct load from memory to
	 * the Loop Registers. If the context area memory bank is also being written to
	 * by the DMA engine, then the loop register values can get corrupted when they
	 * are being restored.
	 * This is anomaly 09000023/15000005 */
    LCNTR = DM(I3, M6);             /* always restore LCNTR (same in SIMD or SISD) */
        
	/* Restore the PC Stack */
    R0 = DM(I3, M6);                    /* get the saved stack count */
    R0 = PASS R0;                       /* test it for zero */
    IF EQ JUMP (PC, .Restore_PCSTK_Done);  /* if stack count is zero then we're done */

.Restore_PCSTK:                  /* top of loop */
    PUSH PCSTK;                         /* current PC goes on stack here... */
    R0 = R0 - 1, PCSTK = DM(I3, M6);    /* ...and gets replaced here */
    IF NE JUMP (PC, .Restore_PCSTK); /* We can't use the delayslots on this. */

.Restore_PCSTK_Done:
	I11 = I3					; // copy context record address to system I11
		call .RestoreTaskContext	;

#if WA_20000081
	/* Globally disable interrupts before ending the interrupt on the SEC, to prevent
	 * the reschedule interrupt from being re-entered, until after RTI, below. 
	 */
	  JUMP .intDisable1 (DB);
	  BIT CLR MODE1 MODE1_INT_EN_BIT;
	  R4 = DM(VAR_NAME(_adi_OSRescheduleIntID));
.intDisable1:
      DM(REG_SEC0_END) = R4;         // tell SEC that the interrupt is handled
#endif

#if WA_20000009
      /* If we return from the interrupt to an idle instruction we might hit
      ** anomaly 20000009. So make sure there aren't memory instructions in
      ** the last five cycles of the interrupt dispatcher.
      */
	  NOP;
	  NOP;
	  NOP;
	  NOP;
	  NOP;
#endif
    // We're done, resume the switched-in task by returning from the interrupt.
    // This will restore the task's MODE1 register contents (currently on the status stack) and
    // hence will switch to the background (user) data and DAG registers. We use the delayed
    // form of RTI as insurance against effect delays in popping the status stack.
    RTI (DB);
        NOP;
        NOP;
        NOP;
END_LABEL(xPortSFT31Handler):

		
.RestoreTaskContext:
		/* Start restoring the preserved regs, if needed. */

    /* We restore the multiplier result registers now, before MODE1
     * has been restored, i.e. while the SRCU bit is still clear.
     * ASTAT is restored later, so it doesn't matter that loading
     * the MR registers will clear the M* flag bits.
     * Because the PEy multiplier registers cannot be accessed directly, we
     * need to do the restoring in SIMD but in order not to force the stack to
     * be in internal memory we copy from memory in non-SIMD mode.
     */
    S5 = DM(LOC(MS2B), I3);
    R5 = DM(LOC(MR2B), I3);
    S4 = DM(LOC(MS1B), I3);
    R4 = DM(LOC(MR1B), I3);
    S3 = DM(LOC(MS0B), I3);
    R3 = DM(LOC(MR0B), I3);
    S2 = DM(LOC(MS2F), I3);
    R2 = DM(LOC(MR2F), I3);
    S1 = DM(LOC(MS1F), I3);
    R1 = DM(LOC(MR1F), I3);
    S0 = DM(LOC(MS0F), I3);
    R0 = DM(LOC(MR0F), I3);

    BIT SET MODE1 SIMD_ENABLE_BIT;    /* Restore the multiplier registers. The only way is
                                         to use SIMD because the PEy registers cannot be
                                         used directly */
    NOP;
    MR0F = R0;
    MR1F = R1;
    MR2F = R2;
    MR0B = R3;
    MR1B = R4;
    MR2B = R5;

    BIT CLR MODE1 SIMD_ENABLE_BIT;
    NOP;
  
	/* Select the backgound (task) DAG 2 low & high registers */
	BIT SET MODE1 (BITM_REGF_MODE1_SRD2L | BITM_REGF_MODE1_SRD2H) ;
	NOP; // effect delay
	
	/* DAG2 High registers, load using system I3 */	
	B15  = DM(LOC(B15), I3);
	B14  = DM(LOC(B14), I3);
    B13  = DM(LOC(B13), I3);
    B12  = DM(LOC(B12), I3);
    L15  = DM(LOC(L15), I3);
    L14  = DM(LOC(L14), I3);
    L13  = DM(LOC(L13), I3);
    L12  = DM(LOC(L12), I3);
	M15  = DM(LOC(M15), I3);
	M14  = DM(LOC(M14), I3);
	M13  = DM(LOC(M13), I3);
	M12  = DM(LOC(M12), I3);
	I15  = DM(LOC(I15), I3);
	I14  = DM(LOC(I14), I3);
    I13  = DM(LOC(I13), I3);
    I12  = DM(LOC(I12), I3);
    
	/* DAG2 Low registers, load using system I3 */	
	B11  = DM(LOC(B11), I3);
	B10  = DM(LOC(B10), I3);
	B9   = DM(LOC(B9),  I3);
	B8   = DM(LOC(B8),  I3);
    L11  = DM(LOC(L11), I3);
    L10  = DM(LOC(L10), I3);
    L9   = DM(LOC(L9),  I3);
    L8   = DM(LOC(L8),  I3);
	M11  = DM(LOC(M11), I3);
	M10  = DM(LOC(M10), I3);
	M9   = DM(LOC(M9),  I3);
	M8   = DM(LOC(M8),  I3);
	I11  = DM(LOC(I11), I3);
	I10  = DM(LOC(I10), I3);
	I9   = DM(LOC(I9),  I3);
	I8   = DM(LOC(I8),  I3);
	
	/* Select the foregound (system) DAG 2 low & high registers */
	BIT CLR MODE1 (BITM_REGF_MODE1_SRD2L | BITM_REGF_MODE1_SRD2H) ;
	NOP; // effect delay
	I11 = I3					; // copy I7' to I11

	/* Select the backgound (task) DAG 1 low & high registers */
	BIT SET MODE1 (BITM_REGF_MODE1_SRD1L | BITM_REGF_MODE1_SRD1H) ;
	NOP; // effect delay
	
	/* DAG1 High registers, load using system I11 */	
    B7   = PM(LOC(B7), I11);       /* clobbers I7' */
    B6   = PM(LOC(B6), I11);
	B5   = PM(LOC(B5), I11);
    B4   = PM(LOC(B4), I11);
    L7   = PM(LOC(L7), I11);
    L6   = PM(LOC(L6), I11);
    L5   = PM(LOC(L5), I11);
    L4   = PM(LOC(L4), I11);
    M7   = PM(LOC(M7), I11);
    M6   = PM(LOC(M6), I11);
    M5   = PM(LOC(M5), I11);
    M4   = PM(LOC(M4), I11);
    I6   = PM(LOC(I6), I11);
    I7   = I11;                   // copy I11 to I7'
    /* Deallocate scratch record - we're still using the context structure              */
    MODIFY(I7, CONTEXT_RECORD_SIZE) (NW); /* will raise CB7I if stack underflows */

	I5   = PM(LOC(I5), I11);
    I4   = PM(LOC(I4), I11);
    
	/* DAG1 Low registers, load using system I11 */	
	B3   = PM(LOC(B3), I11);
	B2   = PM(LOC(B2), I11);
	B1   = PM(LOC(B1), I11);
	B0   = PM(LOC(B0), I11);      
    L3   = PM(LOC(L3), I11);
    L2   = PM(LOC(L2), I11);
    L1   = PM(LOC(L1), I11);
    L0   = PM(LOC(L0), I11);   
	M3   = PM(LOC(M3), I11);
	M2   = PM(LOC(M2), I11);
	M1   = PM(LOC(M1), I11);
	M0   = PM(LOC(M0), I11);      
	I3   = PM(LOC(I3), I11);
	I2   = PM(LOC(I2), I11);
	I1   = PM(LOC(I1), I11);
	I0   = PM(LOC(I0), I11);     
	
	/* Select the foregound (system) DAG 1 low & high registers */
	BIT CLR MODE1 (BITM_REGF_MODE1_SRD1L | BITM_REGF_MODE1_SRD1H) ;
	NOP; // effect delay
	I3 = I11;
	
    /* Restore the bit fifo */
    R2 = DM(LOC(BitFIFO_1),  I3);
    R1 = DM(LOC(BitFIFO_0),  I3);
    R0 = DM(LOC(BitFIFOWRP), I3);                        
    
    BFFWRP = 0;
    BITDEP R2 BY 32;
    BITDEP R1 BY 32;
    BFFWRP = R0;
			 
.RESTORE_DATA_REGISTERS:

	/* Select the backgound (task) data low & high registers */
	BIT SET MODE1 ALL_DATA_REGS	;
	NOP; // effect delay
	
    USTAT1 = DM(LOC(MODE1), I3);
    BIT TST USTAT1 RND32_ENABLE_BIT;
    IF TF JUMP .ContextRestore_32bit_Only;
    

    R0 = DM(GLOC(R0_3), I3);
    R1 = DM(GLOC(S0_3), I3);
    UNGLOM(S0, S1, S2, S3, R0, R1, R2, R3);

    R4 = DM(GLOC(R4_7), I3);
    R5 = DM(GLOC(S4_7), I3);
    UNGLOM(S4, S5, S6, S7, R4, R5, R6, R7);

    R8 = DM(GLOC(R8_11), I3);
    R9 = DM(GLOC(S8_11), I3);
    UNGLOM(S8, S9, S10, S11, R8, R9, R10, R11);

    R12 = DM(GLOC(R12_15), I3);
    R13 = DM(GLOC(S12_15), I3);
    UNGLOM(S12, S13, S14, S15, R12, R13, R14, R15);

    /* All data registers are now restored - except R4, R8 and R12. */
    JUMP .RestoreContext_DReg_Done;

.ContextRestore_32bit_Only:
    R15 = DM(LOC(R15), I3);
    R14 = DM(LOC(R14), I3);
    R13 = DM(LOC(R13), I3);
    R12 = DM(LOC(R12), I3);
    R11 = DM(LOC(R11), I3);
    R10 = DM(LOC(R10), I3);
    R9  = DM(LOC(R9),  I3);
    R8  = DM(LOC(R8),  I3);
    R7  = DM(LOC(R7),  I3);
    R6  = DM(LOC(R6),  I3);
    R5  = DM(LOC(R5),  I3);
    R4  = DM(LOC(R4),  I3);    
    R3  = DM(LOC(R3),  I3);    
    R2  = DM(LOC(R2),  I3);
    R1  = DM(LOC(R1),  I3);
    R0  = DM(LOC(R0),  I3);             /* Save off the primary ALU registers */

    S15 = DM(LOC(S15), I3);
    S14 = DM(LOC(S14), I3);
    S13 = DM(LOC(S13), I3);
    S12 = DM(LOC(S12), I3);
    S11 = DM(LOC(S11), I3);
    S10 = DM(LOC(S10), I3);
    S9  = DM(LOC(S9),  I3);
    S8  = DM(LOC(S8),  I3);
    S7  = DM(LOC(S7),  I3);
    S6  = DM(LOC(S6),  I3);
    S5  = DM(LOC(S5),  I3);
    S4  = DM(LOC(S4),  I3);
    S3  = DM(LOC(S3),  I3);
    S2  = DM(LOC(S2),  I3);
    S1  = DM(LOC(S1),  I3);
    S0  = DM(LOC(S0),  I3);
    

.RestoreContext_DReg_Done:
      
	// Restore the user status registers
    USTAT4 = DM(LOC(USTAT4), I3);
    USTAT3 = DM(LOC(USTAT3), I3);
    USTAT2 = DM(LOC(USTAT2), I3);
    USTAT1 = DM(LOC(USTAT1), I3);

	// Restore the status registers, via the status stack
    ASTATy = DM(LOC(ASTATy), I3);       /* Restore ASTATx and ASTATy. */
    ASTATx = DM(LOC(ASTATx), I3);
    STKYy  = DM(LOC(STKYy),  I3);        /* Restore the STKY flags, nothing that we do from here on out should change these */
    STKYx   = DM(LOC(STKYx), I3);         

	// Push the status stack so we can set its top, ready for RTI
    PUSH STS;                            // selects system data & DAG registers as side-effect

	// Note: We don't need to worry about the self-nesting flag in MODE1 as long as we're using a core interrupt (SFT3I) as the
	// reschedule interrupt . This may change if/when we're using an SEC interrupt as an anomaly workaround.
    //
    MODE1STK = DM(LOC(MODE1), I3);                   /* MODE1 has not been restored, 215xx allows access to the top of the STS         */
    WA_20000069_1NOP                     /* workaround for anomaly 20-00-0069 */

	RTS						 ;
.RestoreTaskContext.end:
