REGRESSION_TESTS_TYPE = library

UNIFIED_TESTS = 

INDEPENDENT_TESTS += test_rom
INDEPENDENT_TESTS += test_struct_size
INDEPENDENT_TESTS += test_vcache_flush
INDEPENDENT_TESTS += test_vcache_simplified
INDEPENDENT_TESTS += test_vcache_stride
INDEPENDENT_TESTS += test_vcache_sequence
INDEPENDENT_TESTS += test_printing
INDEPENDENT_TESTS += test_manycore_packets
INDEPENDENT_TESTS += test_manycore_init
INDEPENDENT_TESTS += test_manycore_dmem_read_write
INDEPENDENT_TESTS += test_manycore_vcache_sequence
INDEPENDENT_TESTS += test_manycore_dram_read_write
INDEPENDENT_TESTS += test_manycore_eva
INDEPENDENT_TESTS += test_manycore_credits
INDEPENDENT_TESTS += test_manycore_eva_read_write
INDEPENDENT_TESTS += test_read_mem_scatter_gather

REGRESSION_TESTS = $(UNIFIED_TESTS) $(INDEPENDENT_TESTS)

DEFINES += -DBSG_MANYCORE_DIR=$(abspath $(BSG_MANYCORE_DIR))
DEFINES += -DCL_MANYCORE_DIM_X=$(CL_MANYCORE_DIM_X) 
DEFINES += -DCL_MANYCORE_DIM_Y=$(CL_MANYCORE_DIM_Y)
DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE

CDEFINES   += $(DEFINES)
CXXDEFINES += $(DEFINES)

FLAGS     = -g -Wall
CFLAGS   += -std=c99 $(FLAGS) 
CXXFLAGS += -std=c++11 $(FLAGS)
LDFLAGS  += -lbsg_manycore_runtime -lm 
