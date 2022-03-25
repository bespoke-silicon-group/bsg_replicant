/*********************************************************
 BSG Manycore Linker Script 

 data default: dmem
 dram memory size: 0x40000000
 imem allocated size: 0x01000000
 stack pointer init: 0x00000ffc

 Generated at 2021-09-22 15:07:24.543997
**********************************************************/

MEMORY {
DMEM_VMA (rw) : ORIGIN = 0x00000000, LENGTH = 0x00001000
DRAM_T_LMA (rx) : ORIGIN = 0x80000000, LENGTH = 0x01000000
DRAM_D_LMA (rw) : ORIGIN = 0x81000000, LENGTH = 0x3f000000
}

SECTIONS {

.dmem  :
AT(0x0) {
  *(.dmem.interrupt)
  *(.dmem)
  *(.dmem.*)
  *(.rodata)
  *(.rodata*)
  *(.srodata.cst16)
  *(.srodata.cst8)
  *(.srodata.cst4)
  *(.srodata.cst2)
  *(.srodata*)
  . = ALIGN(8);

} >DMEM_VMA 

.data.dmem  :
AT(LOADADDR(.dmem) + ADDR(.data.dmem) - ADDR(.dmem)) {
  *(.data)
  *(.data*)
  . = ALIGN(8);

} >DMEM_VMA 

.sdata.dmem  :
AT(LOADADDR(.data.dmem) + ADDR(.sdata.dmem) - ADDR(.data.dmem)) {
  *(.sdata)
  *(.sdata.*)
  *(.sdata*)
  *(.sdata*.*.gnu.linkonce.s.*)
  . = ALIGN(8);

} >DMEM_VMA 

.sbss.dmem  :
AT(LOADADDR(.sdata.dmem) + ADDR(.sbss.dmem) - ADDR(.sdata.dmem)) {
  *(.sbss)
  *(.sbss.*)
  *(.gnu.linkonce.sb.*)
  *(.scommon)
  . = ALIGN(8);

} >DMEM_VMA 

.bss.dmem  :
AT(LOADADDR(.sbss.dmem) + ADDR(.bss.dmem) - ADDR(.sbss.dmem)) {
  *(.bss)
  *(.bss*)
  . = ALIGN(8);

} >DMEM_VMA 

.tdata.dmem  :
AT(LOADADDR(.bss.dmem) + ADDR(.tdata.dmem) - ADDR(.bss.dmem)) {
  *(.tdata)
  *(.tdata*)
  . = ALIGN(8);

} >DMEM_VMA 

.tbss.dmem  :
AT(LOADADDR(.tdata.dmem) + ADDR(.tbss.dmem) - ADDR(.tdata.dmem)) {
  *(.tbss)
  *(.tbss*)
  . = ALIGN(8);

} >DMEM_VMA 

.striped.data.dmem  :
AT(LOADADDR(.tbss.dmem) + ADDR(.striped.data.dmem) - ADDR(.tbss.dmem)) {
  *(.striped.data)
  . = ALIGN(8);

} >DMEM_VMA 

__dmem_end = .;
ASSERT((__dmem_end <= 0x00001000), "Error: dmem size limit exceeded")

.text.dram 0x0 :
 {
  *(.text.interrupt)
  *(.crtbegin)
  *(.text)
  *(.text.startup)
  *(.text.*)
  PROVIDE (__.text.dram_end = .);
  ASSERT((__.text.dram_end <= 0x01000000), "Error: .text.dram section exceeded limit 0x01000000");
  . = ALIGN(8);

}  AT>DRAM_T_LMA

. = 0x80000000 + 0x01000000;

.eh_frame.dram  :
 {
  *(.eh_frame)
  *(.eh_frame*)
  . = ALIGN(8);

}  AT>DRAM_D_LMA

.dram  :
 {
  *(.dram)
  *(.dram.*)
  . = ALIGN(8);

}  AT>DRAM_D_LMA

_gp = ADDR(.sdata.dmem) + 0x800;
_sp = 0x00000ffc;
_bsg_data_start_addr = 0x00000000;
_bsg_data_end_addr = ADDR(.striped.data.dmem) + SIZEOF(.striped.data.dmem);
_bsg_striped_data_start = ADDR(.striped.data.dmem);
_bsg_dram_end_addr = LOADADDR(.dram) + SIZEOF(.dram);
_end = _bsg_dram_end_addr;

}
