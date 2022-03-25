# AES (Advanced Encryption Standard)

This HB App implements the AES algorithm.

The code for this implementation of AES is the
[tiny-AES-c](https://github.com/kokke/tiny-AES-c) project. This is a
lightweight, small footprint implementation that fits in the
HammerBlade instruction cache.

# Quick-Start:

1. Clone the submodule, tiny-AES-c: `git submodule update --init --checkout tiny-AES-c`
2. From one of the four sub-directories, run the typical HB flow:
   - `unopt-single`: Unoptimized version of AES, running on a single tile
   - `unopt-pod`: Unoptimized version of AES, running on an entire pod
   - `opt-single`: Optimized version of AES, running on a single tile. 
   - `opt-pod`: Optimized of AES, running on an entire pod

# Versions:

As described above, there are four versions of the AES code.

`unopt-single` and `unopt-pod` are unmodified implementations of the AES code.

`opt-single` and `opt-pod` have two optimizations.

1. The sbox look-up table is located in scratchpad memory/DMEM. (See Line 85 in aes.c)
2. Memory copies from DRAM are batched, and performed as 32 bit reads to increase network efficiency (See Line 518 in aes.c)


The default is to run 4 iterations of a 1204 byte encryption.