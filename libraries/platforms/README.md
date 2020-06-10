# Platforms

HammerBlade runs on a variety of platforms: AWS F1 (natively),
Synopsys VCS (simulation), and others. Each platform requires a
different runtime library, hardware files, and build commands.
We abstract these differences in the makefiles. 

Available platforms are subdirectories in this directory: aws-fpga,
aws-vcs. Each platform provides its own `bsg_manycore_platform.cpp`
file that implements the API in `bsg_manycore_platform.h`. Each
platform also provides a `library.mk` with build rules.

To switch platforms, set the variable `BSG_PLATFORM` in
[platform.mk](../../platform.mk). For the most part, users
do not need to worry about what platform they are running on;
`platform.mk` will automatically deduce the correct platform from
the environment

## Notes

The aws-fpga and aws-vcs platforms are identical EXCEPT for how they
manage MMIO-based reads/writes. aws-vcs uses a DPI interface, while
aws-fpga uses the utilities provided by the kernel (namely,
mmap). Therefore, in aws-vcs we reuse the `bsg_manycore_platform.cpp`
file in aws-fpga, but procide our own 1bsg_manycore_mmio.cpp` file that
handles DPI-based MMIO.
