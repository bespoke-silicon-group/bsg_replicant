REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

include $(REPLICANT_PATH)/environment.mk
include $(BSG_MACHINE_PATH)/Makefile.machine.include

# $1 = pod-x
# $2 = pod-y
# $3 = south-not-north
# $4 = vcache-x
all:

# range function
range = $(shell echo {0..$(shell echo $1-1|bc)})
# test name function
test-name = pod-x-$1_pod-y-$2_south-not-north-$3_vcache-x-$4
define test
.PHONY: generate-$(call test-name,$1,$2,$3,$4)
generate-$(call test-name,$1,$2,$3,$4): run/$(call test-name,$1,$2,$3,$4)/main.cpp
generate-$(call test-name,$1,$2,$3,$4): run/$(call test-name,$1,$2,$3,$4)/Makefile
generate-$(call test-name,$1,$2,$3,$4): run/$(call test-name,$1,$2,$3,$4)/unit.inc
run/$(call test-name,$1,$2,$3,$4)/main.cpp: main.cpp
	@mkdir -p $$(dir $$@)
	cp $$< $$(dir $$@)

run/$(call test-name,$1,$2,$3,$4)/Makefile: template.mk
	@mkdir -p $$(dir $$@)
	cat $$< >> $$@

run/$(call test-name,$1,$2,$3,$4)/unit.inc:
	@mkdir -p $$(dir $$@)
	touch $$@
	@$(foreach set,$(call range,$(BSG_MACHINE_VCACHE_SET)),\
	printf "DECLARE_TEST(%d,%d,%d,%d,%d)\n" $1 $2 $3 $4 $(set) >> $$@;)

.PHONY: run-$(call test-name,$1,$2,$3,$4)
run-$(call test-name,$1,$2,$3,$4): generate-$(call test-name,$1,$2,$3,$4)
	$(MAKE) -C run/$(call test-name,$1,$2,$3,$4) main.exec.log

tests     += run-$(call test-name,$1,$2,$3,$4)
generates += generate-$(call test-name,$1,$2,$3,$4)

endef

$(foreach pod-x,$(call range,$(BSG_MACHINE_PODS_X)),\
$(foreach pod-y,$(call range,$(BSG_MACHINE_PODS_Y)),\
$(foreach south-not-north,$(call range,2),\
$(foreach l2-x,$(call range,$(BSG_MACHINE_POD_TILES_X)),\
$(eval $(call test,$(pod-x),$(pod-y),$(south-not-north),$(l2-x)))))))


.PHONY: clean all debug generate

debug:
	@echo $(tests)

all: $(tests)
generate: $(generates)

clean:
	rm -rf run
