# BigBlade Verilator platform

This platform supports Linux and macOS hosts. On macOS, use current GNU Make
(`gmake`) and install `argp-standalone` with Homebrew. `VERILATOR_ROOT` must
refer to a configured Verilator source checkout. Start with the
`pod_X1Y1_ruche_X4Y2_hbm_one_pseudo_channel` machine for a small supported model.

| Build-only target | Application run target | Model directory | Instrumentation |
|---|---|---|---|
| `sim-exec` | `exec.log` | `exec/` | Fast execution; hardware profilers, DRAM statistics and instruction text disabled |
| `sim-profile` | `profile.log` | `profile/` | Core/cache counters, PC histogram and memory profiling; **no `vanilla.log` or BloodGraph** |
| `sim-trace` | `trace.log` | `trace/` | Profile instrumentation **plus `vanilla.log`** instruction text and BloodGraph |
| `sim-debug` | `debug.log` or `debug.fst` | `debug/` | Flat model with FST waveforms, profiling and BloodGraph |

Each model has its own generated code, configuration stamp and `simsc` binary
under `$BSG_MACHINE_PATH/bigblade-verilator`. Selecting trace does not replace
the profile binary. Existing full-path build targets remain valid.

**Migration:** older profile builds could emit instruction text. Rebuild profile
with these rules for the no-text policy; use `sim-trace` / `trace.log` when that
text is required. The trace target filters the legacy
`VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE` define from `VDEFINES`, including its
`=value` form, and explicitly enables the testbench text observer. It does not
override independently disabled core/cache/PC observers.

DRAMSim3 follows the selected mode: `exec` links `libdramsim3_exec.so`, built
without BloodGraph, counter/histogram collection, or DRAM statistics output.
`profile` links `libdramsim3_profile.so`, which collects ordinary DRAM statistics
without BloodGraph. `trace` and `debug` link `libdramsim3_trace.so`, adding
BloodGraph statistics and its per-cycle bank-state trace (`blood_graph_ch*.log`).
Those detailed DRAM traces start at initialization and are independent of the
HammerBlade runtime operation-trace enable. Separate DRAM command traces and
BloodGraph periodic-statistics dumping are not enabled automatically.
DRAM command scheduling, refresh, self-refresh and completion timing remain
active in every mode. Lightweight HammerBlade tile markers also remain.

All DRAM libraries default to `DRAMSIM3_OPT_FLAGS=-O2`, independently of
`VERILATOR_OPT_FAST`. Override that variable for controlled experiments, for
example `DRAMSIM3_OPT_FLAGS=-O0`. Compiler/flag changes rebuild the library and
relink its dependent model; an unchanged build is a no-op. The common Verilator
runtime deliberately has no DRAMSim3 dependency: the executable chooses exactly
one library with a distinct SONAME/install name, so building another mode does
not change an existing mode's statistics policy. Other simulator platforms
retain `libdramsim3.so` with their existing statistics and BloodGraph counters,
without the detailed BloodGraph trace.

This requires DRAMSim3's `DRAMSIM3_STATISTICS_CONTROL` support; `exec` rejects an
older BaseJump DRAMSim3 submodule with an update message. The disabled-statistics
build is incompatible with the separate DRAMSim3 thermal model, which requires
statistics. Use updated compatible submodule pins when distributing the change.

Instruction text is emitted after reset, independently of runtime operation
tracing. `hb_mc_manycore_log_enable()` controls operation CSVs in profile/trace;
it is not an on/off switch for `vanilla.log`. Match PCs against the device ELF:
the legacy logger can show zero FP instruction bits and predecoded branch/jump
bits, so it is not a byte-exact disassembly stream.

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
| `VERILATOR_PROFILE_HIERARCHY` | follows `VERILATOR_HIERARCHY`; `flat` keeps profile and trace flat |
| `VERILATOR_THREADS` | `1` with processor hierarchy on Linux and macOS; other values are rejected in this mode |
| `VERILATOR_OPT_FAST` | `-O2 -march=native` for generated hot-category code |
| `VERILATOR_OPT_SLOW` | empty (unoptimized generated cold-category code) |
| `VERILATOR_OPT_GLOBAL` | `-Os` for Verilator support code |

`profile/simsc` and `trace/simsc` also reuse processor+endpoint code, through the existing
`bsg_manycore_hetero_socket` shell. Four **simulation-only** ports carry the
global counter, marker valid/tag, and trace enable into this boundary. Bound
core and remote-operation observers consume those signals locally; processor
and endpoint execution logic and interfaces are unchanged. This needs the
`BSG_VERILATOR_PROFILE_PORTS` support in `bsg_manycore`; an older checkout fails
with an explicit update/fallback message. The macro is enabled only for the
optimized profile/trace builds. Ordinary flat and synthesis builds keep their original
ports and bindings. Waveform `debug/simsc` remains flat.

Processor execution disables
the optional remote-operation tracer using **both** existing configuration and
bind-disabling macros. Ordinary correctness and kernel-marker output remain.
Use the profile model for hardware counters. Flat execution retains its previous
tracer policy and OS-specific worker default (Linux 16, macOS 1); explicitly set
`VERILATOR_THREADS=1` for matched comparisons.
Profiling stays single-worker. Core/cache counters, PC histograms and runtime
operation tracing remain enabled in optimized profiling; the Verilator router
profiler remains unavailable under its existing RTL guard, unrelated to hierarchy.

## Build on Linux or macOS

Use the normal machine/example make target, with Python 3, GNU Make and Verilator
5.050 or newer. No Mac-only binary or compiler path is required. From an example
directory with the usual Bladerunner environment, for example:

```sh
make -j8 BSG_PLATFORM=bigblade-verilator CXX=g++ CC=gcc \
  "$BSG_MACHINE_PATH/bigblade-verilator/exec/simsc"
# Build all four independently named models without running an application:
make -j8 BSG_PLATFORM=bigblade-verilator CXX=g++ CC=gcc \
  sim-exec sim-profile sim-trace sim-debug
# Run the current example (select one):
make BSG_PLATFORM=bigblade-verilator profile.log
make BSG_PLATFORM=bigblade-verilator trace.log
# If bsg_manycore has not yet been updated, keep profile/trace flat:
make -j8 BSG_PLATFORM=bigblade-verilator VERILATOR_PROFILE_HIERARCHY=flat \
  CXX=g++ CC=gcc "$BSG_MACHINE_PATH/bigblade-verilator/profile/simsc"
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

Run outputs are **not** isolated automatically by model selection. The `.log`
targets retain the application's current working directory and `C_ARGS` for
compatibility with relative inputs. Use separate application/run directories
for variants or concurrent processes: counters, `vanilla.log`, DRAM files and
`debug.fst` have common names. Existing `.log` files may be considered up to date
by Make; a fresh run directory avoids confusing an old log with a new run.

Modern Verilator FST debug builds require LZ4 development files (`brew install
lz4` on macOS; the distribution's LZ4 development package on Linux). They are
not required by exec/profile/trace. For a nonstandard installation, set
`VERILATOR_FST_CPPFLAGS` and `VERILATOR_FST_LDLIBS`; on macOS the default discovers
Homebrew's LZ4 prefix. Older writers with bundled compression retain their
existing dependencies.

The per-model `.verilator_config` stamp regenerates code when mode, worker count,
compiler/optimization strings or generation flags change. A separate support
stamp tracks the support compiler/options. Regeneration forces
generated-file timestamps so old compiled objects are not silently reused;
Verilator's archive rule reconstructs the archive including child libraries.
An unchanged invocation does not rebuild. Tool/source replacement still requires
normal provenance checks; the stamp is not a complete content-addressed cache.

## Version guard and structural validation

Verilator 5.050 at `848d926ebd4addacacd294dc84e35d9d4ae8078c` and 5.052 at
`ea338be98e1e838d3518809ce8899f85a009963c` can generate a
parameterized child library but leave the parent flat. A correctness test alone
does not detect this. `hierarchy.py` requires both parent DPI imports and actual
parent C++ calls to the processor wrapper, and records `hierarchy.json`.

For **5.050 and 5.052 only**, the helper supplies the comment-only `empty-params.v` through
the internal `--hierarchical-params-file` option. In these releases,
`ParameterizedHierBlocks` in `src/V3Param.cpp` gates wrapper substitution on a
nonempty `hierParamFile`, while `src/V3HierBlock.cpp` supplies the file only for
type parameters. This workaround makes numeric-parameter reuse effective without
patching the tool. Later versions receive no workaround and must pass the same
structural check; they have not been performance-qualified here. Failures are
explicit, never silent flat fallbacks. Use `VERILATOR_HIERARCHY=flat` if needed.

The helper creates `bsg_unified_dpi.cpp` from the active parent/child export
wrappers. Verilator's generated per-export guards coalesce common C names such
as `bsg_dpi_init`, while scope dispatch still selects each instance's callback.
Generated archive builds exclude only these C wrappers from `VM_FAST`; callbacks
stay in the archives. This also handles small unity builds, which otherwise
produce duplicate symbols. The unified object is linked explicitly, so weak
runtime references cannot silently drop the child's profiler API. There is no
shared profiler state, generated C++ rewriting, or duplicate-symbol linker flag.
Hierarchical children add their specialization name to DPI scope paths; tools
that inspect those paths must use the registered scopes. The existing public
`hb_mc_platform_get_icount` remains `HB_MC_NOIMPL` on this platform; this change
does not implement that separate API.

Run policy tests from the RP root:

```sh
BASEJUMP_STL_DIR=/path/to/basejump_stl CXX=clang++ python3 tests/test_dramsim3_modes.py
python3 tests/test_verilator_hierarchy.py
TEST_VERILATOR="$VERILATOR_ROOT/bin/verilator" CXX=clang++ \
  python3 tests/test_verilator_hierarchy.py
TEST_VERILATOR="$VERILATOR_ROOT/bin/verilator" CXX=clang++ \
  python3 tests/test_verilator_profile_boundary.py
python3 tests/test_dpi_defines.py
```

The optional real-tool test builds a small numeric-parameter design through the
actual link rules, switches processor → flat → processor, checks archive symbols
and unchanged-build reuse. The profile test checks both clock edges, changing
marker controls, trace transitions, independent child state and shared-name DPI
exports in small flat/hierarchical native executables.
Mock Linux/Darwin policy checks do not constitute a
Linux runtime benchmark. See the handbook's **Simulation time optimization**
guide for controlled 16×8 PageRank/AES measurements and limitations.
