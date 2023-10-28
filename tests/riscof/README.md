# Run the RISC-V architectural tests

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
$ make
$ [sudo] make install

## TODO

Run the architectural tests:

$ make

