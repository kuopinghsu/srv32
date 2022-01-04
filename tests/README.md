# Compliance Test

<https://github.com/riscv-non-isa/riscv-arch-test>

    export ROOT_SRV32=<root path of simple RISCV>
    cp -r ${ROOT_SRV32}/tests/srv32 <path of riscv-arch-test>/riscv-target/.

    cd ${ROOT_SRV32}
    make build

    export CROSS_COMPILER=riscv64-unknown-elf-
    export TARGET_SIM=${ROOT_SRV32}/sim/sim
    export RISCV_PREFIX=${CROSS_COMPILER}
    export RISCV_TARGET=srv32
    make

    export TARGET_SIM=${ROOT_SRV32}/tools/rvsim
    export RISCV_PREFIX=${CROSS_COMPILER}
    export RISCV_TARGET=srv32
    make

Requires 1716K of IRAM/DRAM to run architecture test v2, and 128K of IRAM/DRAM for
compliance test v1.

## TODO

In architecture test V2, the following misaligned instruction tests will match targets
which have compressed extension support enabled by default. Targets without the
compressed extension support will fail the following tests:

*   rv32i_m/privilege/src/misalign-b\[ge\[u\],lt\[u\],eq,ne\]-01.S
*   rv32i_m/privilege/src/misalign\[1,2\]-jalr-01.S
