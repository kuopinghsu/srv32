The default build in this directory produces `sim_axi`, which instantiates
the AXI4-Lite SoC through the common `testbench/testbench.v`. The repository-level `make all` runs its hardware and
firmware regressions.

Use `make -C sim top=0` from the repository root to build the split-RAM
simulator (`sim`), or `make -C sim top=1` to build the single-RAM simulator
(`sim_single`). The repository-level `make hello` runs the split-RAM
diagnostic with RTL/ISS trace comparison.

Buffered instruction fetch (`MEM_WAIT_STATES=1`, including AXI-Lite and
both direct-RAM modes) enables static backward-taken/forward-not-taken prediction by
default. Aligned direct JAL targets are also predicted; JALR resolves in EX.
EX redirects only on a wrong prediction or a trap. Predictions respect pipeline
stalls and use the existing redirect handshake to discard stale fetches.
Set `STATIC_BRANCH_PREDICTOR=0` on the core or top-level wrapper to disable it.
The single `testbench/testbench.v` selects its system through elaboration
parameters:

- `MEMORY_MODE=0`: split RAM (`top=0`).
- `MEMORY_MODE=1`: shared RAM (`top=1`).
- `MEMORY_MODE=2`: AXI-Lite SoC (`top=axi`).
- `BRANCH_PREDICTOR=0/1`: disable/enable the RTL predictor.
- `UART_CLOCKS_PER_BIT`: AXI-Lite UART test timing (default 16).

The Makefile passes these parameters for each build. Both direct-RAM modes
use the same `top_ram` adapter and core hierarchy. Only the physical memory
models differ:
`top=0` has independent instruction/data ports, while `top=1` arbitrates a
single shared port. The split data RAM accepts reads at the EX/WB boundary
and supplies WB directly, preserving the original split-RAM load throughput.
MMIO, syscalls, retirement qualification, and trace output
are shared. All RTL prediction decisions and recovery logic reside in
`rtl/riscv.v`; the adapter only buffers responses and handles core redirects.

Run `make -C sim check-branch` to run CoreMark, IRQ, exception, and CSR
diagnostics through the merged `testbench/testbench.v` in all four combinations
of RAM layout and prediction enabled/disabled. Each run compares RTL and ISS
traces. `make -C sim check` includes these tests and
the AXI-Lite RAM, UART, interrupt, retirement, and firmware regressions, all
from `testbench/testbench.v`. The AXI scenarios remain selectable with
`./sim_axi`, `+redirect`, `+store_irq`, and `+image=path/to/firmware.hex`.

Verilator-built executables remain in their build directories and are copied
to the simulator entry points. This allows cached builds to be reused when
switching back to a previous configuration.

For a CoreMark comparison, select either RAM configuration:

```sh
make top=1 branch_predictor=1 coremark
make top=1 branch_predictor=0 coremark
make top=0 branch_predictor=1 coremark
make top=0 branch_predictor=0 coremark
```

The build automatically rebuilds the RTL simulator when its configuration
changes and passes the same prediction setting to the ISS. Both runs compare
RTL and ISS traces, including cycle-counter values read by the benchmark.
Compare `Total ticks`, `CoreMark/MHz`, and the final cycle/CPI statistics across
runs. Both should report correct operation, matching CRCs, and simulation passed.
Both `top=0` and `top=1` support the same prediction policy. Split RAM allows
instruction fetches to overlap data accesses, so their cycle counts differ.

To run just the RTL after building the firmware, use
`make -C sim top=1 branch_predictor=1 coremark.elf` (or `branch_predictor=0`).

The split-RAM and shared-RAM simulators build and run without the AXI
stimulus file. The `sim_axi` build defines `AXI_LITE_REGRESSION` to include
the AXI stimulus and RAM response model.

For AXI-Lite, `memsize` sets total external RAM in KiB. The build passes
`rv32m`, `rv32e`, `rv32b`, and `rv32c` through to the SoC. The RTL does
not implement the B or C instruction extensions. UART timing can be set with
`uart_clocks_per_bit` (default 16).

```sh
make -C sim top=axi memsize=64 rv32e=1 branch_predictor=0
```

For a direct Verilog build, override the shared `RAM_BYTES`, `RV32M`,
`RV32E`, `RV32B`, `RV32C`, `BRANCH_PREDICTOR`, and
`UART_CLOCKS_PER_BIT` on `testbench`.
