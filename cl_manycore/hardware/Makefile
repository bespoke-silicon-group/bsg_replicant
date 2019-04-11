include ../Makefile.dimensions

all: bsg_bladerunner_configuration.v cl_manycore_pkg.v

bsg_bladerunner_configuration.rom:
	python create_bladerunner_rom.py >> $(subst v,rom,$@)

bsg_bladerunner_configuration.v: envcheck bsg_bladerunner_configuration.rom
	python $(BSG_IP_CORES_DIR)/bsg_mem/bsg_ascii_to_rom.py \
		$(subst v,rom,$@) bsg_bladerunner_configuration >> $@

cl_manycore_pkg.v: envcheck $(CL_DIR)/Makefile.dimensions
	sed "s/CL_MANYCORE_DIM_X/$(CL_MANYCORE_DIM_X)/ ;\
		s/CL_MANYCORE_DIM_Y/$(CL_MANYCORE_DIM_Y)/" \
		$(subst v,template,$@) > $@

clean:
	rm -f bsg_bladerunner_configuration.{rom,v}
	rm -f cl_manycore_pkg.v

include ../Makefile.environment
