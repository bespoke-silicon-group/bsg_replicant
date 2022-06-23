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

# This fragment generates a msg_config file, which controls the
# warnings and lint messages from non-BSG code. We generate it
# dynamically so that it works on multiple machine setups.

# BSG_PLATFORM_PATH: The path to the testbenches folder in BSG F1
ifndef BSG_PLATFORM_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: BSG_PLATFORM_PATH is not defined$(NC)"))
endif

# The following variables are set by $(BSG_F1_DIR)/hdk.mk, which will fail if
# hdk_setup.sh has not been run, or environment.mk is not included
#
# HDK_SHELL_DESIGN_DIR: Path to the directory containing all the AWS "shell" IP
# AWS_FPGA_REPO_DIR: Path to the clone of the aws-fpga repo
# HDK_COMMON_DIR: Path to HDK 'common' directory w/ libraries for cosimluation.
# SDK_DIR: Path to the SDK directory in the aws-fpga repo
include $(BSG_F1_DIR)/hdk.mk

$(BSG_PLATFORM_PATH)/msg_config:
	$(eval XILINX_VIVADO=$(realpath $(XILINX_VIVADO)))

	@echo "{+warn=none;  +file=$(BSG_MANYCORE_DIR)/imports/HardFloat/source/HardFloat_primitives.v;}" >> $@
	@echo "{+lint=none;  +file=$(BSG_MANYCORE_DIR)/imports/HardFloat/source/HardFloat_primitives.v;}" >> $@

	@echo "{+warn=none;  +file=$(BSG_MANYCORE_DIR)/imports/HardFloat/source/HardFloat_primitives.v;}" >> $@
	@echo "{+lint=none;  +file=$(BSG_MANYCORE_DIR)/imports/HardFloat/source/HardFloat_primitives.v;}" >> $@

	@echo "{+warn=none;  +file=$(BSG_MANYCORE_DIR)/imports/HardFloat/source/HardFloat_rawFN.v;}" >> $@
	@echo "{+lint=none;  +file=$(BSG_MANYCORE_DIR)/imports/HardFloat/source/HardFloat_rawFN.v;}" >> $@

	@echo "{+warn=none;  +file=$(BSG_MANYCORE_DIR)/imports/HardFloat/source/mulAddRecFN.v;}" >> $@
	@echo "{+lint=none;  +file=$(BSG_MANYCORE_DIR)/imports/HardFloat/source/mulAddRecFN.v;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DIR)/hlx/verif/cl_ports_sh_bfm.vh;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DIR)/hlx/verif/cl_ports_sh_bfm.vh;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/flop_ccf.sv;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/flop_ccf.sv;}" >> $@
	@echo "{-error=none; +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/flop_ccf.sv;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/axi4_slave_bfm.sv;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/axi4_slave_bfm.sv;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/sh_ddr.sv;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/sh_ddr.sv;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/sync.v;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/sync.v;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/ccf_ctl.v;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/ccf_ctl.v;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/gray.inc;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/sh_ddr/sim/gray.inc;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/lib/lib_pipe.sv;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/lib/lib_pipe.sv;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/ip/axi_register_slice_light/hdl/axi_register_slice_v2_1_vl_rfs.v;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/ip/axi_register_slice_light/hdl/axi_register_slice_v2_1_vl_rfs.v;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/ip/axi_clock_converter_0/hdl/fifo_generator_v13_2_rfs.v;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/ip/axi_clock_converter_0/hdl/fifo_generator_v13_2_rfs.v;}" >> $@
	@echo "{-error=none; +file=$(HDK_SHELL_DESIGN_DIR)/ip/axi_clock_converter_0/hdl/fifo_generator_v13_2_rfs.v;}" >> $@

	@echo "{+warn=none;  +file=$(XILINX_VIVADO)/data/ip/xilinx/fifo_generator_v13_1/simulation/fifo_generator_vlog_beh.v;}" >> $@
	@echo "{+lint=none;  +file=$(XILINX_VIVADO)/data/ip/xilinx/fifo_generator_v13_1/simulation/fifo_generator_vlog_beh.v;}" >> $@
	@echo "{-error=none; +file=$(XILINX_VIVADO)/data/ip/xilinx/fifo_generator_v13_1/simulation/fifo_generator_vlog_beh.v;}" >> $@

	@echo "{+warn=none;  +file=$(XILINX_VIVADO)/data/ip/xilinx/fifo_generator_v13_2/hdl/fifo_generator_v13_2_rfs.v;}" >> $@
	@echo "{+lint=none;  +file=$(XILINX_VIVADO)/data/ip/xilinx/fifo_generator_v13_2/hdl/fifo_generator_v13_2_rfs.v;}" >> $@
	@echo "{-error=none; +file=$(XILINX_VIVADO)/data/ip/xilinx/fifo_generator_v13_2/hdl/fifo_generator_v13_2_rfs.v;}" >> $@

	@echo "{+warn=none;  +file=$(XILINX_VIVADO)/data/ip/xilinx/axi_infrastructure_v1_1/hdl/axi_infrastructure_v1_1_vl_rfs.v;}" >> $@
	@echo "{+lint=none;  +file=$(XILINX_VIVADO)/data/ip/xilinx/axi_infrastructure_v1_1/hdl/axi_infrastructure_v1_1_vl_rfs.v;}" >> $@

	@echo "{+warn=none;  +file=$(XILINX_VIVADO)/data/ip/xilinx/axi_clock_converter_v2_1/hdl/axi_clock_converter_v2_1_vl_rfs.v;}" >> $@
	@echo "{+lint=none;  +file=$(XILINX_VIVADO)/data/ip/xilinx/axi_clock_converter_v2_1/hdl/axi_clock_converter_v2_1_vl_rfs.v;}" >> $@

	@echo "{+warn=none;  +file=$(XILINX_VIVADO)/data/verilog/src/glbl.v;}" >> $@
	@echo "{+lint=none;  +file=$(XILINX_VIVADO)/data/verilog/src/glbl.v;}" >> $@

	@echo "{+warn=none;  +file=$(XILINX_VIVADO)/data/verilog/src/retarget/BUFGMUX.v;}" >> $@
	@echo "{+lint=none;  +file=$(XILINX_VIVADO)/data/verilog/src/retarget/BUFGMUX.v;}" >> $@

	@echo "{+warn=none;  +file=$(XILINX_VIVADO)/data/verilog/src/unisims/BUFGCTRL.v;}" >> $@
	@echo "{+lint=none;  +file=$(XILINX_VIVADO)/data/verilog/src/unisims/BUFGCTRL.v;}" >> $@

	@echo "{+warn=none;  +file=$(XILINX_VIVADO)/data/verilog/src/unisims/IOBUFDS.v;}" >> $@
	@echo "{+lint=none;  +file=$(XILINX_VIVADO)/data/verilog/src/unisims/IOBUFDS.v;}" >> $@

	@echo "{+warn=none;  +file=$(XILINX_VIVADO)/data/verilog/src/unisims/IOBUFE3.v;}" >> $@
	@echo "{+lint=none;  +file=$(XILINX_VIVADO)/data/verilog/src/unisims/IOBUFE3.v;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/interfaces/unused_ddr_a_b_d_template.inc;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/interfaces/unused_ddr_a_b_d_template.inc;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_SHELL_DESIGN_DIR)/interfaces/cl_ports.vh;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_SHELL_DESIGN_DIR)/interfaces/cl_ports.vh;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_DIR)/common/verif/models/sh_bfm/axil_bfm.sv}" >> $@
	@echo "{+lint=none;  +file=$(HDK_DIR)/common/verif/models/sh_bfm/axil_bfm.sv}" >> $@

	@echo "{+warn=none;  +file=$(HDK_DIR)/common/verif/models/ddr4_model/StateTable.sv;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_DIR)/common/verif/models/ddr4_model/StateTable.sv;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_DIR)/common/verif/models/ddr4_model/arch_package.sv;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_DIR)/common/verif/models/ddr4_model/arch_package.sv;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_DIR)/common/verif/models/fpga/fpga_ddr.svh;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_DIR)/common/verif/models/fpga/fpga_ddr.svh;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_DIR)/common/verif/models/xilinx_axi_pc/axi_protocol_checker_v1_1_vl_rfs.v;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_DIR)/common/verif/models/xilinx_axi_pc/axi_protocol_checker_v1_1_vl_rfs.v;}" >> $@


	@echo "{+warn=none;  +file=$(HDK_DIR)/common/verif/models/fpga/fpga.sv;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_DIR)/common/verif/models/fpga/fpga.sv;}" >> $@
	@echo "{-error=none;  +file=$(HDK_DIR)/common/verif/models/fpga/fpga.sv;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_DIR)/common/verif/models/fpga/card.sv;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_DIR)/common/verif/models/fpga/card.sv;}" >> $@

	@echo "{+warn=none;  +file=$(HDK_DIR)/common/verif/models/sh_bfm/sh_bfm.sv;}" >> $@
	@echo "{+lint=none;  +file=$(HDK_DIR)/common/verif/models/sh_bfm/sh_bfm.sv;}" >> $@
