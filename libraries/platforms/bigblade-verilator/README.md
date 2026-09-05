# BigBlade Verilator platform

This platform supports Linux and macOS hosts. On macOS, use current GNU Make
(`gmake`) and install `argp-standalone` with Homebrew. `VERILATOR_ROOT` must
refer to a configured Verilator source checkout.

Start with the `pod_X1Y1_ruche_X4Y2_hbm_one_pseudo_channel` machine. The
available simulator variants are:

- `exec/simsc`: fast execution with hardware profilers disabled
- `profile/simsc`: core, cache, router, and memory profiler output
- `debug/simsc`: FST waveform generation

The execution model defaults to one Verilator worker thread on macOS and 16 on
other hosts. Override it when invoking an application target, for example
`gmake VERILATOR_THREADS=4 exec.log`. Small models can run slower when host
threading overhead exceeds the available parallel work, so benchmark before
increasing the macOS default.

The requested worker count is stored with the generated execution model.
Changing `VERILATOR_THREADS` invalidates and rebuilds that model, preventing an
existing model with a different thread count from being reused silently.

Generated C++ is split at 10,000 statements by default to keep individual
translation units tractable for Clang. Override `VERILATOR_OUTPUT_SPLIT` and
`VERILATOR_OUTPUT_SPLIT_CFUNCS` at make time if a larger machine needs smaller
translation units.
