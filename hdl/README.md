# `bsg_bladerunner.rom` file format

| lines                     | Description |
| ---------------------     | ----------- |
| `MC_VERSION_ID`           | manycore version ID used in the bladerunner project |
| `COMPILATION_DATA`        | compilation date|
| `NETWORK_ADDR_WIDTH`      | manycore network address width |
| `NETWORK_DATA_WIDTH`      | manycore network data width |
| `NETWORK_DIMENSION_X`     | manycore network X dimension, number of columns of the mesh node |
| `NETWORK_DIMENSION_X`     | manycore network Y dimension, number of rows of the mesh node |
| `HOST_INTERFACE_COORD_X`  | the X location of the host node in the manycore mesh network |
| `HOST_INTERFACE_COORD_Y`  | the Y location of the host node in the manycore mesh network |
| `DESCRIPTION_PTR`         | description pointer, TBD |
| `BSG_IP_CORES_HASH`       | `bsg_ip_cores` commit ID |
| `BSG_MANYCORE_HASH`       | `bsg_manycore` commit ID |
| `BSG_F1_HASH`             | `bsg_f1` commit ID |


Note: 

1) We use 32 bits raw value rather than ASCII to record the configurations.

2) The corresponding verilog file is generated using BaseJump STL. (`bsg_ip_cores/bsg_mem/bsg_ascii_to_rom.py`)