# BigBlade Verilator platform

This platform supports Linux and macOS hosts. On macOS, use current GNU Make
(`gmake`) and install `argp-standalone` with Homebrew. `VERILATOR_ROOT` must
refer to a configured Verilator source checkout. Start with the
`pod_X1Y1_ruche_X4Y2_hbm_one_pseudo_channel` machine for a small supported model.

- `exec/simsc`: fast execution with hardware profilers disabled
- `profile/simsc`: core, cache, router, and memory profiler output
- `debug/simsc`: FST waveform generation

Generated C++ is split at 10,000 statements by default. Override
`VERILATOR_OUTPUT_SPLIT` and `VERILATOR_OUTPUT_SPLIT_CFUNCS` if a larger model
needs smaller translation units.

## Simulation build policy

Execution (`exec/simsc`) defaults to **processor hierarchy**: Verilator compiles
`bsg_manycore_proc_vanilla` (scalar core plus endpoint) once per parameter
specialization and reuses the native code across independent tile states.
Routers and caches remain outside. No RTL source, hardware timing, or application
compiler changes are involved. The external control file is `processor.vlt`.

| Setting | Default / purpose |
|---|---|
| `VERILATOR_HIERARCHY` | `processor`; use `flat` for the previous code-generation path |
| `VERILATOR_THREADS` | `1` with processor hierarchy on Linux and macOS; other values are rejected in this mode |
| `VERILATOR_OPT_FAST` | `-O2 -march=native` for generated hot-category code |
| `VERILATOR_OPT_SLOW` | empty (unoptimized generated cold-category code) |
| `VERILATOR_OPT_GLOBAL` | `-Os` for Verilator support code |

`profile/simsc` and `debug/simsc` remain flat; profiling/tracing binds access
internal signals across the proposed boundary. Processor execution disables
the optional remote-operation tracer using **both** existing configuration and
bind-disabling macros. Ordinary correctness and kernel-marker output remain.
Use the profile model for hardware counters. Flat execution retains its previous
tracer policy and OS-specific worker default (Linux 16, macOS 1); explicitly set
`VERILATOR_THREADS=1` for matched comparisons.

## Build on Linux or macOS

Use the normal machine/example make target, with Python 3, GNU Make and Verilator
5.050 or newer. No Mac-only binary or compiler path is required. From an example
directory with the usual Bladerunner environment, for example:

```sh
make -j8 BSG_PLATFORM=bigblade-verilator CXX=g++ CC=gcc \
  "$BSG_MACHINE_PATH/bigblade-verilator/exec/simsc"
# Escape hatch, including older Verilator installations:
make -j8 BSG_PLATFORM=bigblade-verilator VERILATOR_HIERARCHY=flat \
  VERILATOR_THREADS=1 CXX=g++ CC=gcc \
  "$BSG_MACHINE_PATH/bigblade-verilator/exec/simsc"
```

Use `gmake` and `CXX=clang++ CC=clang` where appropriate. `-j8` controls build
parallelism, not simulator workers. `-march=native` means the executable must be
rebuilt for a different host CPU. Preserve active models: use a separate RP
checkout and machine/library destinations for experiments, not just a new run
directory. Never clean shared infrastructure while another process uses it.

The `.verilator_config` stamp regenerates execution code when mode, worker count,
compiler/optimization strings or generation flags change. A separate support
stamp tracks the support compiler/options. Regeneration forces
generated-file timestamps so old compiled objects are not silently reused;
Verilator's archive rule reconstructs the archive including child libraries.
An unchanged invocation does not rebuild. Tool/source replacement still requires
normal provenance checks; the stamp is not a complete content-addressed cache.

## Version guard and structural validation

Verilator 5.050 at `848d926ebd4addacacd294dc84e35d9d4ae8078c` can generate a
parameterized child library but leave the parent flat. A correctness test alone
does not detect this. `hierarchy.py` requires both parent DPI imports and actual
parent C++ calls to the processor wrapper, and records `hierarchy.json`.

For **5.050 only**, the helper supplies the comment-only `empty-params.v` through
the internal `--hierarchical-params-file` option. In that release,
`ParameterizedHierBlocks` in `src/V3Param.cpp` gates wrapper substitution on a
nonempty `hierParamFile`, while `src/V3HierBlock.cpp` supplies the file only for
type parameters. This workaround makes numeric-parameter reuse effective without
patching the tool. Later versions receive no workaround and must pass the same
structural check; they have not been performance-qualified here. Failures are
explicit, never silent flat fallbacks. Use `VERILATOR_HIERARCHY=flat` if needed.

Run policy tests from the RP root:

```sh
python3 tests/test_verilator_hierarchy.py
TEST_VERILATOR="$VERILATOR_ROOT/bin/verilator" CXX=clang++ \
  python3 tests/test_verilator_hierarchy.py
python3 tests/test_dpi_defines.py
```

The optional real-tool test builds a small numeric-parameter design through the
actual link rules, switches processor → flat → processor, checks archive symbols
and unchanged-build reuse. Mock Linux/Darwin policy checks do not constitute a
Linux runtime benchmark. See the handbook's **Simulation time optimization**
guide for controlled 16×8 PageRank/AES measurements and limitations.
