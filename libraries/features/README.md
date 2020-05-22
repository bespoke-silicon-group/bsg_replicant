# Platform Features

This directory contains optional platform features that provide a similar API. 

For example: DMA. DMA is implemented in simulation using a "magic"
back-door to load data directly into simulated DRAM when using
C++-based memory models. The DMA driver for simulation is in the
[dma/simulation](dma/simulation) directory, and the API is in the
[dma](dma) directory.

However, some platforms do not implement these features. In this case,
each feature should provide a **noimpl** "driver" that simply returns
`HB_MC_NO_IMPL` for every API function call. For example, the
`aws-fpga` platform does not currently support DMA, so it reuses the
files in [dma/noimpl](dma/noimpl).