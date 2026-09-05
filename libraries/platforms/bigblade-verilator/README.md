# BigBlade Verilator platform

This platform supports Linux and macOS hosts. On macOS, use current GNU Make
(`gmake`) and install `argp-standalone` with Homebrew. `VERILATOR_ROOT` must
refer to a configured Verilator source checkout.

Start with the `pod_X1Y1_ruche_X4Y2_hbm_one_pseudo_channel` machine. The
available simulator variants are:

- `exec/simsc`: fast execution with hardware profilers disabled
- `profile/simsc`: core, cache, router, and memory profiler output
- `debug/simsc`: FST waveform generation

The execution model uses 16 Verilator worker threads by default. For a
conservative first build, set `VERILATOR_THREADS=1` when invoking an application
target, for example `gmake VERILATOR_THREADS=1 exec.log`. The thread count is
fixed when the model is generated; run `gmake platform.link.clean` before
rebuilding the same machine with a different value.

Generated C++ is split at 10,000 statements by default to keep individual
translation units tractable for Clang. Override `VERILATOR_OUTPUT_SPLIT` and
`VERILATOR_OUTPUT_SPLIT_CFUNCS` at make time if a larger machine needs smaller
translation units.
