# bsg_bladerunner.rom format

file parameters are defined in bsg_bladerunner_rom_pkg.vh

content of the file:

| lines                 | Description |
| --------------------- | ----------- |
| `@0`                  | The first line defines the start address of the rom.|
| `MC_VERSION_ID`       | manycore version ID in bladerunner project |
| `COMPILATION_DATA`    | compilation date|
| `NETWORK_ADDR_WIDTH`  | manycore network address width parameter |
| `NETWORK_DATA_WIDTH`  | manycore network data width parameter |
| `NETWORK_DIMENSION`   | manycore network dimension |
| `HOST_NODE_LOCATION`  | the location of the host node in the manycore mesh network: loc[31:16]=y_cord, loc[15:0]=x_cord|
| `DESCRIPTION_PTR`     | description pointer, TBD |
| `BSG_IP_CORES_HASH`   | bsg_ip_cores commit ID |
| `BSG_MANYCORE_HASH`   | bsg_manycore commit ID |
| `BSG_F1_HASH`         | bsg_f1 commit ID |


Note: we use 32 bits raw value rather than ASCII to record the configurations.
