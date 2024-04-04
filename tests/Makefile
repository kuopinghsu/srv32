ROOT_SRV32  ?= $(shell cd .. && pwd)
memsize     ?= 1716
coverage    ?= 0
test_v      ?= 3
rv32m       ?= 1
rv32c       ?= 0
rv32e       ?= 0
rv32b       ?= 0

V2_TAG      := 2.7.4
V3_TAG      := 3.8.10

PATH        := $(ROOT_SRV32)//tools:$(ROOT_SRV32)/sim:$(PATH)
DEBUG       := #--verbose debug

ifndef CROSS_COMPILE
export CROSS_COMPILE = riscv64-unknown-elf-
endif

ifneq (, $(findstring darwin, $(shell gcc -dumpmachine)))
POST_PATCH ?= cd riscv-arch-test.v2 && git checkout . && patch -p 1 < ../macos.patch && cd ..
else
POST_PATCH ?= echo
endif

.PHONY: pre-build

tests: pre-build
	if [ "$(test_v)" = "3" ]; then \
		riscof $(DEBUG) run --config=rv32rtl-config.ini --suite=riscv-arch-test.v3/riscv-test-suite/ --env=riscv-arch-test.v3/riscv-test-suite/env; \
	else \
		$(MAKE) rv32m=$(rv32m) rv32c=$(rv32c) rv32b=$(rv32b) memsize=$(memsize) -C $(ROOT_SRV32)/tools; \
		export ROOT_SRV32=$(ROOT_SRV32); \
		export TARGET_SIM="$(ROOT_SRV32)/sim/sim +trace"; \
		export TARGET_SWSIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize)"; \
		export RISCV_PREFIX=$(CROSS_COMPILE); \
		export RISCV_TARGET=srv32; \
		$(MAKE) rv32c=$(rv32c) rv32e=$(rv32e) rv32b=$(rv32b) -C riscv-arch-test.v$(test_v); \
	fi

tests-sw: pre-build
	if [ "$(test_v)" = "3" ]; then \
		riscof $(DEBUG) run --config=rvsim-config.ini --suite=riscv-arch-test.v3/riscv-test-suite/ --env=riscv-arch-test.v3/riscv-test-suite/env; \
	else \
		export ROOT_SRV32=$(ROOT_SRV32); \
		export TARGET_SIM="$(ROOT_SRV32)/tools/rvsim --memsize $(memsize) -l trace.log"; \
		export TARGET_SWSIM=""; \
		export RISCV_PREFIX=$(CROSS_COMPILE); \
		export RISCV_TARGET=srv32; \
		$(MAKE) rv32c=$(rv32c) rv32e=$(rv32e) rv32b=$(rv32b) -C riscv-arch-test.v$(test_v); \
	fi

pre-build:
	$(MAKE) -C $(ROOT_SRV32)/tools clean
	$(MAKE) -C $(ROOT_SRV32)/sim clean
	$(MAKE) rv32m=$(rv32m) rv32c=$(rv32c) rv32b=$(rv32b) rv32e=$(rv32e) memsize=$(memsize) -C $(ROOT_SRV32)/sim
	@if [ "$(test_v)" = "2" ]; then \
		$(MAKE) rv32m=$(rv32m) rv32c=$(rv32c) rv32b=$(rv32b) memsize=$(memsize) -C $(ROOT_SRV32)/tools; \
		if [ ! -d riscv-arch-test.v2 ]; then \
            echo "clone riscv-arch-test v2"; \
			git clone -b $(V2_TAG) https://github.com/riscv/riscv-arch-test.git riscv-arch-test.v2; \
		fi; \
		$(POST_PATCH); \
		$(RM) -rf riscv-arch-test.v2/riscv-target/srv32; \
		cp -r srv32.v2 riscv-arch-test.v2/riscv-target/srv32; \
	elif [ "$(test_v)" = "3" ]; then \
		$(MAKE) rv32m=1 rv32c=1 rv32b=1 memsize=$(memsize) -C $(ROOT_SRV32)/tools; \
		if [ ! -d riscv-arch-test.v3 ]; then \
            echo "clone riscv-arch-test v3"; \
			git clone -b $(V3_TAG) https://github.com/riscv/riscv-arch-test.git riscv-arch-test.v3; \
		fi; \
		if [ ! -f rvsim-config.ini ]; then \
			sed s^{PWD}^`pwd`^g rvsim/rvsim-config.ini.in > rvsim-config.ini; \
    	fi; \
		if [ ! -f rv32rtl-config.ini ]; then \
			sed s^{PWD}^`pwd`^g rv32rtl/rv32rtl-config.ini.in > rv32rtl-config.ini; \
		fi; \
	else \
		echo "No support test_v"; \
		exit 1; \
	fi

validate: pre-build
	riscof validateyaml --config=rvsim-config.ini

ref: pre-build
	riscof --verbose info run --no-dut-run --config=rvsim-config.ini --suite=riscv-arch-test.v3/riscv-test-suite/ --env=riscv-arch-test.v3/riscv-test-suite/env

clean:
	$(MAKE) -C $(ROOT_SRV32)/tools clean
	$(MAKE) -C $(ROOT_SRV32)/sim clean
	$(RM) -rf riscv-arch-test.v2/work
	$(RM) -rf rvsim-config.ini rv32rtl-config.ini riscof_work
	$(RM) -rf rv32rtl/__pycache__ rvsim/__pycache__ spike/__pycache__

distclean:
	$(RM) -rf riscv-arch-test.v*

