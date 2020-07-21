ROOT_SIMPLE_RISCV ?= $(shell cd .. && pwd)

tests: riscv-compliance
	export ROOT_SIMPLE_RISCV=$(ROOT_SIMPLE_RISCV); \
	export TARGET_SIM="$(ROOT_SIMPLE_RISCV)/sim/sim +trace"; \
	export RISCV_PREFIX=riscv32-unknown-elf-; \
	export RISCV_TARGET=simple-riscv; \
	make -C riscv-compliance

tests-sw: riscv-compliance
	export ROOT_SIMPLE_RISCV=$(ROOT_SIMPLE_RISCV); \
	export TARGET_SIM="$(ROOT_SIMPLE_RISCV)/tools/gentrace -l trace.log"; \
	export RISCV_PREFIX=riscv32-unknown-elf-; \
	export RISCV_TARGET=simple-riscv; \
	make -C riscv-compliance

riscv-compliance:
	git clone git://github.com/riscv/riscv-compliance.git
	cp -r simple-riscv riscv-compliance/riscv-target/.

clean:
	rm -rf riscv-compliance/work

distclean:
	rm -rf riscv-compliance

