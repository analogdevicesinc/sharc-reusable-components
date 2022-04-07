/* Copyright (c) 2014, Analog Devices, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 * * Neither the name of Analog Devices, Inc.  nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

MEMORY
{
  /* #### SHARED L2 Memory ############################################### */
  /* MCAPI ICC DATA, uncached */
  MEM_L2_MCAPI_DATA_ICC : ORIGIN = 0x20080000, LENGTH = 4K

  /* MCAPI Buffer ARM Core 0, uncached */
  MEM_L2_MCAPI_DATA_CORE0 : ORIGIN = 0x20081000, LENGTH = 4K

  /* MCAPI Buffer SHARC Core 2, uncached */
  MEM_L2_MCAPI_DATA_CORE2 (r) : ORIGIN = 0x20082000, LENGTH = 4K

  /* MCAPI Buffer SHARC Core 1, uncached */
  MEM_L2_MCAPI_DATA_CORE1 (r) : ORIGIN = 0x20083000, LENGTH = 48K

  /* #### ARM L2 Memory ################################################## */

  /* ARM Core 0 L2 RAM, uncached */
  MEM_L2_UNCACHED : ORIGIN = 0x2008F000, LENGTH = 16K

  /* ARM Core 0 L2 RAM, cached */
  MEM_L2_CACHED : ORIGIN = 0x20093000, LENGTH = 52K

  /* #### SHARC L2 Memory ################################################ */
  /* SHARC Core 2 L2 RAM, cached */
  MEM_L2_CORE2 (r) : ORIGIN = 0x200A0000, LENGTH = 64K

  /* SHARC CORE 1 L2 RAM, cached */
  MEM_L2_CORE1 (r) : ORIGIN = 0x200B0000, LENGTH = 64K

  /* #### SHARC L3 Memory ################################################ */

  /* L3: SHARC Core 1 */
  /* SHARC CORE 1 NW Code, DMC0 */
  MEM_L3_CORE1_NW_CODE (r) : ORIGIN = 0x80000000, LENGTH = 6M

  /* SHARC CORE1 DATA, DMC0 */
  MEM_L3_CORE1_DATA (r) : ORIGIN = 0x80600000, LENGTH = 4M

  /* SHARC CORE1 VISA Code, DMC0 */
  MEM_L3_CORE1_VISA_CODE (r) : ORIGIN = 0x80A00000, LENGTH = 6M

  /* SHARC CORE1 DATA, DMC0 */
  MEM_L3_CORE1_DATA2 (r) : ORIGIN = 0x81000000, LENGTH = 128M

  /* L3: SHARC Core 2 */
  /* SHARC CORE2 DATA, DMC0 */
  MEM_L3_CORE2_DATA2 (r) : ORIGIN = 0x89000000, LENGTH = 112M

  /* SHARC CORE2 NW Code, DMC1 */
  MEM_L3_CORE2_NW_CODE (r) : ORIGIN = 0xC0000000, LENGTH = 6M

  /* SHARC CORE2 DATA, DMC1 */
  MEM_L3_CORE2_DATA (r) : ORIGIN = 0xC0600000, LENGTH = 4M

  /* SHARC CORE2 VISA Code, DMC1 */
  MEM_L3_CORE2_VISA_CODE (r) : ORIGIN = 0xC0A00000, LENGTH = 6M

  /* #### ARM L3 Memory ################################################## */

  /* ARM CORE 0 L3, DMC1 */
  /*  MEM_L3 : ORIGIN = 0xC1000000, LENGTH = 240M*/
  MEM_L3 : ORIGIN = 0xC1000000, LENGTH = 225M
  MEM_L3_UNCACHED : ORIGIN = 0xCF000000, LENGTH = 15M
}

SECTIONS {
  __MCAPI_common_start = ORIGIN(MEM_L2_MCAPI_DATA_ICC) ;
  __MCAPI_common_end   = ORIGIN(MEM_L2_MCAPI_DATA_ICC) + LENGTH(MEM_L2_MCAPI_DATA_ICC) ;
  __MCAPI_arm_start = ORIGIN(MEM_L2_MCAPI_DATA_CORE0) ;
  __MCAPI_arm_end   = ORIGIN(MEM_L2_MCAPI_DATA_CORE0) + LENGTH(MEM_L2_MCAPI_DATA_CORE0) ;
  __MCAPI_sharc0_start = ORIGIN(MEM_L2_MCAPI_DATA_CORE1) ;
  __MCAPI_sharc0_end   = ORIGIN(MEM_L2_MCAPI_DATA_CORE1) + LENGTH(MEM_L2_MCAPI_DATA_CORE1) ;
  __MCAPI_sharc1_start = ORIGIN(MEM_L2_MCAPI_DATA_CORE2) ;
  __MCAPI_sharc1_end   = ORIGIN(MEM_L2_MCAPI_DATA_CORE2) + LENGTH(MEM_L2_MCAPI_DATA_CORE2) ;
  __l2_start = 0x20080000 ;
  __l2_end = 0x200C0000 ;
  __l2_uncached_start = ORIGIN(MEM_L2_MCAPI_DATA_CORE0) ;
  __l2_uncached_end = ORIGIN(MEM_L2_UNCACHED) + LENGTH(MEM_L2_UNCACHED) ;
  __l2_cached_start = ORIGIN(MEM_L2_CACHED) ;
  __l2_cached_end = ORIGIN(MEM_L2_CORE1) + LENGTH(MEM_L2_CORE1) ;
}

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
          "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
/* Do we need any of these for elf?
   __DYNAMIC = 0;    */
SECTIONS
{
  /* Read-only sections, merged into text segment: */
  .interp     : { *(.interp)    }
  .hash          : { *(.hash)       }
  .dynsym        : { *(.dynsym)     }
  .dynstr        : { *(.dynstr)     }
  .gnu.version   : { *(.gnu.version)    }
  .gnu.version_d   : { *(.gnu.version_d)    }
  .gnu.version_r   : { *(.gnu.version_r)    }
  .rel.init      : { *(.rel.init)   }
  .rela.init     : { *(.rela.init)  }
  .rel.text      :
    {
      *(.rel.text)
      *(.rel.text.*)
      *(.rel.gnu.linkonce.t*)
    }
  .rela.text     :
    {
      *(.rela.text)
      *(.rela.text.*)
      *(.rela.gnu.linkonce.t*)
    }
  .rel.fini      : { *(.rel.fini)   }
  .rela.fini     : { *(.rela.fini)  }
  .rel.rodata    :
    {
      *(.rel.rodata)
      *(.rel.rodata.*)
      *(.rel.gnu.linkonce.r*)
    }
  .rela.rodata   :
    {
      *(.rela.rodata)
      *(.rela.rodata.*)
      *(.rela.gnu.linkonce.r*)
    }
  .rel.data      :
    {
      *(.rel.data)
      *(.rel.data.*)
      *(.rel.gnu.linkonce.d*)
    }
  .rela.data     :
    {
      *(.rela.data)
      *(.rela.data.*)
      *(.rela.gnu.linkonce.d*)
    }
  .rel.ctors     : { *(.rel.ctors)  }
  .rela.ctors    : { *(.rela.ctors) }
  .rel.dtors     : { *(.rel.dtors)  }
  .rela.dtors    : { *(.rela.dtors) }
  .rel.got       : { *(.rel.got)        }
  .rela.got      : { *(.rela.got)       }
  .rel.sdata     :
    {
      *(.rel.sdata)
      *(.rel.sdata.*)
      *(.rel.gnu.linkonce.s*)
    }
  .rela.sdata     :
    {
      *(.rela.sdata)
      *(.rela.sdata.*)
      *(.rela.gnu.linkonce.s*)
    }
  .rel.sbss      : { *(.rel.sbss)       }
  .rela.sbss     : { *(.rela.sbss)  }
  .rel.bss       : { *(.rel.bss)        }
  .rela.bss      : { *(.rela.bss)       }
  .rel.plt       : { *(.rel.plt)        }
  .rela.plt      : { *(.rela.plt)       }
  .plt      : { *(.plt) }
  /* L2 uncached section:
  ** Place any objects required to be uncached. */
  .l2_uncached :
  {
    *(.l2_uncached_crt)
    *(.l2_uncached_code)
    *(.l2_uncached_data)
  } >MEM_L2_UNCACHED = 0

  /* L2 cached section:
  ** Place any objects explicitly required to be cached in L2. */
  .l2_cached :
  {
    *(.l2_cached_crt)
    *(.l2_cached_code)
    *(.l2_cached_data)
  } >MEM_L2_CACHED = 0
  /* L3 Memory. No code/data placed here by default */
  .l3 :
  {
    *(.l3_code)
    *(.l3_data)
  } >MEM_L3 = 0

  /* MMU Configuration data */
  .mmu_config :
  {
    KEEP (*(.mmu_config))
  } >MEM_L3 = 0

  /* Heap object */
  .heap (NOLOAD) :
  {
    __heap_start = . ;
    *(.heap)
    __heap_end = . ;
  } >MEM_L3 = 0

  /* System mode stack */
  .stack_sys (NOLOAD) :
  {
    __stack_sys_end = . ;
    KEEP(*(.stack_sys))
    __stack_sys_start = . ;
  } > MEM_L2_CACHED = 0

  /* Supervisor mode stack */
  .stack_sup (NOLOAD) :
  {
    __stack_sup_end = . ;
    KEEP(*(.stack_sup))
    __stack_sup_start = . ;
  } > MEM_L2_CACHED = 0

  /* FIQ mode stack */
  .stack_fiq (NOLOAD) :
  {
    __stack_fiq_end = . ;
    KEEP(*(.stack_fiq))
    __stack_fiq_start = . ;
  } > MEM_L2_CACHED = 0

  /* IRQ mode stack */
  .stack_irq (NOLOAD) :
  {
    __stack_irq_end = . ;
    KEEP(*(.stack_irq))
    __stack_irq_start = . ;
  } > MEM_L2_CACHED = 0

  /* Abort mode stack */
  .stack_abort (NOLOAD) :
  {
    __stack_abort_end = . ;
    KEEP(*(.stack_abort))
    __stack_abort_start = . ;
  } > MEM_L3 = 0

  /* Undef mode stack */
  .stack_undef (NOLOAD) :
  {
    __stack_undef_end = . ;
    KEEP(*(.stack_undef))
    __stack_undef_start = . ;
  } > MEM_L3 = 0

  /* MCAPI section:
     Only declare sections that are writeable by the ARM
     core.  */
  .mcapi_data_icc   :
  {
    *(.mcapi_data_icc)
  } >MEM_L2_MCAPI_DATA_ICC

  .mcapi_data_core0   :
  {
    *(.mcapi_data_core0)
  } >MEM_L2_MCAPI_DATA_CORE0

  .text      :
  {
    *(.text)
    *(.text.*)
    *(.stub)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
    *(.glue_7t) *(.glue_7)
  } >MEM_L3 =0
  .init          :
  {
    KEEP (*(.init))
  } >MEM_L3 =0
  _etext = .;
  PROVIDE (etext = .);
  .fini      :
  {
    KEEP (*(.fini))
  } >MEM_L3 =0
  .rodata   : { *(.rodata) *(.rodata.*) *(.gnu.linkonce.r*) } >MEM_L3 =0
  .rodata1   : { *(.rodata1) } >MEM_L3 =0
  .eh_frame_hdr : { *(.eh_frame_hdr) } >MEM_L3 =0
  .ARM.extab   : { *(.ARM.extab*  .gnu.linkonce.armextab.*) } >MEM_L3 =0
  .ARM.exidx   : {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } >MEM_L3 =0
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  . = ALIGN(256) + (. & (256 - 1));
  .data    :
  {
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d*)
    SORT(CONSTRUCTORS)
  } >MEM_L3 =0
  .data1   : { *(.data1) } >MEM_L3 =0
  .eh_frame : { KEEP (*(.eh_frame)) } >MEM_L3 =0
  .gcc_except_table : { *(.gcc_except_table) } >MEM_L3 =0
  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >MEM_L3 =0
  .init_array     :
  {
     PROVIDE_HIDDEN (__init_array_start = .);
     KEEP (*(SORT(.init_array.*)))
     KEEP (*(.init_array))
     PROVIDE_HIDDEN (__init_array_end = .);
  } >MEM_L3 =0
  .fini_array     :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(.fini_array))
    KEEP (*(SORT(.fini_array.*)))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >MEM_L3 =0
  .ctors   :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin.o(.ctors))
    /* We don't want to include the .ctor section from
       from the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } >MEM_L3 =0
   .dtors         :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } >MEM_L3 =0
  .jcr           : { KEEP (*(.jcr)) } >MEM_L3 =0
  .got           : { *(.got.plt) *(.got) } >MEM_L3 =0
  .dynamic       : { *(.dynamic) } >MEM_L3 =0
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
  .sdata     :
  {
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
  } >MEM_L3 =0
  _edata = .;
  PROVIDE (edata = .);
  __bss_start = .;
  __bss_start__ = .;
  .sbss      :
  {
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.scommon)
  } >MEM_L3 =0
  .bss       :
  {
   *(.dynbss)
   *(.bss)
   *(.bss.*)
   *(COMMON)
   /* Align here to ensure that the .bss section occupies space up to
      _end.  Align after .bss to ensure correct alignment even if the
      .bss section disappears because there are no input sections.  */
   . = ALIGN(32 / 8);
  } >MEM_L3 =0
  _bss_end__ = . ; __bss_end__ = . ;
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  .debug_ranges   0 : { *(.debug_ranges) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
}



SECTIONS {
 .l3_uncached :
  {
    *(.l3_uncached_code)
    *(.l3_uncached_data)
  } >MEM_L3_UNCACHED = 0
}
