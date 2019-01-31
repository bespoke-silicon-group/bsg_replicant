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
	make -C $@ $(CHECK_RULE) PROJECT=$(PROJECT) $(if $(DEBUG),DEBUG=$(DEBUG))

clean:
	make -C cosim clean
	make -C rtlsim clean
	rm -rf {rtlsim,cosim}/dpi.so
