ROOT_SIMPLE_RISCV ?= $(shell cd .. && pwd)

.PHONY: riscv-compliance

tests: riscv-compliance_v1
	export ROOT_SIMPLE_RISCV=$(ROOT_SIMPLE_RISCV); \
	export TARGET_SIM="$(ROOT_SIMPLE_RISCV)/sim/sim +trace"; \
	export RISCV_PREFIX=riscv64-unknown-elf-; \
	export RISCV_TARGET=srv32; \
	make -C riscv-compliance

tests-sw: riscv-compliance_v1
	export ROOT_SIMPLE_RISCV=$(ROOT_SIMPLE_RISCV); \
	export TARGET_SIM="$(ROOT_SIMPLE_RISCV)/tools/rvsim -l trace.log"; \
	export RISCV_PREFIX=riscv64-unknown-elf-; \
	export RISCV_TARGET=srv32; \
	make -C riscv-compliance

riscv-compliance_v1:
	if [ ! -d riscv-compliance ]; then git clone -b v1.0 git://github.com/riscv/riscv-compliance.git; fi
	rm -rf riscv-compliance/riscv-target/srv32
	cp -r srv32.v1 riscv-compliance/riscv-target/srv32

riscv-compliance_v2:
	if [ ! -d riscv-compliance ]; then git clone git://github.com/riscv/riscv-compliance.git; fi
	rm -rf riscv-compliance/riscv-target/srv32
	cp -r srv32.v2 riscv-compliance/riscv-target/srv32

clean:
	rm -rf riscv-compliance/work

distclean:
	rm -rf riscv-compliance

