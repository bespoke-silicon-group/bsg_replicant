# Black-Scholes

This is app is the well-known Black-Scholes benchmark. It is an option
pricing app using a Partial Differential Equation (PDE).

The code is derived from the CPU PARSEC benchmark suite.

There are four versions:

- `unopt-single` and `unopt-pod`: More or less unmodified PARSEC code, using Single-precision computation.
- `opt-single` and `opt-pod`: Optimized versions of the PARSEC code.

There are two main optimizations:

1. Reading multiple options per tile (to improve cache line locality)
2. Using a custom link script to put the logf/expf look-up-tables in Scratchpad (see bs_link.ld).

% Quick-Start:

To get the data:
```
wget http://parsec.cs.princeton.edu/download/3.0/parsec-3.0-input-native.tar.gz
wget http://parsec.cs.princeton.edu/download/3.0/parsec-3.0-input-sim.tar.gz
tar -xf parsec-3.0-input-sim.tar.gz
tar -xf parsec-3.0-input-native.tar.gz
cp parsec-3.0/pkgs/apps/blackscholes/inputs/* data
```

Unzip the .tar file of interest, and then run the apps using the normal cuda flow.
