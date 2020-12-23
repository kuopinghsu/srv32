# Compliance Test

<https://github.com/riscv/riscv-compliance>

    export ROOT_SRV32=<root path of simple RISCV>
    cp -r ${ROOT_SRV32}/tests/srv32 <path of riscv-compliance>/riscv-target/.

    cd ${ROOT_SRV32}
    make build

    export TARGET_SIM=${ROOT_SRV32}/sim/sim
    export RISCV_PREFIX=riscv64-unknown-elf-
    export RISCV_TARGET=srv32
    make

    export TARGET_SIM=${ROOT_SRV32}/tools/rvsim
    export RISCV_PREFIX=riscv64-unknown-elf-
    export RISCV_TARGET=srv32
    make

Requires 1716K of IRAM/DRAM to run compliance test v2, and 128K of IRAM/DRAM for
compliance test v1.
