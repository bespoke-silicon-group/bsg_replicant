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

# HARDWARE_PATH: The path to the hardware folder in BSG F1
ifndef HARDWARE_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: HARDWARE_PATH is not defined$(NC)"))
endif

ifndef __HARDWARE_MEMSYS_DRAMSIM3_MK
__HARDWARE_MEMSYS_DRAMSIM3_MK := 1

# is this a dramsim3 memory configuration?
ifneq ($(filter dramsim3, $(subst _, ,$(CL_MANYCORE_MEM_CFG))),)

CL_MANYCORE_MEMSYS_ID := "DRS3"

# hbm2_4gb_x128?
ifneq ($(filter %_hbm2_4gb_x128, $(CL_MANYCORE_MEM_CFG)),)
# memory hierarchy bits
CL_MANYCORE_MEMSYS_DRAM_RO_BITS := 14
CL_MANYCORE_MEMSYS_DRAM_BG_BITS := 2
CL_MANYCORE_MEMSYS_DRAM_BA_BITS := 2
CL_MANYCORE_MEMSYS_DRAM_CO_BITS := 6
CL_MANYCORE_MEMSYS_DRAM_BYTE_OFF_BITS := 5

# memory hierarchy bitidx
CL_MANYCORE_MEMSYS_DRAM_RO_BITIDX := 11
CL_MANYCORE_MEMSYS_DRAM_BG_BITIDX := 27
CL_MANYCORE_MEMSYS_DRAM_BA_BITIDX := 25
CL_MANYCORE_MEMSYS_DRAM_CO_BITIDX := 5
CL_MANYCORE_MEMSYS_DRAM_BYTE_OFF_BITIDX := 0

# set which DRAMSim3 chip we're modeling
DRAMSIM3_MEMORY := hbm2_4gb_x128
endif

# hbm2_8gb_x128?
ifneq ($(filter %_hbm2_8gb_x128, $(CL_MANYCORE_MEM_CFG)),)
# memory hierarchy bits
CL_MANYCORE_MEMSYS_DRAM_RO_BITS := 15
CL_MANYCORE_MEMSYS_DRAM_BG_BITS := 2
CL_MANYCORE_MEMSYS_DRAM_BA_BITS := 2
CL_MANYCORE_MEMSYS_DRAM_CO_BITS := 6
CL_MANYCORE_MEMSYS_DRAM_BYTE_OFF_BITS := 5

# memory hierarchy bitidx
CL_MANYCORE_MEMSYS_DRAM_RO_BITIDX := 11
CL_MANYCORE_MEMSYS_DRAM_BG_BITIDX := 28
CL_MANYCORE_MEMSYS_DRAM_BA_BITIDX := 26
CL_MANYCORE_MEMSYS_DRAM_CO_BITIDX := 5
CL_MANYCORE_MEMSYS_DRAM_BYTE_OFF_BITIDX := 0

# set which DRAMSim3 chip we're modeling
DRAMSIM3_MEMORY := hbm2_8gb_x128
endif

# some extra VDEFINES
MEMSYS_VDEFINES-yes += DRAMSIM3_MEMORY=$(strip $(DRAMSIM3_MEMORY))
MEMSYS_VDEFINES-yes += DRAMSIM3_MEM_PKG=bsg_dramsim3_$(strip $(DRAMSIM3_MEMORY))_pkg
MEMSYS_VDEFINES-yes += USING_DRAMSIM3=1

endif
endif
