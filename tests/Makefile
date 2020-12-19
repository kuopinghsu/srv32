ROOT_SIMPLE_RISCV ?= $(shell cd .. && pwd)

tests: riscv-compliance
	rm -rf riscv-compliance/riscv-target/srv32
	cp -r srv32 riscv-compliance/riscv-target/.
	export ROOT_SIMPLE_RISCV=$(ROOT_SIMPLE_RISCV); \
	export TARGET_SIM="$(ROOT_SIMPLE_RISCV)/sim/sim +trace"; \
	export RISCV_PREFIX=riscv64-unknown-elf-; \
	export RISCV_TARGET=srv32; \
	make -C riscv-compliance

tests-sw: riscv-compliance
	rm -rf riscv-compliance/riscv-target/srv32
	cp -r srv32 riscv-compliance/riscv-target/.
	export ROOT_SIMPLE_RISCV=$(ROOT_SIMPLE_RISCV); \
	export TARGET_SIM="$(ROOT_SIMPLE_RISCV)/tools/rvsim -l trace.log"; \
	export RISCV_PREFIX=riscv64-unknown-elf-; \
	export RISCV_TARGET=srv32; \
	make -C riscv-compliance

riscv-compliance:
	git clone -b v1.0 git://github.com/riscv/riscv-compliance.git
	rm -rf riscv-compliance/riscv-target/srv32
	cp -r srv32 riscv-compliance/riscv-target/.

clean:
	rm -rf riscv-compliance/work

distclean:
	rm -rf riscv-compliance

