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

# This Makefile fragment defines rules for compilation of the C/C++
# files for running regression tests.

ORANGE=\033[0;33m
RED=\033[0;31m
NC=\033[0m

# This file REQUIRES several variables to be set. They are typically
# set by the Makefile that includes this makefile..
# 

DEFINES    += -DVCS
INCLUDES   += -I$(LIBRARIES_PATH)
INCLUDES   += -I$(BSG_PLATFORM_PATH)

CXXFLAGS   += -lstdc++ $(DEFINES) -fPIC
CFLAGS     += -lc $(DEFINES) -fPIC

# each regression target needs to build its .o from a .c and .h of the
# same name
%.o: %.c
	$(CC) -c -o $@ $< $(INCLUDES) $(CFLAGS) $(CDEFINES)

# ... or a .cpp and .hpp of the same name
%.o: %.cpp
	$(CXX) -c -o $@ $< $(INCLUDES) $(CXXFLAGS) $(CXXDEFINES)

# Compile all of the sources into a shared object file for dynamic loading.
TEST_CSOURCES   += $(filter %.c,$(TEST_SOURCES))
TEST_CXXSOURCES += $(filter %.cpp,$(TEST_SOURCES))
TEST_OBJECTS    += $(TEST_CXXSOURCES:.cpp=.o)
TEST_OBJECTS    += $(TEST_CSOURCES:.c=.o)

main.so: $(TEST_OBJECTS)
	$(LD) -shared -o $@ $^ $(LDFLAGS)

.PRECIOUS: %.o %.so

.PHONY: platform.compilation.clean
platform.compilation.clean:
	rm -rf *.o *.so

compilation.clean: platform.compilation.clean
