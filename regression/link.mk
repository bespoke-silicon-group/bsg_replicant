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

# This Makefile Fragment defines rules for linking object files of
# regression tests.

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this fragment...
# 
# EXEC_PATH: The path to the directory where tests will be executed
ifndef EXEC_PATH
$(error $(shell echo -e "$(RED)BSG MAKE ERROR: EXEC_PATH is not defined$(NC)"))
endif

LDFLAGS += -lbsg_manycore_runtime -lm

$(UNIFIED_TESTS): %: $(EXEC_PATH)/test_loader
$(EXEC_PATH)/test_loader: LD=$(CC)
$(EXEC_PATH)/test_loader: %: %.o
	$(LD) $(filter %.o, $^) $(LDFLAGS) -o $@

# each target, '%', in INDEPENDENT_TESTS relies on an object file '%.o'
$(INDEPENDENT_TESTS): LD=$(CC)
$(INDEPENDENT_TESTS): %: $(EXEC_PATH)/%.o
	$(LD) -o $@ $(filter %.o, $^) $(LDFLAGS)


link.clean:
	rm -rf $(INDEPENDENT_TESTS) test_loader
