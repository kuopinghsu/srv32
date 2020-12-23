ROOT_SRV32 ?= $(shell cd .. && pwd)
memsize    ?= 128
test_v     ?= 1

.PHONY: riscv-compliance

tests: riscv-compliance
	export ROOT_SRV32=$(ROOT_SRV32); \
	export TARGET_SIM="$(ROOT_SRV32)/sim/sim +trace"; \
	export TARGET_SWSIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize)"; \
	export RISCV_PREFIX=riscv64-unknown-elf-; \
	export RISCV_TARGET=srv32; \
	make -C riscv-compliance

tests-sw: riscv-compliance
	export ROOT_SRV32=$(ROOT_SRV32); \
	export TARGET_SIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize) -l trace.log"; \
	export TARGET_SWSIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize)"; \
	export RISCV_PREFIX=riscv64-unknown-elf-; \
	export RISCV_TARGET=srv32; \
	make -C riscv-compliance

riscv-compliance:
	if [ "$(test_v)" == "1" ]; then \
		if [ ! -d riscv-compliance ]; then \
			git clone -b v1.0 git://github.com/riscv/riscv-compliance.git; \
		fi; \
		rm -rf riscv-compliance/riscv-target/srv32; \
		cp -r srv32.v1 riscv-compliance/riscv-target/srv32; \
	else \
		if [ ! -d riscv-compliance ]; then \
			git clone git://github.com/riscv/riscv-compliance.git; \
		fi; \
		rm -rf riscv-compliance/riscv-target/srv32; \
		cp -r srv32.v2 riscv-compliance/riscv-target/srv32; \
	fi

clean:
	rm -rf riscv-compliance/work

distclean:
	rm -rf riscv-compliance

