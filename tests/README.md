# Run the RISC-V architectural tests

## Architectural tests V3

Refer to [RISCOF](https://riscof.readthedocs.io/en/stable/installation.html) to setup.

Install RISCOF:

    $ pip3 install git+https://github.com/riscv/riscof.git

Install spike:

    $ sudo apt-get install device-tree-compiler
    $ git clone https://github.com/riscv-software-src/riscv-isa-sim.git
    $ cd riscv-isa-sim
    $ mkdir build
    $ cd build
    $ ../configure --prefix=/path/to/install
    $ make install

## Architectural tests V2

The older 2.x version of the framework was based on Makefile. This is no longer officially supported, but the environment is simpler (no need to install RISCOF) and runs faster.

## Memory requirement

Requires 1716KB of IRAM/DRAM to run architectural tests

## TODO

Passes the rv32i_m/privilege tests for RTL simulation.

