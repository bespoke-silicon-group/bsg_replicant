# SPMD (Single Program Multiple Data)

This directory runs regression tests of Manycore functionality using
the CUDA Lite runtime libraries. Each test is a .c/.h, or a .cpp/.hpp
file pair, located in the `regression/spmd` directory.

To run all tests in an appropriately configured environment, run:

```make regression``` 

Device C/C++ sources use GCC by default. For LLVM 22, pair this wrapper with
[`bsg_manycore:hammerblade-llvm22-support`](https://github.com/bespoke-silicon-group/bsg_manycore/tree/hammerblade-llvm22-support)
at `2725e68c7ee8` or a compatible descendant and the
[`hammerblade-llvm22` compiler](https://github.com/bespoke-silicon-group/llvm-project/tree/hammerblade-llvm22)
at `d2ebb0fda484` (LLVM 22.1.8). From an appropriately configured, isolated
SPMD checkout, the compiler-selection options are:

```sh
gmake regression SPMD_COMPILER=llvm LLVM_DIR=/path/to/llvm22-build \
  LLVM_LLC_FILETYPE=obj LLVM_OPT_OPTS=-enable-dfa-jump-thread OPT_LEVEL=-O3
```

`LLVM_LLC_FILETYPE=obj` avoids sending modern LLVM assembly through the older
GNU assembler. The SDK still supplies runtime objects and the final linker.
The legacy `CLANG=1` spelling remains supported; LLVM 10's assembly path can
still be selected by omitting the object-output option. Use fresh or explicitly
cleaned task-owned device build products when switching compilers: objects in
`bsg_manycore/software/spmd` are shared by wrappers using that checkout, and
changing flags alone does not invalidate them.

The command selects a compiler, not a compatible physical machine or a tested
test subset. The published LLVM 22 validation covers 15 selected SPMD tests on
physical 2x1, not every target discovered by `regression`. Preserve each test's
tile-group requirements and numerical flags when extending the selection.

Or, alternatively, run `make help` to see a list of available targets.
