/*
** ADSP-SC589 startup code generated on Aug 23, 2021 at 14:10:08.
*/
/*
** Copyright (C) 2000-2019 Analog Devices Inc., All Rights Reserved.
**
** This file is generated automatically based upon the options selected
** in the System Configuration utility. Changes to the Startup Code configuration
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

.FILE_ATTR libGroup="startup";
.FILE_ATTR libName="libc";

.FILE_ATTR libFunc="___lib_prog_term";
.FILE_ATTR FuncName="___lib_prog_term";
.FILE_ATTR libFunc="start";
.FILE_ATTR FuncName="start";
.FILE_ATTR prefersMem="any";
.FILE_ATTR prefersMemNum="50";

#include <sys/anomaly_macros_rtl.h> // defines silicon anomaly macros
#include <interrupt.h>              // defines interrupt support
#include <platform_include.h>       // defines MMR macros
#include <adi_osal.h>               // OSAL support
#include <sys/fatal_error_code.h>   // defines fatal error support

/* End marker for the list of static constructors.
** seg_ctdml has to be mapped right after seg_ctdm.
*/

.SECTION/DATA/DOUBLEANY seg_ctdml;
.RETAIN_NAME __ctor_NULL_marker;
.GLOBAL __ctor_NULL_marker;
.VAR __ctor_NULL_marker = 0;

/* The .gdt (global dispatch table) data is used by the C++ exception
** runtime library support to determine which area of code a particular
** exception belongs. These .gdt sections must be mapped to contiguous
** memory by the LDF starting with this one and and ending with .gdtl.
** The word addressed variants of these sections also need to be included
** and are called .gdt32 and .gdtl32.
*/

.SECTION/DOUBLEANY .gdt;
.ALIGN 4;
.BYTE __eh_gdt.[4];
.GLOBAL __eh_gdt.;
.TYPE __eh_gdt.,STT_OBJECT;

.SECTION/DM/DOUBLEANY .gdt32;
.VAR ___eh_gdt[1];
.GLOBAL ___eh_gdt;
.TYPE ___eh_gdt,STT_OBJECT;

/* Suppress the assemblers informational message regarding the automatic
** call sequence change it does when building -swc.
*/
.MESSAGE/SUPPRESS 2536;


.GLOBAL start;

.SECTION/CODE/DOUBLEANY seg_pmco;

start:

      /* Ensure caches are disabled - enables a soft reset to jump to start
      ** and also avoids problems if a preload enables caches.
      */
      /* Cache configuration updates only while executing in L1 memory. */
      R0 = 0;
      JUMP (.disable_caches);
.SECTION/SW/DOUBLEANY seg_int_code;
.disable_caches:
      DM(REG_SHL1C0_CFG) = R0;
      /* 12 uncompressed NOPs after a cache MMR access. */
      .NOCOMPRESS;
      NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
      .COMPRESS;
      JUMP (.caches_disabled);
.PREVIOUS;
.caches_disabled:

      /* Enable instruction, data and system transfer parity checking for
      ** parts and revisions that have all required support. The ADSP-SC58x
      ** and ADSP-2158x revisions 0.0 and 0.1 system reset does not work and
      ** this is required so we don't enable parity for these.
      */
#if !defined(__ADSPSC589_FAMILY__) || \
    (!defined(__SILICON_REVISION__) || \
      ((__SILICON_REVISION__ >= 0x0100) && (__SILICON_REVISION__ != 0xFFFF)))
      BIT SET MODE1 (BITM_REGF_MODE1_IPERREN | BITM_REGF_MODE1_DPERREN |
                     BITM_REGF_MODE1_SPERREN );
      /* Wait 11 cycles for the change to take effect. */
      NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
      #define ENABLED_PARITY_ERROR_CHECKING
#else
      /* Explicitly disable parity error checking when we don't enable it
      ** for the application. This may be necessary for ADSP-215xx (non-ARM)
      ** parts as the SHARC+ boot can enable parity error checking.
      */
      BIT CLR MODE1 (BITM_REGF_MODE1_IPERREN | BITM_REGF_MODE1_DPERREN |
                     BITM_REGF_MODE1_SPERREN );
      /* Wait 11 cycles for the change to take effect. */
      NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
#endif

      R0 = DM(REG_SHBTB0_CFG);              /* Get current BTB configuration. */
      R0 = BCLR R0 BY BITP_SHBTB_CFG_DIS;   /* Enable the BTB                 */
      R0 = BSET R0 BY BITP_SHBTB_CFG_INVAL; /* Invalidate the BTB             */
      /* Turn off software return prediction in the BTB, because we can't
      ** guarantee i12 has a valid code address, and a speculative access
      ** to an invalid address can cause a hardware error.
      */
      R0 = BSET R0 BY BITP_SHBTB_CFG_SRETDIS; /* Software return disable.     */
      DM(REG_SHBTB0_CFG) = R0;              /* Write back to BTB_CFG MMR.     */

      /* Wait 11 cycles for the invalidation of the BTB to take effect. */
      NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;

     /* There might be entries on the loop, PC and status stacks. Clear them all. 
     ** First, clear the loop stack.
     */
     BIT TST STKY BITM_REGF_STKYX_LSEM;
.pop_loop_stack:
     IF TF JUMP (PC, .loop_stack_empty);
     JUMP (PC, .pop_loop_stack) (DB);
       POP LOOP;
       BIT TST STKY BITM_REGF_STKYX_LSEM;
.loop_stack_empty:

     /* Then, clear the status stack. */
     BIT TST STKY BITM_REGF_STKYX_SSEM;
.pop_status_stack:
     IF TF JUMP (PC, .status_stack_empty);
     JUMP (PC, .pop_status_stack) (DB);
       POP STS;
       BIT TST STKY BITM_REGF_STKYX_SSEM;
.status_stack_empty:

     /* Then clear the PC stack. PC stack cannot be popped in a delay slot. */
.pop_pc_stack:
     BIT TST STKY BITM_REGF_STKYX_PCEM;
     IF TF JUMP (PC, .pc_stack_empty);
     POP PCSTK;
     JUMP (PC, .pop_pc_stack);
.pc_stack_empty:

      /* Set the vector register to 'start' so that if the processor
      ** is reset (for example, when the ARM calls adi_core_enable for the
      ** SHARCs), the processor bypasses the boot ROM code.
      */
      R0 = start;
      DM(REG_RCU0_SVECT1) = R0;

.WAITLOOP:
      // Wait for this core to be enabled.
      R0 = DM(REG_RCU0_MSG);
      BTST R0 BY BITP_RCU_MSG_C1ACTIVATE;
      IF SZ JUMP .WAITLOOP;


/*$VDSG<insert-code-very-beginning>                             */
.start_of_user_code_very_beginning:
      /*
      ** Insert any additional code to be executed before any other
      ** startup code here.
      ** Code inserted here is preserved when this file is re-generated.
      ** Note that the C runtime environment hasn't been set up yet
      ** so you cannot call C or C++ functions.
      */
.end_of_user_code_very_beginning:
/*$VDSG<insert-code-very-beginning>                             */

      /*
      ** Initializes the processor, memory, C runtime and heaps.
      */
      .EXTERN ___lib_setup_c;
      CALL ___lib_setup_c;


/*$VDSG<insert-code-after-setup>                                */
.start_of_user_code_after_setup:
      /*
      ** Insert any additional code to be executed at the start after
      ** the C runtime and hardware has been initialized.
      ** Code inserted here is preserved when this file is re-generated.
      */
.end_of_user_code_after_setup:
/*$VDSG<insert-code-after-setup>                                */

      /*
      ** Enable the ILOPI interrupt to support illegal opcode detection.
      ** The interrupt vector code for Illegal Opcode detection
      ** jumps directly to stub handler _adi_ilop_detected.
      */
      .EXTERN adi_rtl_activate_dispatched_handler.;
      R4 = ADI_CID_ILOPI;
      CJUMP adi_rtl_activate_dispatched_handler. (DB);
         DM(I7, M7) = R2;
         DM(I7, M7) = PC;

#if defined(ENABLED_PARITY_ERROR_CHECKING)
      /*
      ** Enable the PARI interrupt to support L1 parity error detection.
      ** The interrupt vector code for this interrupt jumps directly
      ** to stub handler _adi_parity_error_detected.
      */
      .EXTERN adi_rtl_activate_dispatched_handler.;
      R4 = ADI_CID_PARI;
      CJUMP adi_rtl_activate_dispatched_handler. (DB);
         DM(I7, M7) = R2;
         DM(I7, M7) = PC;
#endif

      /*
      ** Call the OSAL init function.
      */
      .EXTERN adi_osal_Init.;    // ADI_OSAL_STATUS adi_osal_Init(void);
      CJUMP adi_osal_Init. (DB);
         DM(I7, M7) = R2;
         DM(I7, M7) = PC;

      R1 = E_ADI_OSAL_SUCCESS;
      COMPU(R0, R1);
      IF NE JUMP .osal_Init_failed;

      /*
      ** Call all the byte-addressed then word-addressed C++ static
      ** class instance constructors.
      */
      .EXTERN  __ctors, _ctors.; // defined in the LDF file

      I0 = _ctors.;              // load pointer to the list of constructors
                                 // for byte addressing into call preserved I0

      R15 = 2;                   // counter held in R15
      R5 = I6;                   // hold current I6 in preserved register R5

.call_ctors_start:
      R0 = DM(I0, M6);           // get the address of the first constructor
      R0 = PASS R0, I13 = R0;    // check if it's the NULL list terminator
.call_ctors:
      IF EQ JUMP (PC, .call_ctors_next); // if NULL we found the list end

      I6 = I7;
      JUMP (M13, I13) (DB);      // call the constructor
         DM(I7, M7) = R5;
         DM(I7, M7) = PC;
      JUMP (PC, .call_ctors) (DB);
         R0 = DM(I0, M6);        // get the address of the next constructor
         R0 = PASS R0, I13 = R0; // check if it's the NULL list terminator
.call_ctors_next:
      R15 = R15 - 1;             // decrement counter
      If EQ JUMP (PC, .call_ctors_exit);

      I0 = __ctors;              // word-addressing constructors list

      I7 = B2W(I7);
      B7 = B2W(B7);
      JUMP (PC, .call_ctors_start) (DB);
         I6 = B2W(I6);
         B6 = B2W(B6);
.call_ctors_exit:

      /*
      ** Transform back to byte-addressed frame.
      */
      I7 = W2B(I7);
      B7 = W2B(B7);
      I6 = W2B(I6);
      B6 = W2B(B6);

      R4 = R4 - R4,              // argc = 0
         R8 = M5;                // argv = NULL


/*$VDSG<insert-code-before-call-to-main>                        */
.start_of_user_code1:
      /*
      ** Insert any additional code to be executed before calling main() here.
      ** Code inserted here is preserved when this file is re-generated.
      ** Avoid clobbering the values of registers R4 and R8 as they hold
      ** the argc and argv parameters for main().
      */
.end_of_start_of_user_code1:
/*$VDSG<insert-code-before-call-to-main>                        */

      .EXTERN main.;
      JUMP main. (DB);           // call main()
         DM(I7, M7) = 0;         // NULL FP to terminate call stack unwinding
         DM(I7, M7) = PC;

      .GLOBAL ___lib_prog_term;
___lib_prog_term:
#if WA_20000009
      /* If there's a memory access in the previous 5 cycles, we might hit
      ** the 20000009 anomaly. However, it's unlikely, because:
      ** - we should only jump here once, so the jump won't be predicted
      ** - the jumps to here from library code don't have delay slots.
      ** So suppress the anomaly warning.
      */
      .MESSAGE/SUPPRESS 2561 FOR 2 LINES;
#endif
      IDLE;
      JUMP ___lib_prog_term;     // Stay put.
.___lib_prog_term.end:

      /*
      ** The call to _adi_osal_Init returned an error so call _adi_fatal_error.
      */
      .EXTERN adi_fatal_error.;
.osal_Init_failed:
      R12 = R0;                  // pass adi_osal_Init result value
      JUMP adi_fatal_error. (DB); // doesn't return
         R4 = _AFE_G_LibraryError;
         R8 = _AFE_S_adi_osal_Init_failure;
.osal_Init_failed.end:

.start.end:

