IP_PATH = ./
IP_TARGETS = $(basename $(notdir $(wildcard $(IP_PATH)/*/*.xci)))

all: $(IP_TARGETS)


%.xml: %.xci
	vivado -mode batch -source ipgen.tcl -tclargs $(basename $(notdir $@))

.SECONDEXPANSION:
$(IP_TARGETS): $$@/$$@.xml

clean:
	rm -rf */*.veo */*.vho */*.xml */*.xdc *.log *.jou 
	rm -rf */simulation */synth */hdl */sim .Xil
