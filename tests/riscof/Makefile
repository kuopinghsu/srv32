SHELL := /bin/bash
CWD   := $(shell pwd)
PATH  := $(CWD)/../../tools:$(PATH)
DEBUG := --verbose debug

all: pre-build
	make rv32b=1 memsize=1716 -C ../../tools
	if [ ! -d riscv-arch-test ]; then \
		riscof --verbose info arch-test --clone; \
	fi
	riscof $(DEBUG) run --config=rvsim-config.ini --suite=riscv-arch-test/riscv-test-suite/ --env=riscv-arch-test/riscv-test-suite/env

validate: pre-build
	riscof validateyaml --config=rvsim-config.ini

ref: pre-build
	riscof --verbose info run --no-dut-run --config=rvsim-config.ini --suite=riscv-arch-test/riscv-test-suite/ --env=riscv-arch-test/riscv-test-suite/env

pre-build:
	@if [ ! -f rvsim-config.ini ]; then \
		sed s^{PWD}^`pwd`^g rvsim-config.ini.in > rvsim-config.ini; \
	fi

clean:
	$(MAKE) -C ../../tools clean
	$(RM) -rf rvsim-config.ini riscof_work rvsim/__pycache__ spike/__pycache__

distclean: clean
	$(RM) -rf riscv-arch-test

