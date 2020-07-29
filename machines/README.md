# BSG Replicant Machines

This directory contains a variety of target machine configurations for
HammerBlade. Each sub-directory is a target with a valid
Makefile.machine.include file. The Makefile.machine.include specifies
the machine configuration.

To use a specific machine, change BSG_MACHINE_PATH to specify one of
the following directories in
[machine.mk](../machine.mk). BSG_MACHINE_PATH must contain a vaild
Makefile.machine.include file.

## Machines and Platforms

All machines are supported by the `aws-vcs` platform.

These machines are supported by the `dpi-verilator` and `dpi-vcs` platform:

- timing_v0_8_4
- timing_v0_16_8
- timing_v0_32_16
- timing_v0_64_32
- baseline_v0_8_4
- baseline_v0_16_8
- baseline_v0_32_16
- baseline_v0_64_32
- 4x4_fast_n_fake
- 16x8_fast_n_fake

The `aws-fpga` platform only supports the 4x4_blocking_vcache_f1_model. 

## Performance Metric Machines

- timing_v0_16_8 and baseline_v0_16_8: This is a 16x8 array of RISC-V
  Vanilla Cores with caches on top and bottom. The top caches all
  share an HBM channel, and the bottom caches are mapped to a
  separate, independent, HBM channel. Each cache is mapped to a
  separate channel bank within its respective channel. *This
  configuration should be the default for taking performance
  measurements and making decisions based on them.*

- timing_v0_32_16 and baseline_v0_32_16: Same as
  timing/baseline_v0_16_8, but a 32x16 array of tiles. There are four
  HBM channels.

- timing_v0_64_32 and baseline_v0_64_32: Same as
  timing/baseline_v0_16_8, but a 64x32 pod. There are eight HBM
  channels.

- timing_v0_8_4 and baseline_v0_8_4: Same as timing/baseline_v0_16_8,
  but a 8x4 pod. There is one HBM channel. This is probably not a
  sensible configuration, as the tile to channel ratio is very
  low. But for memory bound benchmarks, it may be reasonable to use
  for iteration.

The baseline designs use a crossbar network to access memory.

## Correctness Machines

**DO NOT USE THESE MACHINES FOR PERFORMANCE METRICS**

- 4x4_fast_n_fake: this is a 4x4 array of RISC-V Vanilla Cores with
  1-cycle infinite memories attached directly to network links on
  top-and-bottom. This is used for getting code running and debugged,
  but no performance analysis should be done, and no optimization
  decisions made based on this model.

- 16x8_fast_n_fake: this is like 4x4_fast_n_fake except the RISC-V
  Vanilla Core array is 16x8. This can be used for getting code running
  and debugged or observing performance with and ideal memory system.

- 4x4_blocking_vcache_f1_model: This is a 4x4 array of RISC-V Vanilla
  Cores with 1 FAKE DRAM DIM with bounded/low latency attached through
  an AXI-4 interface to the top and bottom of the array. This models
  the configuration on F1 and is used to build the F1 image. 

- 4x4_amo_support: Same as 4x4_blocking_vcache_f1_model

- 4x4_blocking_vcache_f1_dram: This is a 4x4 array of RISC-V Vanilla
  Cores with 1 DRAM DIM with bounded/low latency attached through an
  AXI-4 interface to the top and bottom of the array. The DRAM is a
  Micron model provided by Xilinx. This models the configuration on F1
  and is used to build the F1 image. *This machine simulates very
  slowly*

## Legacy Machines

**DO NOT USE THESE MACHINES**

- 4x4_blocking_dramsim3_hbm2_512mb: An early version of a DRAMSim3 HBM
  Architecture. Still used for regression testing but nothing else.

- 8x4_blocking_dramsim3_hbm2_4gb: An early version of a DRAMSim3 HBM
  Architecture. Still used for regression testing but nothing else.
