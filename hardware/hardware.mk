# Copyright (c) 2019, University of Washington All rights reserved.
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

ifndef __BSG_HARDWARE_MK
__BSG_HARDWARE_MK := 1

# This makefile fragment adds to the list of hardware sources
# $(VSOURCES) and $(VHEADERS) that are necessary to use this project.

# Some $(VSOURCES) and $(VHEADERS) are generated by scripts. These
# files have rules that generate the outputs, so it is good practice
# to have rules depend on $(VSOURCES) and $(VHEADERS)

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
#
# The path to the Makefile.machine.include that defines Machine parameters
ifndef BSG_MACHINE_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MACHINE_PATH is not defined$(NC)"))
endif

# HARDWARE_PATH: The path to the hardware folder in BSG F1
ifndef HARDWARE_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: HARDWARE_PATH is not defined$(NC)"))
endif

# BSG_MANYCORE_DIR: The path to the bsg_manycore repository
ifndef BSG_MANYCORE_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_MANYCORE_DIR is not defined$(NC)"))
endif

# BASEJUMP_STL_DIR: The path to the bsg_manycore repository
ifndef BASEJUMP_STL_DIR
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BASEJUMP_STL_DIR is not defined$(NC)"))
endif

# Makefile.machine.include defines the Manycore hardware
# configuration.
include $(BSG_MACHINE_PATH)/Makefile.machine.include
BSG_BLADERUNNER_RELEASE_VERSION          ?= $(shell echo $(FPGA_IMAGE_VERSION) | sed 's/\([0-9]*\)\.\([0-9]*\).\([0-9]*\)/000\10\20\3/')
BSG_BLADERUNNER_COMPILATION_DATE         ?= $(shell date +%m%d%Y)

# The following variables are defined by environment.mk if this bsg_f1
# repository is a submodule of bsg_bladerunner. If they are not set,
# we use default values.
BASEJUMP_STL_COMMIT_ID ?= deadbeef
BSG_MANYCORE_COMMIT_ID ?= feedcafe
BSG_F1_COMMIT_ID       ?= 42c0ffee
FPGA_IMAGE_VERSION     ?= 0.0.0

ifdef BSG_MACHINE_CHIP_ID
CHIP_ID = $(BSG_MACHINE_CHIP_ID)
else
CHIP_ID = 00000000
endif

# The manycore architecture sources are defined in arch_filelist.mk. The
# unsynthesizable simulation sources (for tracing, etc) are defined in
# sim_filelist.mk. Each file adds to VSOURCES and VINCLUDES and depends on
# BSG_MANYCORE_DIR
include $(BSG_MANYCORE_DIR)/machines/arch_filelist.mk

# So that we can limit tool-specific to a few specific spots we use VDEFINES,
# VINCLUDES, and VSOURCES to hold lists of macros, include directores, and
# verilog sources (respectively). These are used during simulation compilation,
# but transformed into a tool-specific syntax where necesssary.
VINCLUDES += $(HARDWARE_PATH)
VINCLUDES += $(BSG_MACHINE_PATH)
VINCLUDES  += $(BSG_MANYCORE_DIR)/testbenches/common/v/

# Configuration definitions
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_network_cfg_pkg.sv
VSOURCES += $(BSG_MANYCORE_DIR)/testbenches/common/v/bsg_manycore_mem_cfg_pkg.sv

# Machine-Specific Source (Autogenerated)
VSOURCES += $(BSG_MACHINE_PATH)/bsg_bladerunner_pkg.sv

VDEFINES += BSG_MACHINE_ORIGIN_COORD_X=$(BSG_MACHINE_ORIGIN_COORD_X)
VDEFINES += BSG_MACHINE_ORIGIN_COORD_Y=$(BSG_MACHINE_ORIGIN_COORD_Y)

# Platform-Specific Sources
include $(BSG_PLATFORM_PATH)/hardware.mk

# The following functions convert a decimal string to a binary string,
# and a hexadecimal string (WITHOUT the preceeding 0x) into binary
# strings of 32-characters in length
define dec2bin
	`perl -e 'printf "%032b\n",'$(strip $(1))`
endef

define hex2bin
	`perl -e 'printf "%032b\n",'0x$(strip $(1))`
endef

define charv2hex
	$(shell python -c 'print("{:02x}{:02x}{:02x}{:02x}".format(*(ord(c) for c in $(strip $(1))[::-1])))')
endef

charv2bin = $(call hex2bin, $(call charv2hex, $(1)))

include $(HARDWARE_PATH)/memsys.mk

# This target generates the ASCII file for the memory system ROM data.
# It is important to keep these grouped together.
$(BSG_MACHINE_PATH)/bsg_bladerunner_memsys.rom: $(BSG_MACHINE_PATH)/Makefile.machine.include
	@echo $(call charv2bin,$(CL_MANYCORE_MEMSYS_ID)) > $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_CHANNELS)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_BANK_SIZE)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_RO_BITS)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_BG_BITS)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_BA_BITS)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_CO_BITS)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_BYTE_OFF_BITS)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_RO_BITIDX)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_BG_BITIDX)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_BA_BITIDX)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_CO_BITIDX)) >> $@.temp
	@echo $(call dec2bin,$(CL_MANYCORE_MEMSYS_DRAM_BYTE_OFF_BITIDX)) >> $@.temp
	mv $@.temp $@

# This target generates the ASCII file for the ROM. To add entries to
# the ROM, add more commands below.
$(BSG_MACHINE_PATH)/bsg_bladerunner_configuration.rom: $(BSG_MACHINE_PATH)/bsg_bladerunner_memsys.rom
$(BSG_MACHINE_PATH)/bsg_bladerunner_configuration.rom: $(BSG_MACHINE_PATH)/Makefile.machine.include
	@echo $(call hex2bin,$(BSG_BLADERUNNER_RELEASE_VERSION))   > $@.temp
	@echo $(call hex2bin,$(BSG_BLADERUNNER_COMPILATION_DATE))  >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_MAX_EPA_WIDTH))     >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_DATA_WIDTH))        >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_PODS_X))       >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_PODS_Y))       >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_POD_TILES_X))       >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_POD_TILES_Y))       >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_HOST_COORD_X))      >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_HOST_COORD_Y))      >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_ORIGIN_COORD_X))    >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_ORIGIN_COORD_Y))    >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_NOC_COORD_X_WIDTH)) >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_NOC_COORD_Y_WIDTH)) >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_RUCHE_FACTOR_X))    >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_BARRIER_RUCHE_FACTOR_X)) >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_WH_RUCHE_FACTOR))   >> $@.temp
	@echo $(call hex2bin,$(BASEJUMP_STL_COMMIT_ID))        >> $@.temp
	@echo $(call hex2bin,$(BSG_MANYCORE_COMMIT_ID))        >> $@.temp
	@echo $(call hex2bin,$(BSG_F1_COMMIT_ID))              >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_VCACHE_WAY))       >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_VCACHE_SET))       >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_VCACHE_LINE_WORDS))  >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_VCACHE_STRIPE_WORDS)) >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_VCACHE_MISS_FIFO_ELS)) >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_VCACHE_IPOLY_HASHING)) >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_VCACHE_WORD_TRACKING)) >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_ENABLE_DMA)) >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_IO_REMOTE_LOAD_CAP)) >> $@.temp
	@echo $(call dec2bin,$(BSG_MACHINE_IO_EP_CREDITS)) >> $@.temp
	@echo $(call dec2bin,0)                            >> $@.temp
	@echo $(call hex2bin,$(CHIP_ID))                   >> $@.temp
	@cat $(BSG_MACHINE_PATH)/bsg_bladerunner_memsys.rom >> $@.temp
	mv $@.temp $@

# This target generates a C header file with defines for machine parameters
$(BSG_MACHINE_PATH)/bsg_manycore_machine.h: $(BSG_MACHINE_PATH)/Makefile.machine.include $(BSG_F1_DIR)/machine.mk
	@echo "#ifndef BSG_MANYCORE_MACHINE_H" >  $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_H" >> $@.temp
	@echo "/* Chip address and data width */" >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_MAX_EPA_WIDTH $(BSG_MACHINE_MAX_EPA_WIDTH)" >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_DATA_WIDTH    $(BSG_MACHINE_DATA_WIDTH)"    >> $@.temp
	@echo "/* Chip dimensions */"     >> $@.temp
	@echo "#define BSG_MANYCORE_PODS_Y         $(BSG_MACHINE_PODS_Y)"       >> $@.temp
	@echo "#define BSG_MANYCORE_PODS_X         $(BSG_MACHINE_PODS_X)"       >> $@.temp
	@echo "/* Chip Pod dimensions */"     >> $@.temp
	@echo "#define BSG_MANYCORE_POD_TILES_Y         $(BSG_MACHINE_POD_TILES_Y)"         >> $@.temp
	@echo "#define BSG_MANYCORE_POD_TILES_X         $(BSG_MACHINE_POD_TILES_X)"         >> $@.temp
	@echo "/* Host coordinates */"            >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_HOST_COORD_X  \\" >> $@.temp
	@echo "    $(BSG_MACHINE_HOST_COORD_X)"  >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_HOST_COORD_Y  \\" >> $@.temp
	@echo "    $(BSG_MACHINE_HOST_COORD_Y)"  >> $@.temp
	@echo "/* NoC field widths */"     >> $@.temp
	@echo "#define BSG_MANYCORE_NOC_COORD_X_WIDTH $(BSG_MACHINE_NOC_COORD_X_WIDTH)" >> $@.temp
	@echo "#define BSG_MANYCORE_NOC_COORD_Y_WIDTH $(BSG_MACHINE_NOC_COORD_Y_WIDTH)" >> $@.temp
	@echo "/* L2 cache parameters */"         >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_VCACHE_WAYS   \\" >> $@.temp
	@echo "    $(BSG_MACHINE_VCACHE_WAY)"   >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_VCACHE_BANK_SETS   \\" >> $@.temp
	@echo "    $(BSG_MACHINE_VCACHE_SET)" >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_VCACHE_BLOCK_SIZE_WORDS \\"  >> $@.temp
	@echo "    $(BSG_MACHINE_VCACHE_BLOCK_WORDS)" >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_VCACHE_BANK_STRIPE_SIZE_WORDS \\" >> $@.temp
	@echo "    $(BSG_MACHINE_VCACHE_STRIPE_WORDS)" >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_VCACHE_BANKS \\" >> $@.temp
	@echo "    (2 * (BSG_MANYCORE_MACHINE_DIM_X))" >> $@.temp
	@echo "#define BSG_MANYCORE_MACHINE_VCACHE_SETS \\" >> $@.temp
	@echo "    ((BSG_MANYCORE_MACHINE_VCACHE_BANK_SETS)*(BSG_MANYCORE_MACHINE_VCACHE_BANKS))" >> $@.temp
	@echo "#endif" >> $@.temp
	mv $@.temp $@


# This package defines the number of lines in the ROM, the width of
# those elements, and the contents of the ROM.
#
# ROM_STR defines the contents of the ROM. The first sed command adds
# verilog format specifiers and commas. The scond sed command removes
# the trailing comma (fencepost problem). We need to reverse the order
# of the lines so that the first element appears last in the string
# (at the 0'th position according to verilog).
#
# dpi_fifo_els_gp defines the number of elements in the
# manycore-to-host (request and response) FIFOs.
$(BSG_MACHINE_PATH)/bsg_bladerunner_pkg.sv: $(BSG_MACHINE_PATH)/bsg_bladerunner_configuration.rom
	$(eval ROM_STR=$(shell tac $< | sed "s/^\(.*\)$$/32'b\1,/" | sed '$$s/,$$//'))
	@echo "\`ifndef BSG_BLADERUNNER_PKG" > $@
	@echo "\`define BSG_BLADERUNNER_PKG" >> $@
	@echo >> $@
	@echo "package bsg_bladerunner_pkg;" >> $@
	@echo >> $@
	@echo "import bsg_manycore_pkg::*;" >> $@
	@echo "import bsg_manycore_network_cfg_pkg::*;" >> $@
	@echo "import bsg_manycore_mem_cfg_pkg::*;" >> $@
	@echo >> $@
	@echo "parameter bsg_machine_dpi_fifo_els_gp = $(BSG_MACHINE_IO_REMOTE_LOAD_CAP);" >> $@
	@echo >> $@
	@echo "parameter bsg_machine_rom_width_gp = 32;" >> $@
	@echo "parameter bsg_machine_rom_els_gp = `wc -l < $<`;" >> $@
	@echo "parameter bit [bsg_machine_rom_width_gp-1:0] bsg_machine_rom_arr_gp [bsg_machine_rom_els_gp-1:0] = '{$(ROM_STR)};" >> $@
	@echo "parameter int bsg_machine_pods_x_gp = $(BSG_MACHINE_PODS_X);" >> $@
	@echo "parameter int bsg_machine_pods_y_gp = $(BSG_MACHINE_PODS_Y);" >> $@
	@echo "parameter int bsg_machine_pods_cycle_time_ps_gp = $(BSG_MACHINE_PODS_CYCLE_TIME_PS);" >> $@
	@echo >> $@
	@echo "parameter int bsg_machine_pod_tiles_y_gp = $(BSG_MACHINE_POD_TILES_Y);" >> $@
	@echo "parameter int bsg_machine_pod_tiles_x_gp = $(BSG_MACHINE_POD_TILES_X);" >> $@
	@echo "parameter int bsg_machine_pod_llcache_rows_gp = $(BSG_MACHINE_POD_VCACHE_NUM_ROWS);" >> $@
	@echo >> $@
	@echo "parameter int bsg_machine_pod_tiles_subarray_y_gp = $(BSG_MACHINE_POD_TILES_SUBARRAY_Y);" >> $@
	@echo "parameter int bsg_machine_pod_tiles_subarray_x_gp = $(BSG_MACHINE_POD_TILES_SUBARRAY_X);" >> $@
	@echo >> $@
	@echo "parameter bsg_manycore_network_cfg_e bsg_machine_noc_cfg_gp = $(BSG_MACHINE_NETWORK_CFG);" >> $@
	@echo "parameter int bsg_machine_noc_ruche_factor_X_gp = $(BSG_MACHINE_RUCHE_FACTOR_X);" >> $@
	@echo "parameter int bsg_machine_barrier_ruche_factor_X_gp = $(BSG_MACHINE_BARRIER_RUCHE_FACTOR_X);" >> $@
	@echo "parameter int bsg_machine_wh_ruche_factor_gp = $(BSG_MACHINE_WH_RUCHE_FACTOR);" >> $@
	@echo "parameter int bsg_machine_noc_epa_width_gp = $(BSG_MACHINE_MAX_EPA_WIDTH);" >> $@
	@echo "parameter int bsg_machine_noc_data_width_gp = $(BSG_MACHINE_DATA_WIDTH);" >> $@
	@echo "parameter int bsg_machine_noc_coord_x_width_gp = $(BSG_MACHINE_NOC_COORD_X_WIDTH);" >> $@
	@echo "parameter int bsg_machine_noc_coord_y_width_gp = $(BSG_MACHINE_NOC_COORD_Y_WIDTH);" >> $@
	@echo "parameter int bsg_machine_noc_pod_coord_x_width_gp = bsg_machine_noc_coord_x_width_gp - \$$clog2(bsg_machine_pod_tiles_x_gp);" >> $@
	@echo "parameter int bsg_machine_noc_pod_coord_y_width_gp = bsg_machine_noc_coord_y_width_gp - \$$clog2(bsg_machine_pod_tiles_y_gp);" >> $@
	@echo >> $@
	@echo "parameter int bsg_machine_llcache_sets_gp = $(BSG_MACHINE_VCACHE_SET);" >> $@
	@echo "parameter int bsg_machine_llcache_ways_gp = $(BSG_MACHINE_VCACHE_WAY);" >> $@
	@echo "parameter int bsg_machine_llcache_line_words_gp = $(BSG_MACHINE_VCACHE_LINE_WORDS);" >> $@
	@echo "parameter int bsg_machine_llcache_words_gp = bsg_machine_llcache_line_words_gp * bsg_machine_llcache_ways_gp * bsg_machine_llcache_sets_gp;" >> $@
	@echo "parameter int bsg_machine_llcache_miss_fifo_els_gp = $(BSG_MACHINE_VCACHE_MISS_FIFO_ELS);" >> $@
	@echo "parameter int bsg_machine_llcache_channel_width_gp = $(BSG_MACHINE_VCACHE_DMA_DATA_WIDTH);" >> $@
	@echo "parameter int bsg_machine_llcache_dram_channel_ratio_gp = $(BSG_MACHINE_VCACHE_PER_DRAM_CHANNEL);" >> $@
	@echo "parameter int bsg_machine_llcache_word_tracking_gp = $(BSG_MACHINE_VCACHE_WORD_TRACKING);" >> $@
	@echo "parameter int bsg_machine_llcache_ipoly_hashing_gp = $(BSG_MACHINE_VCACHE_IPOLY_HASHING);" >> $@
	@echo >> $@
	@echo "parameter int bsg_machine_dram_bank_words_gp = $(BSG_MACHINE_DRAM_BANK_WORDS);" >> $@
	@echo "parameter int bsg_machine_dram_channels_gp = $(BSG_MACHINE_DRAM_CHANNELS);" >> $@
	@echo "parameter int bsg_machine_dram_words_gp = $(BSG_MACHINE_DRAM_WORDS);" >> $@
	@echo "parameter bsg_manycore_mem_cfg_e bsg_machine_dram_cfg_gp = $(BSG_MACHINE_MEM_CFG);" >> $@
	@echo >> $@
	@echo "parameter bit bsg_machine_branch_trace_en_gp = $(BSG_MACHINE_BRANCH_TRACE_EN);" >> $@
	@echo >> $@
	@echo "parameter int bsg_machine_io_coord_y_gp = $(BSG_MACHINE_HOST_COORD_Y);" >> $@
	@echo "parameter int bsg_machine_io_coord_x_gp = $(BSG_MACHINE_HOST_COORD_X);" >> $@
	@echo "parameter int bsg_machine_io_credits_max_gp = $(BSG_MACHINE_IO_EP_CREDITS);" >> $@ # credits for endpoint
	@echo >> $@
	@echo "parameter int bsg_machine_origin_coord_y_gp = $(BSG_MACHINE_ORIGIN_COORD_Y);" >> $@
	@echo "parameter int bsg_machine_origin_coord_x_gp = $(BSG_MACHINE_ORIGIN_COORD_X);" >> $@
	@echo >> $@
	@echo "parameter int bsg_machine_pod_num_cores_gp = bsg_machine_pod_tiles_x_gp * bsg_machine_pod_tiles_y_gp;" >> $@
	@echo "parameter int bsg_machine_hetero_type_vec_gp [0:(bsg_machine_pod_num_cores_gp)-1] = '{$(strip $(BSG_MACHINE_HETERO_TYPE_VEC))};" >> $@
	@echo >> $@
	@echo "parameter bsg_machine_core_dmem_words_gp = 1024;" >> $@
	@echo "parameter bsg_machine_core_icache_entries_gp = 1024;" >> $@
	@echo "parameter bsg_machine_core_icache_tag_width_gp = 12;" >> $@
	@echo "parameter bsg_machine_core_icache_line_words_gp = 4;" >> $@
	@echo >> $@
	@echo "parameter string bsg_machine_name_gp = \"$(BSG_MACHINE_NAME)\";" >> $@
	@echo >> $@
	@echo "endpackage" >> $@
	@echo >> $@
	@echo "\`endif" >> $@


.PHONY: hardware.clean hardware.bleach

hardware.bleach:

# The rm commands on the hardware path are legacy, but we keep them
# because NOT cleaning those files can really F*** with running
# VCS. You can thank Max and I later
hardware.clean:
	rm -f $(BSG_MACHINE_PATH)/bsg_bladerunner_configuration.{rom,sv}
	rm -f $(BSG_MACHINE_PATH)/bsg_bladerunner_memsys.rom
	rm -f $(BSG_MACHINE_PATH)/bsg_manycore_machine.h
	rm -f $(BSG_MACHINE_PATH)/bsg_bladerunner_pkg.sv
	rm -f $(BSG_MACHINE_PATH)/bsg_bladerunner_configuration.sv
	rm -f $(BSG_MACHINE_PATH)/bsg_tag_boot_rom.tr
	rm -f $(BSG_MACHINE_PATH)/bsg_tag_boot_rom.sv

.PRECIOUS: $(BSG_MACHINE_PATH)/bsg_bladerunner_configuration.rom
.PRECIOUS: $(BSG_MACHINE_PATH)/bsg_bladerunner_pkg.sv
.PRECIOUS: $(BSG_MACHINE_PATH)/bsg_bladerunner_memsys.rom
.PRECIOUS: $(BSG_MACHINE_PATH)/bsg_manycore_machine.h

endif
