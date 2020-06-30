ROOT_SIMPLE_RISCV ?= $(shell pwd)/../

tests: riscv-compliance
	export ROOT_SIMPLE_RISCV=$(ROOT_SIMPLE_RISCV); \
	export TARGET_SIM=$(ROOT_SIMPLE_RISCV)/sim/riscsim; \
	export RISCV_TARGET=simple-riscv; \
	make -C riscv-compliance

tests-sw: riscv-compliance
	export ROOT_SIMPLE_RISCV=$(ROOT_SIMPLE_RISCV); \
	export TARGET_SIM=$(ROOT_SIMPLE_RISCV)/tools/gentrace; \
	export RISCV_TARGET=simple-riscv; \
	make -C riscv-compliance

riscv-compliance:
	git clone https://github.com/riscv/riscv-compliance.git
	cp -r simple-riscv riscv-compliance/riscv-target/.

clean:
	rm -rf riscv-compliance/work

distclean:
	rm -rf riscv-compliance .gitget

