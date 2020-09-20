
# Compliance Test

    <https://github.com/riscv/riscv-compliance>

~~~
    export ROOT_SIMPLE_RISCV=<root path of simple RISCV>
    cp -r ${ROOT_SIMPLE_RISCV}/tests/simple-riscv <path of riscv-compliance>/riscv-target/.

    cd ${ROOT_SIMPLE_RISCV}
    make build

    export TARGET_SIM=${ROOT_SIMPLE_RISCV}/sim/sim
    export RISCV_PREFIX=riscv32-unknown-elf-
    export RISCV_TARGET=simple-riscv
    make

    export TARGET_SIM=${ROOT_SIMPLE_RISCV}/tools/rvsim
    export RISCV_PREFIX=riscv32-unknown-elf-
    export RISCV_TARGET=simple-riscv
    make
~~~
