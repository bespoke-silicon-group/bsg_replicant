# Barnes-Hut N-body Simulation App

The Barnes-Hut algorithm is an algorithm for estimating the
interactions between N bodies in 3D space. You can read more about it
here: [Wikipedia](https://en.wikipedia.org/wiki/Barnes%E2%80%93Hut_simulation)

This workload is derived from the Lonestar Benchmark Suite.

# Summary:

There are four kernels in the application:

1. Build Tree (Varying parallelism, irregular memory accesses, requires allocation)
2. Summarize Centers of Mass (Recursive, varying parallelism, irregular memory accesses)
3. Compute Forces (Do-all parallelism, extremely irregular memory accesses, **main computational load**)
4. Update positions (Do-all paralllelism, regular memory accesses)

Sometimes, authors will write about 1-2 of the kernels (Compute Forces, Update Positions) and ignore the others.

The basic flow is to build an octree (1), summarize the center of mass
below each internal node (2), compute forces on each body using the
centers of masses as a distance-filtered approximation (3), and then
update the positions of each object using its computed force and mass(4).


These steps are encapsulated as kernels in the following files:
- `build.cpp`
- `summarize.cpp`
- `forces.cpp`
- `update.cpp`

The host file is called `Barneshut.cpp` and uses modified example code
from the Galois library. Once the application is built you can choose
the number of bodies and phase of the computation to run on
HammerBlade.

# Setup

You will need a relatively modern version of GCC that supports the C++14 standard, and Boost> 1.69

1. Run `make Galois`
2. Run `make setup`
3. Run `make exec.log`

This will run the simulation with the default parameters -- 8192
bodies for the Build Tree kernels (step 1, above). You can change
which step is the default in the Makefile.