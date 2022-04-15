database -open debug -shm
probe -create replicant_tb.testbench.DUT -depth all -all -shm -database debug
run
exit
