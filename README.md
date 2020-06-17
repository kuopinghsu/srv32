Simple 3-stage pipeline RISC-V processor
========================================

This is a simple RISC-V 3-stage pipeline processor.
I wrote this code to understand the RV32I instruction set, just for fun.
This is not a RISC-V core available for production

## Building toolchains

    # Ubuntu packages needed:
    sudo apt-get install autoconf automake autotools-dev curl libmpc-dev \
        libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo \
        gperf libtool patchutils bc zlib1g-dev git libexpat1-dev

    git clone https://github.com/riscv/riscv-gnu-toolchain riscv-gnu-toolchain-rv32i
    cd riscv-gnu-toolchain-rv32i
    git submodule update --init --recursive

    mkdir build; cd build
    ../configure --with-arch=rv32i --prefix=/opt/riscv32i
    make -j$(nproc)

## Files list

| Folder    | Description                    |
| --------- | ------------------------------ |
| doc       | Instruction sets document      |
| rtl       | RTL files                      |
| sim       | Icarus verilog simulation env  |
| testbench | testbench, memory model        |
| tool      | software simulator             |

## Simulation parameters

    # Ubuntu package needed to run the RTL simulation
    sudo apt install iverilog

Only running make without paramets will get help.

    $ make
    make all         build all diags and run the RTL sim
    make hello       build hello diag and run the RTL sim
    make dhrystone   build Dhrystone diag and run the RTL sim
    make coremark    build Coremark diag and run the RTL sim
    make clean       clean

Supports following parameter when running the simulation.

    +meminit    memory intialize zero
    +dumpvcd    dump VCD file
    +trace      generate tracelog

For example, following command will generate the VCD dump.

    cd sim && ./riscrun +dumpvcd

The RTL testbench read the memory initialize file from sw/imem.hex and sw/dmem.hex.

## Software simulator

    Usage: tracegen [-h] [-b n] [-l logfile]

           --help, -n              help
           --branch n, -b n        branch penalty (default 2)
           --log file, -l file     generate log file

The simulator read the memory initialize file from sw/imem.bin and sw/dmem.bin.

## Bechmark

* Dhrystone

<img src="https://github.com/kuopinghsu/simple-riscv/blob/master/images/dhrystone.png" alt="Dhrystone" width=640>

* Coremark

<img src="https://github.com/kuopinghsu/simple-riscv/blob/master/images/coremark.png" alt="Coremark" width=640>

## Cycles per Instruction Performance

Two instructions brach panelty if branch taken, CPI is 1 for other instructions. The average Cycles per Instruction (CPI) is approximately 1.2 on Dhrystone diag.

* Register Forwarding

<img src="https://github.com/kuopinghsu/simple-riscv/blob/master/images/forwarding.svg" alt="Register Forwarding" width=640>

* Branch Penalty

<img src="https://github.com/kuopinghsu/simple-riscv/blob/master/images/branch.svg" alt="Branch Penalty" width=640>

## TODO

* pass formal verification
* support ECALL and EBREAK instructions
* interrupt handling
* support FreeRTOS

# License

MIT license

