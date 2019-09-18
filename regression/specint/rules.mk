# This Makefile fragment defines rules for building RISC-V binaries
# associated with the tests in this sub-directory

# SPECINT_SRC_PATH is the path to specint directory in BSG
# Manycore. It contains makefiles to compile specint programs.
SPECINT_SRC_PATH = $(BSG_MANYCORE_DIR)/software/spmd/specint2000

.PHONY: test_%.clean $(USER_RULES)

$(USER_RULES): test_%.rule: $(SPECINT_SRC_PATH)/%.riscv

$(USER_CLEAN_RULES): 
	CL_DIR=$(CL_DIR) \
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	$(MAKE) -C $(SPECINT_SRC_PATH) -f Makefile.$(subst .clean,,$(subst test_,,$@)) clean

$(SPECINT_SRC_PATH)/%.riscv: $(CL_DIR)/Makefile.machine.include
	CL_DIR=$(CL_DIR) \
	BSG_MANYCORE_DIR=$(BSG_MANYCORE_DIR) \
	BASEJUMP_STL_DIR=$(BASEJUMP_STL_DIR) \
	bsg_tiles_X=$(TILE_GROUP_DIM_X) \
	bsg_tiles_Y=$(TILE_GROUP_DIM_Y) \
	IGNORE_CADENV=1 \
	BSG_MACHINE_PATH=$(BSG_MACHINE_PATH) \
	$(MAKE) -C $(SPECINT_SRC_PATH) -f Makefile.$(subst .riscv,,$(notdir $@)) clean $(notdir $@)
