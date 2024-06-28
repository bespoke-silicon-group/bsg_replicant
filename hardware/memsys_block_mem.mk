ifndef HARDWARE_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: HARDWARE_PATH is not defined$(NC)"))
endif

ifndef __HARDWARE_MEMSYS_BLOCKMEM_MK
__HARDWARE_MEMSYS_BLOCKMEM_MK := 1

# is this an inifinite memory configuration?
ifneq ($(filter %_block_mem,$(BSG_MACHINE_MEM_CFG)),)

CL_MANYCORE_MEMSYS_ID := "BLKM"

endif
endif
