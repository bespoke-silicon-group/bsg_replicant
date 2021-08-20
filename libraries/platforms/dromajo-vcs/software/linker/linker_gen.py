# Copyright (c) 2020, University of Washington All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this list
# of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#!/usr/bin/python
from __future__ import print_function

import argparse
import json


def print_linker(bp_dram_base, sp, hb_dram_base):
    print(
        """
    OUTPUT_ARCH( "riscv" )
    ENTRY(_start)
    
    SECTIONS {{
    
      /*--------------------------------------------------------------------*/
      /* Code and read-only segment                                         */
      /*--------------------------------------------------------------------*/
    
      /* Begining of code and text segment */
      . = {bp_dram_base};
      _ftext = .;
      PROVIDE( eprol = . );
    
      /* text: Program code section */
      .text : 
      {{
        *(.text.init)
        *(.text.emu)
        *(.text.amo)
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
      }}
    
      /* init: Code to execute before main (called by crt0.S) */
      .init : 
      {{
        KEEP( *(.init) )
      }}
    
      /* fini: Code to execute after main (called by crt0.S) */
      .fini : 
      {{
        KEEP( *(.fini) )
      }}
    
      /* rodata: Read-only data */
      .rodata : 
      {{
        *(.rdata)
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
      }}
    
      /* End of code and read-only segment */
      PROVIDE( etext = . );
      _etext = .;
    
      /*--------------------------------------------------------------------*/
      /* Global constructor/destructor segement                             */
      /*--------------------------------------------------------------------*/
    
      .preinit_array     :
      {{
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
      }}
    
      .init_array     :
      {{
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array ))
        PROVIDE_HIDDEN (__init_array_end = .);
      }}
    
      .fini_array     :
      {{
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array ))
        PROVIDE_HIDDEN (__fini_array_end = .);
      }}
    
      .eh_frame_hdr     : {{ *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) }}
      .eh_frame         : {{ KEEP (*(.eh_frame)) *(.eh_frame.*) }}
      .gcc_except_table : {{ *(.gcc_except_table .gcc_except_table.*) }}
      .gnu_extab        : {{ *(.gnu_extab) }}
      .exception_ranges : {{ *(.exception_ranges*) }}
      .jcr              : {{ KEEP (*(.jcr))       }}
    
      /*--------------------------------------------------------------------*/
      /* Initialized data segment                                           */
      /*--------------------------------------------------------------------*/
    
      /* Start of initialized data segment */
      . = ALIGN(16);
       _fdata = .;
    
      /* data: Writable data */
      .data : 
      {{
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
      }}
    
      /* Have _gp point to middle of sdata/sbss to maximize displacement range */
      . = ALIGN(16);
      _gp = . + 0x800;
    
      /* Writable small data segment */
      .sdata : 
      {{
        *(.sdata)
        *(.sdata.*)
        *(.srodata.*)
        *(.gnu.linkonce.s.*)
      }}
    
      /* End of initialized data segment */
      PROVIDE( edata = . );
      _edata = .;
    
      /*--------------------------------------------------------------------*/
      /* Uninitialized data segment                                         */
      /*--------------------------------------------------------------------*/
    
      /* Start of uninitialized data segment */
      . = ALIGN(8);
      _fbss = .;
    
      /* Writable uninitialized small data segment */
      .sbss : 
      {{
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linkonce.sb.*)
      }}
    
      /* bss: Uninitialized writeable data section */
      . = .;
      _bss_start = .;
      .bss : 
      {{
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
      }}
    
      /* End of uninitialized data segment (used by syscalls.c for heap) */
      PROVIDE( end = . );
      _end = ALIGN(8);
    
      _sp = {_sp};

      /* Manycore Binary Segment */
      . = {hb_dram_base};
      .manycore :
      {{
        *(.manycore.*)
      }}
    }}""".format(bp_dram_base=bp_dram_base, _sp=sp, hb_dram_base=hb_dram_base)
    )

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("bp_dram_base", help="BlackParrot DRAM Base Address")
    parser.add_argument("sp", help="Top of the stack")
    parser.add_argument("hb_dram_base", help="HammerBlade Manycore DRAM Base Address")
    args = parser.parse_args()

    print_linker(args.bp_dram_base, args.sp, args.hb_dram_base)