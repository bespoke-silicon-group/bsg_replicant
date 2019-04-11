SIM_TARGETS     = cosim rtlsim
RUN_WITH_CHECK ?= 0

ifeq ($(RUN_WITH_CHECK),1)
  CHECK_RULE = check
else
  CHECK_RULE =
endif

.PHONY: $(SIM_TARGETS)

$(SIM_TARGETS): envcheck
	make -C $@ $(CHECK_RULE) PROJECT=$(PROJECT) $(if $(DEBUG),DEBUG=$(DEBUG))

clean:
	rm -rf sim
	make -C cosim clean
	make -C rtlsim clean

include ../Makefile.environment
