SHELL := /bin/bash
CWD   := $(shell pwd)
PATH  := $(CWD)/../../tools:$(CWD)/../../sim:$(PATH)
DEBUG := #--verbose debug

all: tests-sw tests

tests-sw: pre-build
	make rv32b=1 memsize=1716 -C ../../tools
	riscof $(DEBUG) run --config=rvsim-config.ini --suite=riscv-arch-test/riscv-test-suite/ --env=riscv-arch-test/riscv-test-suite/env

tests: pre-build
	make memsize=1716 -C ../../sim
	riscof $(DEBUG) run --config=rv32rtl-config.ini --suite=riscv-arch-test/riscv-test-suite/ --env=riscv-arch-test/riscv-test-suite/env

validate: pre-build
	riscof validateyaml --config=rvsim-config.ini

ref: pre-build
	riscof --verbose info run --no-dut-run --config=rvsim-config.ini --suite=riscv-arch-test/riscv-test-suite/ --env=riscv-arch-test/riscv-test-suite/env

pre-build:
	@if [ ! -d riscv-arch-test ]; then \
        echo "clone riscv-arch-tests ..."; \
		riscof --verbose info arch-test --clone; \
	fi
	@if [ ! -f rvsim-config.ini ]; then \
		sed s^{PWD}^`pwd`^g rvsim-config.ini.in > rvsim-config.ini; \
	fi
	@if [ ! -f rv32rtl-config.ini ]; then \
		sed s^{PWD}^`pwd`^g rv32rtl-config.ini.in > rv32rtl-config.ini; \
	fi

clean:
	$(MAKE) -C ../../tools clean
	$(MAKE) -C ../../sim clean
	$(RM) -rf rvsim-config.ini rv32rtl-config.ini riscof_work rv32rtl/__pycache__ rvsim/__pycache__ spike/__pycache__

distclean: clean
	$(RM) -rf riscv-arch-test

