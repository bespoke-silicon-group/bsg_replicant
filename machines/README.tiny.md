# One- and two-core mesh simulations

`pod_X1Y1_mesh_X1Y1_hbm_one_pseudo_channel` and
`pod_X1Y1_mesh_X2Y1_hbm_one_pseudo_channel` instantiate one and two scalar cores,
respectively. They require the matching bsg_manycore singleton-dimension changes
(commit `660cd3be`, based on `d64efb63`); copying these machine files alone is insufficient.

Both profiles use blocking caches, 64 sets, eight ways and sixteen 32-bit words
per line (32 KiB/bank), one north and one south bank per column, one 1-GiB HBM
pseudochannel, 666-ps core cycles, iPoly off and software AMO tile-group barriers.
The network is a mesh with wormhole factor one. DMEM and instruction cache remain
4 KiB each. These profiles are for small-kernel work, not target-scale contention estimates.

Coordinate fields retain at least one local bit. Both compute origins are (2,2),
with caches at Y=1 and Y=4; the single compute row contains no core at Y=3.
The one-column model has caches at X=2 only and alternates DRAM lines north/south.
Its two cache-to-HBM links drain east. Core counts, coordinate span and cache-bank
selector bits are different quantities. Code that infers a physical origin from
`BSG_MACHINE_GLOBAL_X/Y` must instead use the configured origin. HammerBench
commit `c63185f2` avoids that inference with a single-pod barrier path, which has
passed the two-core suite cases below. Multiple pod columns with one tile column
per pod are unsupported: their I/O X coordinates require gaps that the current
testbench does not generate. Other multipod geometries were not tested here.

## Build and use

Select `BSG_PLATFORM=bigblade-verilator` and set `BSG_MACHINE_PATH` to the desired
directory here. Follow the normal neutral infrastructure build, with separate
Replicant/library/model destinations per concurrently writable build. On macOS,
use GNU Make, Clang and the repository Verilator. Disable the instruction logger
with `VDEFINES += VERILATOR_WORKAROUND_DISABLE_VCORE_TRACE` when counters suffice.

The pinned HammerBench includes explicit machine selection in vector_add and
the other tested templates (`e3019a8`, `c63185f2`). Generate
a case with matching `TILE_X=1 TILE_Y=1` (or `TILE_X=2 TILE_Y=1`); keep its element
count divisible by the tile count. Pass the machine path on build/run Make calls.
Inspect the selected template before reusing any other variant.

Execution and profiling passed the unchanged vector_add checker for 4096 elements
(cold) and 65536 elements (warm/cold) on both profiles. The warmup is a best-effort
cache preparation pass; the full three-array working set exceeds cache capacity.
A freshly built default 16x8 exec model also passed 4096 elements. This does not
establish support for arbitrary inputs, hardware barriers or multipod operation.
The subsequent 2x1 suite has 24 passing execution/profile runs and four retained
failures across fourteen small cases. AES, Black-Scholes, BFS push/pull, Jacobi,
PageRank, SGEMM, Smith-Waterman and memcpy pass their existing checks. FFT128m
passes after path repair; original FFT128 still requires 128 tiles. SpGEMM passes
unit weights; its weighted-input failure matches the kernel's arithmetic limitation.
Barnes-Hut passes with a separately corrected host reference and unchanged device
code/threshold; the original host's failure remains recorded. This pin does not
replace expert kernels or checkers with those diagnostic changes.
The existing eight-word-line 2x2 profile has a separate
zero-width DRAM counter elaboration failure; it was not changed here.

`tests/test_tiny_machine_coordinates.cpp` is a host-only check: compile it with
`-I libraries`, link the built `libbsg_manycore_runtime`, and pass the generated
`bsg_bladerunner_configuration.rom`. It checks core/cache enumeration, unique
addresses and (with iPoly off) EVA/NPA round trips over 1 MiB. Use assertions
(no `-DNDEBUG`). iPoly inverse mapping is outside this test's scope.
