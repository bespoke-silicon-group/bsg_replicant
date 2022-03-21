# BSG Replicant Machines

This directory contains a variety of target machine configurations for
HammerBlade. Each sub-directory is a target with a valid
Makefile.machine.include file that specifies the machine
configuration.

To use a specific machine, change BSG_MACHINE_PATH to specify one of
the following directories in
[machine.mk](../machine.mk). BSG_MACHINE_PATH must contain a vaild
Makefile.machine.include file.

## Machines and Platforms

All machines are currently supported by the `bigblade-vcs`
platform. These simulate the current state-of-the-art HammerBlade
pod-based architecture.

- pod_X1Y1_ruche_X16Y8_hbm_one_pseudo_channel: HammerBlade architecture with 1 Manycore pod, with 128 RISC-V tiles arranged in a 16 x 8 grid (W x H), and connected to a single HBM2 pseudochannel.
- pod_X1_Y1_ruche_X16Y8_hbm: HammerBlade architecture with 1 Manycore pod, with 128 RISC-V tiles arranged in a 16 x 8 grid (W x H), and connected to two HBM channels.
- pod_X2_Y2_ruche_X16Y8_hbm: HammerBlade architecture with 4 Manycore pods arranged in a 2 x 2 grid, each with 128 RISC-V tiles arranged in a 16 x 8 grid (W x H), and connected to eight HBM channels.
- pod_X4_Y4_ruche_X16Y8_hbm: HammerBlade architecture with 16 Manycore pods arranged in a 4 x 4 grid, each with 128 RISC-V tiles arranged in a 16 x 8 grid (W x H), and connected to thirty-two HBM cxhannels.

- bigblade_pod_X1_Y1_ruche_X16Y8_hbm: HammerBlade architecture with 1 Manycore pod arrange, with 128 RISC-V tiles arranged in a 16 x 8 grid (W x H), and connected to an and an HBM2 memory system that mimics the Bigblade memory system.

## Parameters:

The following parameters in each Makefile.machine.include file can be changed:

### Core Configurations
- `BSG_MACHINE_PODS_X`: Number of HammerBlade Pods in the X dimension (width)
- `BSG_MACHINE_PODS_Y`: Number of HammerBlade Pods in the Y dimension (height)

- `BSG_MACHINE_POD_TILES_X`: Number of RV32 Cores in the X dimension (width)
- `BSG_MACHINE_POD_TILES_Y`: Number of RV32 Cores in the Y dimension (height)

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
