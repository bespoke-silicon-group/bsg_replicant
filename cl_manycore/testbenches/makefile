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

TEST_TARGETS := $(CL_DIR)/software/read_write_test.c $(CL_DIR)/software/rom_test.c $(CL_DIR)/software/loopback_test.c

.PHONY:cosim rtlsim REGRESSION_TARGETS

NOISY ?= 1

REGRESSION_TARGETS := $(foreach target, $(TEST_TARGETS), $(target).regression)

$(SIM_TARGETS): $(REGRESSION_TARGETS)
ifndef PROJECT
$(error "PROJECT undefined. Set PROJECT before running this makefile")
endif

ifeq ($(NOISY), 1)
%.regression:
	@echo "========================================="
	@echo "start running $@:"
	make -C cosim $(CHECK_RULE) PROJECT=$(PROJECT) $(if $(DEBUG),DEBUG=$(DEBUG)) C_TB_TOP=$(subst .regression,,$@)
else
%.regression:
	@echo "========================================="
	@echo "start running $@:"
	make -C cosim $(CHECK_RULE) PROJECT=$(PROJECT) $(if $(DEBUG),DEBUG=$(DEBUG)) C_TB_TOP=$(subst .regression,,$@) > tmp.log
	@grep "PASSED\|FAILED" tmp.log
	@echo "========================================="
	@echo ""
endif
	
clean:
	rm -rf sim tmp.log
