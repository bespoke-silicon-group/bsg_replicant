# Amazon FPGA Hardware Development Kit
#
# Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Amazon Software License (the "License"). You may not use
# this file except in compliance with the License. A copy of the License is
# located at
#
#    http://aws.amazon.com/asl/
#
# or in the "license" file accompanying this file. This file is distributed on
# an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
# implied. See the License for the specific language governing permissions and
# limitations under the License.

SIM_TARGETS     = cosim rtlsim
RUN_WITH_CHECK ?= 0

ifeq ($(RUN_WITH_CHECK),1)
  CHECK_RULE = check
else
  CHECK_RULE =
endif

#TEST_TARGETS := read_write_test rom_test
TEST_TARGETS :=
BINARY_TEST_TARGETS := test_bsg_dram_loopback_cache

.PHONY:cosim rtlsim %.binary_clean

NOISY ?= 1

REGRESSION_TARGETS := $(foreach target, $(TEST_TARGETS) $(BINARY_TEST_TARGETS), $(target).regression)
REGRESSION_BINARY := $(foreach target, $(BINARY_TEST_TARGETS), $(subst test_,,$(target)).binary)
REGRESSION_BINARY_CLEAN := $(foreach target, $(BINARY_TEST_TARGETS), $(subst test_,,$(target)).binary_clean)

$(SIM_TARGETS): $(REGRESSION_BINARY) $(REGRESSION_TARGETS)
ifndef PROJECT
$(error "PROJECT undefined. Set PROJECT before running this makefile")
endif

%.binary:
	make -C $(BSG_MANYCORE_DIR)/software/spmd/$(subst .binary,,$@) clean
	make -C $(BSG_MANYCORE_DIR)/software/spmd/$(subst .binary,,$@) main.riscv bsg_tiles_X=$(bsg_tiles_X) bsg_tiles_Y=$(bsg_tiles_Y)

%.binary_clean:
	make -C $(BSG_MANYCORE_DIR)/software/spmd/$(subst .binary_clean,,$@) clean

ifeq ($(NOISY), 1)
%.regression:
	@echo "========================================="
	@echo "start running $@:"
	make -C cosim $(CHECK_RULE) PROJECT=$(PROJECT) $(if $(DEBUG),DEBUG=$(DEBUG)) C_TB_TOP=$(CL_DIR)/regression/$(subst .regression,.c,$@)
else
%.regression:
	@echo "========================================="
	@echo "start running $@:"
	make -C cosim $(CHECK_RULE) PROJECT=$(PROJECT) $(if $(DEBUG),DEBUG=$(DEBUG)) C_TB_TOP=$(CL_DIR)/software/$(subst .regression,.c,$@) > tmp.log
	@grep "PASSED\|FAILED" tmp.log
	@echo "========================================="
	@echo ""
endif

clean: %.binary_clean
	rm -rf sim tmp.log
