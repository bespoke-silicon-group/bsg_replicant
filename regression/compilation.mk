# This Makefile Fragment defines rules for compilation of the C/C++
# regression tests.
LDFLAGS += -lbsg_manycore_runtime -lm

# each target in INDEPENDENT_TESTS needs to build its .o from a
# .c and .h of the same name
OBJECTS = $(foreach tgt, $(INDEPENDENT_TESTS), $(tgt).o)
%.o: %.c %.h
	$(CC) -c $< -o $@ $(CFLAGS) $(CDEFINES) -DBSG_TEST_NAME=$(patsubst %.c,%,$<) 

# ... or a .cpp and .hpp of the same name
%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) $(CXXDEFINES) -DBSG_TEST_NAME=$(patsubst %.cpp,%,$<) -c -o $@ $<

$(UNIFIED_TESTS): %: $(EXEC_PATH)/test_loader
$(EXEC_PATH)/test_loader: LD=$(CC)
$(EXEC_PATH)/test_loader: %: %.o
	$(LD) $(filter %.o, $^) $(LDFLAGS) -o $@

# each target, '%', in INDEPENDENT_TESTS relies on an object file '%.o'
$(INDEPENDENT_TESTS): LD=$(CC)
$(INDEPENDENT_TESTS): %: $(EXEC_PATH)/%.o
	$(LD) -o $@ $(filter %.o, $^) $(LDFLAGS)

# To include a test in regression, the user defines a list of tests in
# REGRESSION_TESTS. Each test can also define a custom rule, <test_name>.rule
# that is run during compilation. These custom rules are useful for building
# spmd or cuda binaries, for example.
USER_RULES:=$(addsuffix .rule,$(REGRESSION_TESTS))
$(USER_RULES):
USER_CLEAN_RULES=$(addsuffix .clean,$(REGRESSION_TESTS))
$(USER_CLEAN_RULES):

compilation.clean: 
	rm -rf $(OBJECTS) $(INDEPENDENT_TESTS) test_loader

.PHONY: compilation.clean $(USER_RULES) $(USER_CLEAN_RULES)
