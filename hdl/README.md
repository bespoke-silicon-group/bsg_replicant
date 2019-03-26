# bsg\_bladerunner.rom format

The bladerunner.rom file is used to initialize the rom in bsg\_bladerunner\_rom.v module.

Specific parameters for this module are defined in bsg_bladerunner\_rom\_pkg.vh

File format:

| lines                 | Description |
| --------------------- | ----------- |
| `@0`                  | The first line defines the start address of the rom.|
| `MC_VERSION_ID`       | manycore version ID in bladerunner project |
| `COMPILATION_DATA`    | compilation date|
| `NETWORK_ADDR_WIDTH`  | manycore network address width parameter |
| `NETWORK_DATA_WIDTH`  | manycore network data width parameter |
| `NETWORK_DIMENSION`   | manycore network dimension: value[31:16]=num\_tiles\_y, value[31:16]=num\_tiles\_x |
| `HOST_NODE_LOCATION`  | the location of the host node in the manycore mesh network: value[31:16]=y\_cord, value[15:0]=x\_cord|
| `DESCRIPTION_PTR`     | description pointer, TBD |
| `BSG_IP_CORES_HASH`   | bsg\_ip\_cores commit ID |
| `BSG_MANYCORE_HASH`   | bsg\_manycore commit ID |
| `BSG_F1_HASH`         | bsg\_f1 commit ID |


Note: 

1) We use 32 bits raw value rather than ASCII to record the configurations.

2) TODO: Generate the rom file in python script.
