# This Makefile Fragment defines rules for compilation of the C/C++
# regression tests.

# each target in INDEPENDENT_TESTS needs to build its .o from a
# .c and .h of the same name
OBJECTS = $(foreach tgt, $(INDEPENDENT_TESTS), $(tgt).o)
%.o: %.c %.h
	$(CC) -c $< -o $@ $(CFLAGS) $(CDEFINES) -DBSG_TEST_NAME=$(patsubst %.c,%,$<) 

# ... or a .cpp and .hpp of the same name
%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) $(CXXDEFINES) -DBSG_TEST_NAME=$(patsubst %.cpp,%,$<) -c -o $@ $<

$(UNIFIED_TESTS): %: test_unified_main
test_unified_main: LD=$(CC)
test_unified_main: %: %.o
	$(LD) $(filter %.o, $^) $(LDFLAGS) -o $@

# each target '%' in INDEPENDENT_TESTS relies on an object file
# '%.o'
$(INDEPENDENT_TESTS): LD=$(CC)
$(INDEPENDENT_TESTS): %: %.o
	$(LD) -o $@ $(filter %.o, $^) $(LDFLAGS)

build.clean: 
	rm -rf *.o $(INDEPENDENT_TESTS) test_unified_main

# To include a test in regression, the user defines a list of tests in
# REGRESSION_TESTS. Each test can also define a custom rule, <test_name>.rule
# that is run during compilation. These custom rules are useful for building
# spmd or cuda binaries, for example.
USER_RULES:=$(addsuffix .rule,$(REGRESSION_TESTS))
$(USER_RULES):
USER_CLEAN_RULES=$(addsuffix .clean,$(REGRESSION_TESTS))
$(USER_CLEAN_RULES):

$(addsuffix .log,$(INDEPENDENT_TESTS)): %.log: % %.rule
	sudo ./$< | tee $@

%.log: test_unified_main %.rule
	sudo ./$< $(basename $@) | tee $@

regression: regression.log
regression.log: $(addsuffix .log, $(REGRESSION_TESTS))
	@pass=0; total=0; \
	echo ""| tee $@; \
	echo ""| tee -a $@; \
	echo "Parsing $(REGRESSION_TESTS_TYPE) Regression Test results..."| tee -a $@; \
	echo ""| tee -a $@; \
	echo ""| tee -a $@; \
	for target in $(basename $(basename $?)); do \
		if grep "BSG REGRESSION TEST .*PASSED.*" $$target.log > /dev/null; then \
			echo "PASS: Regression Test $$target passed!"| tee -a $@; \
			let "pass+=1"; \
		else \
			echo "FAIL: Regression Test $$target failed!"| tee -a $@; \
		fi; \
		let "total+=1"; \
	done; \
	if [ ! $$pass == $$total ]; then \
		echo "==================================================="| tee -a $@; \
		echo "" | tee -a $@; \
		echo "FAIL! $$pass out of $$total $(REGRESSION_TESTS_TYPE) tests passed"| tee -a $@; \
		echo "" | tee -a $@; \
		echo "==================================================="| tee -a $@; \
		exit 1; \
	fi; \
	echo "==========================================================="| tee -a $@; \
	echo ""| tee -a $@; \
	echo "PASS! All $$total tests passed for $(REGRESSION_TESTS_TYPE)"| tee -a $@; \
	echo ""| tee -a $@; \
	echo "==========================================================="| tee -a $@;

regression.clean: $(USER_CLEAN_RULES)
	rm -f *.log

clean: regression.clean build.clean

.PHONY: build.clean
.PHONY: $(USER_RULES) $(USER_CLEAN_RULES) regression regression.clean

.DEFAULT_GOAL := help
help:
	@echo "Usage:"
	@echo "make {regression|clean|<test_name>|<test_name>.log}"
	@echo "      regression: Run all regression tests for "
	@echo "             this subdirectory"
	@echo "      <test_name>: Build the regression binary for a specific"
	@echo "             test"
	@echo "      <test_name.log>: Run a specific regression test and "
	@echo "             generate the log file"
	@echo "      clean: Remove all subdirectory-specific outputs"
