REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

include $(REPLICANT_PATH)/environment.mk
include $(BSG_MACHINE_PATH)/Makefile.machine.include

# $1 = pod-x
# $2 = pod-y
# $3 = tile-x
# $4 = tile-y
all:

# range function
range = $(shell echo {0..$(shell echo $1-1|bc)})
device.S:
	@printf "#include <bsg_manycore_arch.h>\n" > $@
	@printf "#include <bsg_manycore_asm.h>\n" >> $@
	@printf ".text\n" >> $@
	@printf "__start:\n" >> $@
	@$(foreach _,$(call range,1019),printf "\tj __end\n" >> $@;)
	@printf "__end:\n" >> $@
	@printf "\tbsg_asm_finish(IO_X_INDEX, 0)" >> $@
	@printf "__loop: j __loop\n" >> $@

# test name function
test-name = pod-x-$1_pod-y-$2_tile-x-$3_tile-y-$4
define test
.PHONY: generate-$(call test-name,$1,$2,$3,$4)
generate-$(call test-name,$1,$2,$3,$4): device.S
generate-$(call test-name,$1,$2,$3,$4): run/$(call test-name,$1,$2,$3,$4)/Makefile
generate-$(call test-name,$1,$2,$3,$4): run/$(call test-name,$1,$2,$3,$4)/unit.inc

run/$(call test-name,$1,$2,$3,$4)/Makefile: template.mk
	@mkdir -p $$(dir $$@)
	cat $$< > $$@
	@echo "C_ARGS += $(EXAMPLES_PATH)/run/$(call test-name,$1,$2,$3,$4)/device.riscv" >> $$@

run/$(call test-name,$1,$2,$3,$4)/unit.inc:
	@mkdir -p $$(dir $$@)
	touch $$@
	@$(foreach word,$(call range,1019),\
	printf "DECLARE_TEST(%d,%d,%d,%d,%d)\n" $1 $2 $3 $4 $(word) >> $$@;)

.PHONY: run-$(call test-name,$1,$2,$3,$4)
run-$(call test-name,$1,$2,$3,$4): generate-$(call test-name,$1,$2,$3,$4)
	$(MAKE) -C run/$(call test-name,$1,$2,$3,$4) main.exec.log

run       += run-$(call test-name,$1,$2,$3,$4)
generates += generate-$(call test-name,$1,$2,$3,$4)
tests     += $(call test-name,$1,$2,$3,$4)
endef

# $(foreach pod-x,$(call range,$(BSG_MACHINE_PODS_X)),\
# $(foreach pod-y,$(call range,$(BSG_MACHINE_PODS_Y)),\
# $(foreach tile-x,$(call range,$(BSG_MACHINE_POD_TILES_X)),\
# $(foreach tile-y,$(call range,$(BSG_MACHINE_POD_TILES_Y)),\
# $(eval $(call test,$(pod-x),$(pod-y),$(tile-x),$(tile-y)))))))
$(eval $(call test,0,0,0,0))

.PHONY: clean all debug generate report.txt

report.txt: $(run)
	@rm -f $@.tmp
	@touch $@.tmp
	@$(foreach t,$(tests),\
	if [ -z `grep "FAILED" run/$t/main.exec.log` ]; then\
		printf "$(t): PASSED\n" >> $@.tmp;\
	else\
		printf "$(t): FAILED\n" >> $@.tmp;\
	fi;)
	@echo "===================================================" >  $@
	@printf "\t\t%3d tests passed\n" `grep "PASSED" $@.tmp | wc -l` >> $@
	@printf "\t\t%3d tests failed\n" `grep "FAILED" $@.tmp | wc -l` >> $@
	@echo "===================================================" >> $@
	@cat $@.tmp >> $@

debug:
	@echo $(tests)

all: $(run)
generate: $(generates)

clean:
	rm -f device.S
	rm -rf run
