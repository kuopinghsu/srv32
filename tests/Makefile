ROOT_SRV32 ?= $(shell cd .. && pwd)
memsize    ?= 128
test_v     ?= 1
rv32c      ?= 0

.PHONY: riscv-compliance

tests: riscv-compliance
	export ROOT_SRV32=$(ROOT_SRV32); \
	export TARGET_SIM="$(ROOT_SRV32)/sim/sim +trace"; \
	export TARGET_SWSIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize)"; \
	export RISCV_PREFIX=riscv64-unknown-elf-; \
	export RISCV_TARGET=srv32; \
	$(MAKE) rv32c=$(rv32c) -C riscv-compliance.v$(test_v)

tests-sw: riscv-compliance
	export ROOT_SRV32=$(ROOT_SRV32); \
	export TARGET_SIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize) -l trace.log"; \
	export TARGET_SWSIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize)"; \
	export RISCV_PREFIX=riscv64-unknown-elf-; \
	export RISCV_TARGET=srv32; \
	$(MAKE) rv32c=$(rv32c) -C riscv-compliance.v$(test_v)

riscv-compliance:
	if [ "$(test_v)" = "1" ]; then \
		if [ ! -d riscv-compliance.v1 ]; then \
			git clone -b 1.0 git://github.com/riscv/riscv-arch-test.git riscv-compliance.v1; \
		fi; \
		rm -rf riscv-compliance.v1/riscv-target/srv32; \
		cp -r srv32.v1 riscv-compliance.v1/riscv-target/srv32; \
	else \
		if [ ! -d riscv-compliance.v2 ]; then \
			git clone git://github.com/riscv/riscv-arch-test.git riscv-compliance.v2; \
		fi; \
		rm -rf riscv-compliance.v2/riscv-target/srv32; \
		cp -r srv32.v2 riscv-compliance.v2/riscv-target/srv32; \
	fi

clean:
	rm -rf riscv-compliance.v$(test_v)/work

distclean:
	rm -rf riscv-compliance.v*

