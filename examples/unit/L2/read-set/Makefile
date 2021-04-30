REPLICANT_PATH:=$(shell git rev-parse --show-toplevel)

include $(REPLICANT_PATH)/environment.mk
include $(BSG_MACHINE_PATH)/Makefile.machine.include

# $1 = pod-x
# $2 = pod-y
# $3 = south-not-north
# $4 = vcache-x
# $5 = set
all:

test-name = pod-x-$1_pod-y-$2_south-not-north-$3_vcache-x-$4_set-$5
define test
.PHONY: generate-$(call test-name,$1,$2,$3,$4,$5)
generate-$(call test-name,$1,$2,$3,$4,$5): run/$(call test-name,$1,$2,$3,$4,$5)/main.cpp
generate-$(call test-name,$1,$2,$3,$4,$5): run/$(call test-name,$1,$2,$3,$4,$5)/Makefile
run/$(call test-name,$1,$2,$3,$4,$5)/main.cpp: main.cpp
	@mkdir -p $$(dir $$@)
	cp $$< $$(dir $$@)

run/$(call test-name,$1,$2,$3,$4,$5)/Makefile: template.mk
	@mkdir -p $$(dir $$@)
	echo "C_ARGS = $1 $2 $3 $4 $5" > $$@
	cat $$< >> $$@

.PHONY: run-$(call test-name,$1,$2,$3,$4,$5)
run-$(call test-name,$1,$2,$3,$4,$5): generate-$(call test-name,$1,$2,$3,$4,$5)
	$(MAKE) -C run/$(call test-name,$1,$2,$3,$4,$5) main.exec.log

tests     += run-$(call test-name,$1,$2,$3,$4,$5)
generates += generate-$(call test-name,$1,$2,$3,$4,$5)

endef

range = $(shell echo {0..$(shell echo $1-1|bc)})

$(foreach pod-x,$(call range,$(BSG_MACHINE_PODS_X)),\
$(foreach pod-y,$(call range,$(BSG_MACHINE_PODS_Y)),\
$(foreach south-not-north,$(call range,2),\
$(foreach l2-x,$(call range,$(BSG_MACHINE_POD_TILES_X)),\
$(foreach set,$(call range,$(BSG_MACHINE_VCACHE_SET)),\
$(eval $(call test,0,0,$(south-not-north),$(l2-x),$(set))))))))

$(eval $(call test,0,0,0,0,0))

.PHONY: clean all debug generate

debug:
	@echo $(tests)

all: $(tests)
generate: $(generates)

clean:
	rm -rf run
