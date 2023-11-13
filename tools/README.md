
# ISS (Instruction Set Simulator)

## Usage

The rvsim is an instruction set simulator (ISS) that can generate trace logs for comparison with RTL simulation results.

    Instruction Set Simulator for RV32IM, (c) 2020 Kuoping Hsu
    Usage: rvsim [-h] [-b n] [-m n] [-n n] [-p] [-l logfile] file

           --help, -h              help
           --debug, -d             interactive debug mode
           --quiet, -q             quite
           --membase n, -m n       memory base
           --memsize n, -n n       memory size (in Kb)
           --branch n, -b n        branch penalty (default 2)
           --single, -s            single RAM
           --predict, -p           static branch prediction
           --log file, -l file     generate log file

           file                    the elf executable file

## RISC-V disassembler

The disassembler in the interactive debug mode is from [here](https://github.com/michaeljclark/riscv-disassembler/).

Notes: it does not support B extensions yet.

