# Simple 3-stage pipeline RISC-V processor

This is a simple RISC-V 3-stage pipeline processor.
I wrote this code to understand the RV32I instruction set, just for fun.
This is not a production-ready RISC-V core.

## simulation parameters

+meminit: memory intialize zero
+dumpvcd: dump VCD file
+gentrace: generate tracelog

# simulator

tools/gentrace:

# bechmark

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

# License

MIT license

