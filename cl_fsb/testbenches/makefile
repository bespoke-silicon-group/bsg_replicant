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

PROJECT         = cl_fsb
SIM_TARGETS     = cosim rtlsim
RUN_WITH_CHECK ?= 0

ifeq ($(RUN_WITH_CHECK),1)
  CHECK_RULE = check
else
  CHECK_RULE =
endif

.PHONY:cosim rtlsim

$(SIM_TARGETS):
ifndef PROJECT
$(error "PROJECT undefined. Set PROJECT before running this makefile")
endif
	PROJECT=$(PROJECT) make -C $@ $(CHECK_RULE)

clean:
	rm -rf sim
