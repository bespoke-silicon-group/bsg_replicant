# This makefile fragment defines the regression rules that are reused between ALL
# regression directories (Cosimulation and F1 Execution).

# We build a list of LOG_RULES for the regression rule (below)
LOG_RULES = $(addsuffix .log,$(REGRESSION_TESTS))
LOG_TARGETS =$(addprefix $(EXEC_PATH)/,$(LOG_RULES))
$(LOG_RULES): %: $(EXEC_PATH)/%

# The regression target runs all of the tests in the REGRESSION_TESTS variable
# for a directory.
regression: $(EXEC_PATH)/regression.log 
$(EXEC_PATH)/regression.log: $(LOG_TARGETS)
	@pass=0; total=0; \
	echo ""| tee $@; \
	echo "==========================================================="| tee -a $@; \
	echo ""| tee -a $@; \
	echo "Parsing $(REGRESSION_TESTS_TYPE) Regression Test results..."| tee -a $@; \
	echo ""| tee -a $@; \
	echo "==========================================================="| tee -a $@; \
	echo ""| tee -a $@; \
	for target in $(notdir $(basename $^)); do \
		if grep "BSG REGRESSION TEST .*PASSED.*" $(EXEC_PATH)/$$target.log > /dev/null; then \
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
		echo "FAIL! $$pass out of $$total $(REGRESSION_TESTS_TYPE) regression tests passed"| tee -a $@; \
		echo "" | tee -a $@; \
		echo "==================================================="| tee -a $@; \
		exit 1 | tee -a $@; \
	else \
		echo "==========================================================="| tee -a $@; \
		echo ""| tee -a $@; \
		echo "PASS! All $$total tests passed for $(REGRESSION_TESTS_TYPE)"| tee -a $@; \
		echo ""| tee -a $@; \
		echo "==========================================================="| tee -a $@; \
	fi;

regression.clean:
	rm -rf $(LOG_RULES) regression.log

.PHONY: regression regression.clean
CLEANS += regression.clean
