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

These machines are supported by the `dpi-verilator` and `dpi-vcs` platform:

- timing_*
- baseline_*
- infinite_*

The `aws-fpga` and `aws-vcs` platform only supports the
4x4_blocking_vcache_f1_model  4x4_blocking_vcache_f1_dram

## Performance Metric Machines

- timing_* machines: Manycore pod with caches on top and bottom. There
  are 16 caches per HBM channel. Each cache is mapped to a separate
  channel bank within its respective channel. 

  ruche indicates a _ruched_ mesh network. Mesh indicates a
  traditional 2-D mesh network.

  *This configuration should be the default for taking performance
  measurements and making decisions based on them.*

- infinite_* machines: Manycore pod with infinite, single-cycle
  memories on top and bottom of each row. 

  ruche indicates a _ruched_ mesh network. Mesh indicates a
  traditional 2-D mesh network.

  *This configuration should be used to estimate the impact of memory
  latency.*

- baseline_* machines: Manycore pod with infinite, single-cycle
  memories on top and bottom of each row. 

  The baseline designs use a crossbar network.

  *This configuration should be used to estimate the impact of an
  ideal network*

## Correctness Machines

**DO NOT USE THESE MACHINES FOR PERFORMANCE METRICS**

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


## Parameters:

The following parameters in each Makefile.machine.include file can be changed:


### Core Configurations
- `BSG_MACHINE_NUM_CORES_X`: Number of RV32 Cores in the X dimension (width)
- `BSG_MACHINE_NUM_CORES_Y`: Number of RV32 Cores in the Y dimension (height)

- `BSG_MACHINE_HETERO_TYPE_VEC`: Heterogeneous tile composition. Must
be a 1-d array equal to the number of tiles, or shorthand
(default:0). Each value corresponds to a tile type in
[bsg_manycore_hetero_socket.v](https://github.com/bespoke-silicon-group/bsg_manycore/blob/master/v/bsg_manycore_hetero_socket.v). 0
is for Vanilla Core (RV32), and anything else should be described in
the relevant Makefile.machine.include file.

### Network Parameters
- `BSG_MACHINE_NETWORK_CFG`: Network configuration. Valid values are : e_network_mesh, e_network_crossbar, and e_network_half_ruche_x
valid values are defined in [bsg_machine_network_cfg_pkg.v](https://github.com/bespoke-silicon-group/bsg_manycore/blob/master/testbenches/common/v/bsg_manycore_network_cfg_pkg.v)
- `BSG_MACHINE_RUCHE_FACTOR_X`: X-dimension ruche factor. Only applies when `BSG_MACHINE_NETWORK_CFG` is `e_network_half_ruche_x`

### Memory System Parameters
- `BSG_MACHINE_DRAM_INCLUDED`: Defines whether the DRAM interface is used. Default is 1. 
- `BSG_MACHINE_MEM_CFG`: Defines memory system configuration as a triple (Cache, Interface, Type). Values are defined and explained in (bsg_bladerunner_mem_cfg_pkg.v)[https://github.com/bespoke-silicon-group/bsg_replicant/blob/master/hardware/bsg_bladerunner_mem_cfg_pkg.v].

- `BSG_MACHINE_VCACHE_PER_DRAM_CHANNEL`: Defines number of Last-Level Caches per DRAM channel.
- `BSG_MACHINE_VCACHE_SET`: Number of sets in each Last-Level Cache
- `BSG_MACHINE_VCACHE_WAY`: Number of ways (associativity) in each Last-Level Cache
- `BSG_MACHINE_VCACHE_LINE_WORDS`: Number of words in each cache line
