/*
** ADSP-SC589 app_IVT.s generated on Aug 23, 2021 at 14:10:08
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
** If you need to modify this file, select "Permit alteration of Interrupt
** Table Vector" from "Startup Code/LDF: Advanced Options" in the System
** Configuration utility. This encloses the whole file within a user
** modifiable section and any modification made will be preserved.
*/

.FILE_ATTR libGroup="startup";
.FILE_ATTR libName="libc";

#include <sys/anomaly_macros_rtl.h> // defines silicon anomaly macros
#include <interrupt.h>              // defines interrupt support
#include <platform_include.h>       // defines MMR macros
#include <adi_osal.h>               // OSAL support
#include <sys/fatal_error_code.h>   // defines fatal error support

/* INT macro     - suitable to be used for the default interrupt vector
**                 instructions sequence of interrupts other than SECI.
*/
#define INT(irp)                           \
    .RETAIN_NAME ___int_##irp;             \
    .EXTERN __dispatcher;                  \
    ___int_##irp:                          \
    JUMP __dispatcher (DB);                \
       ASTAT = ADI_CID_##irp;              \
       NOP;                                \
    NOP;

/* INT_SEC macro - default SECI interrupt support instructions sequence. */
#define INT_SEC(irp)                       \
    .RETAIN_NAME ___int_SEC;               \
    .EXTERN __dispatcher_SEC;              \
    ___int_SEC:                            \
    JUMP __dispatcher_SEC (DB);            \
       ASTAT = ADI_CID_SECI;               \
       NOP;                                \
    NOP;

/* INT_JUMP      - instructions to jump to a target label without
**                 going through an interrupt dispatcher.
*/
#define INT_JUMP(irp, jump_tgt)            \
    .EXTERN jump_tgt;                      \
    .RETAIN_NAME ___int_jump_##irp;        \
    ___int_jump_##irp:                     \
    NOP;                                   \
    JUMP jump_tgt;                         \
    NOP;                                   \
    NOP;

/* UNUSED_INTERRUPT - The instructions jump to fatal_error for the case
**                    where an interrupt that was not meant to be used
**                    gets latched.
*/
#define UNUSED_INTERRUPT(irp)              \
    .EXTERN _adi_fatal_error;              \
    .RETAIN_NAME ___int_unused_##irp;      \
    ___int_unused_##irp:                   \
    R12 = ADI_CID_##irp;                   \
    JUMP (PC, _adi_fatal_error) (DB);      \
       R4 = _AFE_G_RunTimeError;           \
       R8 = _AFE_S_UnusedInterruptRaised;

/* RESERVED_INTERRUPT - 4 RTI instructions for reserved interrupts. */
#define RESERVED_INTERRUPT RTI;RTI;RTI;RTI;

/*
** ADSP-SC5xx and ADSP-215xx Interrupt vector code
*/
.SECTION/CODE/NW/DOUBLEANY iv_code;

___interrupt_table:

.RETAIN_NAME ___int_EMUI;   // 0x00 - Emulator Interrupt
___int_EMUI:
      NOP;
      NOP;
      NOP;
      // These slots are not used for anything by the emulator. So we
      // don't expect to run through here.
      // However we can do, if an error causes us to jump to address 0x0.
      // Instead of falling through to the reset vector, detect this error
      // and try to give some diagnostics about how it happened.
      .EXTERN _adi_bad_reset_detected;
      JUMP _adi_bad_reset_detected;

INT_JUMP(RSTI,start)        // 0x04  Reset Interrupt

RESERVED_INTERRUPT          // 0x08  reserved
INT_JUMP(PARI,_adi_parity_error_detected) // 0x0c  L1 parity error
                            // 0x10  Illegal opcode detected
INT_JUMP(ILOPI,_adi_ilop_detected)
INT(CB7I)                   // 0x14  Circular Buffer 7 Overflow
INT(IICDI)                  // 0x18  Unaligned long word access detected
INT(SOVFI)                  // 0x1c  Status, loop, or mode stack overflow, or PC stack full
INT(ILADI)                  // 0x20  Illegal address space detected
RESERVED_INTERRUPT          // 0x24  reserved
RESERVED_INTERRUPT          // 0x28  reserved
INT(TMZHI)                  // 0x2c  Timer=0 (high priority option)
INT(BKPI)                   // 0x30  Hardware Breakpoint Interrupt
RESERVED_INTERRUPT          // 0x34  reserved
RESERVED_INTERRUPT          // 0x38  reserved
INT_SEC(SECI)               // 0x3c  System event controller interrupt
RESERVED_INTERRUPT          // 0x40  reserved
RESERVED_INTERRUPT          // 0x44  reserved
RESERVED_INTERRUPT          // 0x48  reserved
RESERVED_INTERRUPT          // 0x4c  reserved
INT(RINSEQI)                // 0x50  Restricted instruction sequence
INT(CB15I)                  // 0x54  Circular Buffer 15 overflow
INT(TMZLI)                  // 0x58  Timer = 0 (low priority option)
INT(FIXI)                   // 0x5c  Fixed-point overflow
INT(FLTOI)                  // 0x60  Floating-point overflow exception
INT(FLTUI)                  // 0x64  Floating-point underflow exception
INT(FLTII)                  // 0x68  Floating-point invalid exception
INT(EMULI)                  // 0x6c  Emulator Low Priority Interrupt
INT(SFT0I)                  // 0x70  User software interrupt 0
INT(SFT1I)                  // 0x74  User software interrupt 1
INT(SFT2I)                  // 0x78  User software interrupt 2
INT(SFT3I)                  // 0x7c  User software interrupt 3
.___interrupt_table.end:    // 0x80

