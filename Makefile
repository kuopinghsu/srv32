
# Toolchain defaults; command-line assignments override Makefile.env.
include Makefile.env

ifndef CROSS_COMPILE
export CROSS_COMPILE = riscv64-unknown-elf-
endif

dirs        = $(dir $(wildcard sw/[^_]*/))
SUBDIRS     = $(filter-out common axi_lite,$(subst /,,$(subst sw/,,$(dirs))))

verilator ?= 1
top       ?= $(if $(filter 1,$(verilator)),axi,0)
coverage  ?= 0
debug     ?= 0
memsize   ?= 256
branch_predictor ?= 1

# Architectural test framework: 2 for Make-based tests, 3 for RISCOF.
test_v    ?= 3

# set 1 to enable rv32c
rv32c     ?= 0

# set 1 to enable rv32e
rv32e     ?= 0

# set 1 to enable rv32b
rv32b     ?= 0

ifeq ($(verilator), 1)
    _verilator := 1
endif

ifeq ($(top), 1)
    _top := 1
endif

ifeq ($(coverage), 1)
    _coverage := 1
endif

MAKE_FLAGS = rv32c=$(rv32c) rv32e=$(rv32e) rv32b=$(rv32b) branch_predictor=$(branch_predictor)

.PHONY: help all all-sw tests-all tests tests-sw build coverage clean distclean $(SUBDIRS) tools

help:
	@echo "Usage: make [target] [option=value ...]"
	@echo ""
	@echo "Targets:"
	@echo "  help               show this help (default target)"
	@echo "  hello / <diag>     build a diagnostic, run RTL and ISS, compare traces"
	@echo "  coremark           run CoreMark with RTL/ISS trace comparison"
	@echo "  dhrystone          run Dhrystone with RTL/ISS trace comparison"
	@echo "  build              build software, the selected RTL simulator, and ISS"
	@echo "  all                clean, run simulation checks, then all RTL/ISS diagnostics"
	@echo "  all-sw             run all software diagnostics on the ISS"
	@echo "  tests              run architectural tests on RTL"
	@echo "  tests-sw           run architectural tests on the ISS"
	@echo "  tests-all          run architectural tests and all software diagnostics"
	@echo "  coverage           generate coverage reports (v3 RTL collection is TODO)"
	@echo "  clean / distclean  remove build output / also remove downloaded arch-tests"
	@echo ""
	@echo "Simulation checks:"
	@echo "  make -C sim check         RAM/predictor matrix plus AXI-Lite regressions"
	@echo "  make -C sim check-branch  CoreMark, IRQ, exception, CSR in all four RAM/predictor modes"
	@echo ""
	@echo "Memory selection:"
	@echo "  top=0              non-shared instruction/data RAM (default for named diagnostics)"
	@echo "  top=1              shared single-port instruction/data RAM"
	@echo "  top=axi            AXI-Lite SoC (default for build and make -C sim)"
	@echo ""
	@echo "Options:"
	@echo "  branch_predictor=0|1  static prediction in all buffered modes (default 1)"
	@echo "  memsize=256          KiB each for instruction and data memory"
	@echo "  debug=1              write sim/wave.fst with Verilator (default 0)"
	@echo "  coverage=1           enable coverage instrumentation (default 0)"
	@echo "  verilator=0          select the legacy Icarus build (default 1)"
	@echo "  rv32e=1              RV32E register file/toolchain (matching multilib required)"
	@echo "  rv32b=1 / rv32c=1    ISS/toolchain switches; RTL B/C execution is not implemented"
	@echo "  test_v=2|3           architectural framework (default 3: RISCOF)"
	@echo "  CROSS_COMPILE=...    RISC-V tool prefix (see Makefile.env)"
	@echo "  EXTRA_CFLAGS=...     extra firmware compiler flags (see Makefile.env)"
	@echo ""
	@echo "Examples:"
	@echo "  make top=0 branch_predictor=1 coremark dhrystone"
	@echo "  make top=1 branch_predictor=0 coremark"
	@echo "  make debug=1 hello"
	@echo "  make test_v=2 tests"

tests-all: tests-sw tests all all-sw

all:
	$(MAKE) clean
ifeq ($(verilator),1)
	$(MAKE) -C sim check
endif
	for i in $(SUBDIRS); do \
		$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) $$i || exit 1; \
	done

all-sw:
	for i in $(SUBDIRS); do \
		$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) -C tools $$i.elf; \
	done

# Architectural tests use 1716 KiB each for instruction and data memory.
tests:
	$(MAKE) coverage=$(coverage) clean
	$(MAKE) $(MAKE_FLAGS) memsize=1716 test_v=$(test_v) -C tests tests

tests-sw:
	$(MAKE) coverage=$(coverage) clean
	$(MAKE) $(MAKE_FLAGS) memsize=1716 test_v=$(test_v) -C tests tests-sw

build:
	for i in sw sim tools; do \
		$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) top=$(top) verilator=$(verilator) -C $$i; \
	done

$(SUBDIRS):
	@$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) -C sw $@
	@$(MAKE) $(if $(_verilator), verilator=1) \
			 $(if $(_coverage), coverage=1) \
			 $(if $(_top), top=1) $(MAKE_FLAGS) memsize=$(memsize) debug=$(debug) -C sim $@.elf
	@$(MAKE) $(if $(_top), top=1) $(MAKE_FLAGS) memsize=$(memsize) tracelog=1 -C tools $@.elf
	@echo "Compare the trace between RTL and ISS simulator"
	@set -e; trace_dir=$$(mktemp -d); trap 'rm -rf "$$trace_dir"' EXIT; \
	    sed -E 's/^ *[0-9]+ //' sim/trace.log > "$$trace_dir/rtl"; \
	    sed -E 's/^ *[0-9]+ //' tools/trace.log > "$$trace_dir/iss"; \
	    diff --brief "$$trace_dir/rtl" "$$trace_dir/iss"
	@echo === Simulation passed ===

coverage: clean
	@$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) coverage=1 all
	@mv sim/*_cov.dat coverage/.
	@$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) coverage=1 tests
	@if [ "$(test_v)" = "2" ]; then \
	    mv tests/riscv-arch-test.v2/work/rv32i_m/I/*_cov.dat coverage/.; \
	    mv tests/riscv-arch-test.v2/work/rv32i_m/M/*_cov.dat coverage/.; \
	    if [ "$(rv32b)" = "1" ]; then \
	        mv tests/riscv-arch-test.v2/work/rv32i_m/B/*_cov.dat coverage/.; \
	    fi; \
	else \
		echo "TODO: code coverage for riscv-arch-test V3"; \
	fi
	@$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) -C coverage
	@$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) -C tools coverage

clean:
	@for i in sw sim tools tests coverage; do \
		$(MAKE) test_v=$(test_v) -C $$i clean; \
	done

distclean:
	@for i in sw sim tools tests coverage; do \
		$(MAKE) test_v=$(test_v) -C $$i distclean; \
	done

