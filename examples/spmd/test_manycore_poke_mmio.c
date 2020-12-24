

#include <stdint.h>

// BlackParrot has 36 bits on address space to embedded the manycore address space in
// bsg_manycore has a 32-bit word-aligned EVA address which we embed in a BlackParrot
//   40-bit physical address as follows:
//   40'h20_0000_0000_0000_0000-40'h20_ffff_ffff_ffff_ffff
//
//   EPA word address width: 16
//   NPA width             : 28
//
//                      1                    31
//   Host EVA  : [dram_not_tile |          address        ]
//
//                1               29                2 
//   DRAM      : [1 |            EPA           | low bits ]
//
//                2      6        6       16        2
//   Tile      : [01 |  y-cord | x-cord | EPA  | low bits ]



#ifdef VCS
int vcs_main(int argc, char ** argv) {
#else
int main(int argc, char ** argv) {
#endif
    uint64_t bp_cfg_offset = 0x200000;
    volatile uint64_t *did_mask_addr = (uint64_t *) (0x0009 + bp_cfg_offset);
    volatile uint64_t *sac_mask_addr = (uint64_t *) (0x000a + bp_cfg_offset);

    // Unlock manycore domain
    *did_mask_addr = 3;

    // BP has a coprocessor address space starting at 0x02_0000_0000
    uint64_t bp_coproc_offset  = (2UL << 36UL);

    // Set up EPA mapping according to vanilla core map
    uint64_t mc_csr_freeze_epa = 0x00020000UL;
    uint64_t mc_csr_pc_epa     = 0x0002000cUL;

    // Set up EVA according to map above
    uint64_t dram_not_tile = (0UL << 31UL);
    uint64_t tile_not_dram = (1UL << 30UL);

    uint64_t tile_y_cord   = (2UL << 24UL);
    uint64_t tile_x_cord   = (0UL << 18UL);

    // Construct EVA
    volatile uint32_t *mc_csr_pc_eva = (uint32_t *) (bp_coproc_offset | dram_not_tile | tile_not_dram | tile_y_cord | tile_x_cord | mc_csr_pc_epa);
    volatile uint32_t *mc_csr_freeze_eva = (uint32_t *) (bp_coproc_offset | dram_not_tile | tile_not_dram | tile_y_cord | tile_x_cord | mc_csr_freeze_epa);

    // Do a write
    *mc_csr_pc_eva = 0xdeadbeef;

    __asm__ volatile ("fence");

    // Read it back
    uint32_t mc_pc = *mc_csr_pc_eva;

    while(1);
}

