#ifndef BSG_MANYCORE_MMIO
#define BSG_MANYCORE_MMIO

/* PCIe FIFOs */
#define HB_MC_MMIO_FIFO_TX_VACANCY_OFFSET 0xC
#define HB_MC_MMIO_FIFO_TX_DATA_OFFSET 0x10
#define HB_MC_MMIO_FIFO_TX_LENGTH_OFFSET 0x14
#define HB_MC_MMIO_FIFO_RX_OCCUPANCY_OFFSET 0x1C
#define HB_MC_MMIO_FIFO_RX_DATA_OFFSET 0x20
#define HB_MC_MMIO_FIFO_RX_LENGTH_OFFSET 0x24
#define HB_MC_MMIO_FIFO_ISR_OFFSET 0x0 
#define HB_MC_MMIO_FIFO_IER_OFFSET 0x4

#define HB_MC_MMIO_FIFO_NUM_BYTES 0x1000

#define hb_mc_mmio_fifo_get_direction_offset(dir) \
	(dir * HB_MC_MMIO_FIFO_NUM_BYTES)

#define hb_mc_mmio_fifo_get_reg_addr(dir, reg) \
	(hb_mc_mmio_fifo_get_direction_offset(dir) + reg)

/* Hammerblade-Manycore ROM */
#define HB_MC_MMIO_ROM_BASE 0x2000

#define HB_MC_MMIO_ROM_VERSION_OFFSET 0x00
#define HB_MC_MMIO_ROM_COMPILATION_DATE_OFFSET 0x04
#define HB_MC_MMIO_ROM_AWIDTH_OFFSET 0x08
#define HB_MC_MMIO_ROM_DIMENSION_X_OFFSET 0x10
#define HB_MC_MMIO_ROM_DIMENSION_Y_OFFSET 0x14
#define HB_MC_MMIO_ROM_HOST_INTF_COORD_X_OFFSET 0x18
#define HB_MC_MMIO_ROM_HOST_INTF_COORD_Y_OFFSET 0x1C

#define hb_mc_mmio_rom_get_reg_addr(reg) \
	(HB_MC_MMIO_ROM_BASE + reg)

/* Flow control */ 
#define HB_MC_MMIO_CREDITS_BASE 0x2100
#define HB_MC_MMIO_MAX_CREDITS 16

#define HB_MC_MMIO_CREDITS_FIFO_HOST_VACANCY_OFFSET 0x0
#define HB_MC_MMIO_CREDITS_FIFO_DEVICE_VACANCY_OFFSET 0x100
#define HB_MC_MMIO_CREDITS_HOST_OFFSET 0x200

#define hb_mc_mmio_credits_get_reg_addr(reg) \
	(HB_MC_MMIO_CREDITS_BASE + reg)


#endif
