REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

include $(REPLICANT_PATH)/environment.mk
include $(BSG_MACHINE_PATH)/Makefile.machine.include

ifeq ($(OPT),yes)
-include $(APPLICATION_PATH)/spmm_abrev/opts-$(INPUT).mk
include $(APPLICATION_PATH)/spmm_abrev/opts-default.mk
else
# unoptimized options
SPMM_RV				= spmm.riscv.rvo
SPMM_SOLVE_ROW_RV		= spmm_solve_row_hash_table
SPMM_COMPUTE_OFFSETS_RV		= spmm_compute_offsets_sum_tree
SPMM_COPY_RESULTS_RV		= spmm_copy_results
SPMM_SORT_ROW_RV		= spmm_sort_row_stdsort
SPMM_SOLVE_ROW_LOCAL_DATA_WORDS	= 0
SPMM_PREFETCH                   = no
SPMM_PREFETCH_N			= 1
SPMM_ALIGNED_TABLE		= no
SMM_COMPLEX_HASH		= no
SPMM_SKIP_SORTING		= no
SPMM_WORK_GRANULARITY		= 1
SPMM_HASH_TABLE                 = yes
endif

# implementation dependent
RISCV_TARGET_OBJECTS += $(SPMM_RV)

# all implementations use these
RISCV_TARGET_OBJECTS-$(SPMM_HASH_TABLE) += spmm_hash_table.riscv.rvo
RISCV_TARGET_OBJECTS += $(RISCV_TARGET_OBJECTS-yes)
RISCV_TARGET_OBJECTS += spmm_init.riscv.rvo
RISCV_TARGET_OBJECTS += spmm_barrier.riscv.rvo

RISCV_CCPPFLAGS-$(SPMM_PREFETCH)	+= -DSPMM_PREFETCH
RISCV_CCPPFLAGS-$(SPMM_PREFETCH)	+= -DPREFETCH=$(SPMM_PREFETCH_N)
RISCV_CCPPFLAGS-$(SPMM_ALIGNED_TABLE)	+= -DALIGNED_TABLE
RISCV_CCPPFLAGS-$(SPMM_COMPLEX_HASH)    += -DCOMPLEX_HASH
RISCV_CCPPFLAGS-$(SPMM_SKIP_SORTING)    += -DSPMM_SKIP_SORTING
RISCV_CCPPFLAGS-yes			+= -DSPMM_SOLVE_ROW_LOCAL_DATA_WORDS=$(SPMM_SOLVE_ROW_LOCAL_DATA_WORDS)
RISCV_CCPPFLAGS-yes			+= -DSPMM_WORK_GRANULARITY=$(SPMM_WORK_GRANULARITY)
RISCV_CCPPFLAGS-yes			+= -DNONZEROS_TABLE_SIZE=1024
RISCV_CCPPFLAGS-yes			+= -I$(APPLICATION_PATH)/include/device/$(SPMM_SOLVE_ROW_RV)/
RISCV_CCPPFLAGS-yes			+= -I$(APPLICATION_PATH)/include/device/$(SPMM_COMPUTE_OFFSETS_RV)/
RISCV_CCPPFLAGS-yes			+= -I$(APPLICATION_PATH)/include/device/$(SPMM_COPY_RESULTS_RV)/
RISCV_CCPPFLAGS-yes			+= -I$(APPLICATION_PATH)/include/device/$(SPMM_SORT_ROW_RV)/

# add include flags
RISCV_CCPPFLAGS += $(RISCV_CCPPFLAGS-yes)
