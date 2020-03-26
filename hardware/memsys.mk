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

ifndef __HARDWARE_MEMSYS_MEMSYS_MK
__HARDWARE_MEMSYS_MEMSYS_MK := 1

# identify the dram backend
CL_MANYCORE_MEMSYS_ID    := "NONE"

# memory hierarchy bits
CL_MANYCORE_MEMSYS_DRAM_RO_BITS := 0
CL_MANYCORE_MEMSYS_DRAM_BG_BITS := 0
CL_MANYCORE_MEMSYS_DRAM_BA_BITS := 0
CL_MANYCORE_MEMSYS_DRAM_CO_BITS := 0
CL_MANYCORE_MEMSYS_DRAM_BYTE_OFF_BITS := $(shell echo $(BSG_MACHINE_MAX_EPA_WIDTH)+2 | bc) # byte offset is all bits by default

# memory hierarchy bitidx
CL_MANYCORE_MEMSYS_DRAM_RO_BITIDX := 0
CL_MANYCORE_MEMSYS_DRAM_BG_BITIDX := 0
CL_MANYCORE_MEMSYS_DRAM_BA_BITIDX := 0
CL_MANYCORE_MEMSYS_DRAM_CO_BITIDX := 0
CL_MANYCORE_MEMSYS_DRAM_BYTE_OFF_BITIDX := 0

include $(HARDWARE_PATH)/memsys_infinite_mem.mk
include $(HARDWARE_PATH)/memsys_dramsim3.mk
include $(HARDWARE_PATH)/memsys_axi4_f1.mk

endif
