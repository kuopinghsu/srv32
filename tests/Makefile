ROOT_SRV32 ?= $(shell cd .. && pwd)
memsize    ?= 256
test_v     ?= 2
rv32c      ?= 0
rv32e      ?= 0
rv32b      ?= 0

ifndef CROSS_COMPILE
export CROSS_COMPILE = riscv64-unknown-elf-
endif

ifneq (, $(findstring darwin, $(shell gcc -dumpmachine)))
POST_PATCH ?= cd riscv-arch-test.v2 && git checkout . && patch -p 1 < ../macos.patch && cd ..
else
POST_PATCH ?= echo
endif

.PHONY: riscv-arch-test

tests: riscv-arch-test
	export ROOT_SRV32=$(ROOT_SRV32); \
	export TARGET_SIM="$(ROOT_SRV32)/sim/sim +trace"; \
	export TARGET_SWSIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize)"; \
	export RISCV_PREFIX=$(CROSS_COMPILE); \
	export RISCV_TARGET=srv32; \
	$(MAKE) rv32c=$(rv32c) rv32e=$(rv32e) rv32b=$(rv32b) -C riscv-arch-test.v$(test_v);

tests-sw: riscv-arch-test
	export ROOT_SRV32=$(ROOT_SRV32); \
	export TARGET_SIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize) -l trace.log"; \
	export TARGET_SWSIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize)"; \
	export RISCV_PREFIX=$(CROSS_COMPILE); \
	export RISCV_TARGET=srv32; \
	$(MAKE) rv32c=$(rv32c) rv32e=$(rv32e) rv32b=$(rv32b) -C riscv-arch-test.v$(test_v)

riscv-arch-test:
	if [ "$(test_v)" = "1" ]; then \
		if [ ! -d riscv-arch-test.v1 ]; then \
			git clone -b 1.0 https://github.com/riscv/riscv-arch-test.git riscv-arch-test.v1; \
		fi; \
		rm -rf riscv-arch-test.v1/riscv-target/srv32; \
		if [ ! -d riscv-arch-test.v1/riscv-test-suite/rv32i_m/B ]; then \
			cp -r riscv-test-suite/rv32i_m/B riscv-arch-test.v1/riscv-test-suite/rv32i_m/. ; \
		fi; \
		cp -r srv32.v1 riscv-arch-test.v1/riscv-target/srv32; \
		if [ "$(rv32b)" = "0" ]; then \
			rm -rf riscv-arch-test.v1/riscv-target/srv32/device/rv32i_m/B; \
		fi; \
	else \
		if [ ! -d riscv-arch-test.v2 ]; then \
			git clone -b 2.7.4 https://github.com/riscv/riscv-arch-test.git riscv-arch-test.v2; \
		fi; \
		if [ ! -d riscv-arch-test.v2/riscv-test-suite/rv32i_m/B ]; then \
			cp -r riscv-test-suite/rv32i_m/B riscv-arch-test.v2/riscv-test-suite/rv32i_m/. ; \
		fi; \
		$(POST_PATCH); \
		rm -rf riscv-arch-test.v2/riscv-target/srv32; \
		cp -r srv32.v2 riscv-arch-test.v2/riscv-target/srv32; \
		if [ "$(rv32b)" = "0" ]; then \
			rm -rf riscv-arch-test.v2/riscv-target/srv32/device/rv32i_m/B; \
		fi; \
	fi

clean:
	rm -rf riscv-arch-test.v$(test_v)/work

distclean:
	rm -rf riscv-arch-test.v*

