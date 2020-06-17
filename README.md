Simple 3-stage pipeline RISC-V processor
========================================

This is a simple RISC-V 3-stage pipeline processor.
I wrote this code to understand the RV32I instruction set, just for fun.
This is not a production-ready RISC-V core.

## Building toolchains

```text
git clone https://github.com/riscv/riscv-gnu-toolchain riscv-gnu-toolchain-rv32i
cd riscv-gnu-toolchain-rv32i
git submodule update --init --recursive

mkdir build; cd build
../configure --with-arch=rv32i --prefix=/opt/riscv32i
make -j$(nproc)
```
## Files list

| Folder    | Description                    |
| --------- | ------------------------------ |
| doc       | Instruction sets document      |
| rtl       | RTL files                      |
| sim       | Icarus verilog simulation env  |
| testbench | testbench, memory model        |
| tool      | software simulator             |

## Simulation parameters

```text
+meminit    memory intialize zero
+dumpvcd    dump VCD file
+trace      generate tracelog
```

## Software simulator

```text
Usage: tracegen [-h] [-b n] [-l logfile]

       --help, -n              help
       --branch n, -b n        branch penalty (default 2)
       --log file, -l file     generate log file
```

## bechmark

* Dhrystone

```text
Number_Of_Runs: 100
User_Time: 29356 cycles, 24343 insn
Cycles_Per_Instruction: 1.205
Dhrystones_Per_Second_Per_MHz: 3406
DMIPS_Per_MHz: 1.938
```

* Coremark

```text
CoreMark 1.0 : 91.194239 / GCC8.2.0 -O3 -march=rv32i -nostartfiles -nostdlib -flto -L../common -DPERFORMANCE_RUN=1   -lc -lm -lgcc -lsys -T ../common/default.ld / STACK
```

## Cycles per Instruction Performance

Two instructions brach panelty if branch taken, the CPI is 1 for other instructions.

## TODO

* add ECALL and EBREAK instructions
* add interrupt
* support FreeRTOS

# License

MIT license

