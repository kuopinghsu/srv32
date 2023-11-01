
ifndef CROSS_COMPILE
export CROSS_COMPILE = riscv64-unknown-elf-
endif

dirs        = $(dir $(wildcard sw/[^_]*/))
SUBDIRS     = $(subst /,,$(subst sw/,,$(subst common,,$(dirs))))

verilator ?= 1
top       ?= 0
coverage  ?= 0
debug     ?= 0
memsize   ?= 256

# set 1 for compliance test v1, 2 for v2
test_v    ?= 2

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

MAKE_FLAGS = rv32c=$(rv32c) rv32e=$(rv32e) rv32b=$(rv32b)

.PHONY: $(SUBDIRS) tools tests coverage

help:
	@echo "make all         build all diags and run the RTL sim"
	@echo "make all-sw      build all diags and run the ISS sim"
	@echo "make tests-all   run all diags and compliance test"
	@echo "make coverage    generate code coverage report"
	@echo "make build       build all diags and the RTL"
	@echo "make dhrystone   build Dhrystone diag and run the RTL sim"
	@echo "make coremark    build Coremark diag and run the RTL sim"
	@echo "make clean       clean"
	@echo "make distclean   clean all"
	@echo ""
	@echo "rv32c=1          enable RV32C (default off)"
	@echo "rv32e=1          enable RV32E (default off)"
	@echo "rv32b=1          enable RV32B (default off)"
	@echo "debug=1          enable waveform dump (default off)"
	@echo "coverage=1       enable coverage test (default off)"
	@echo "test_v=[1|2]     run test compliance v1 or v2 (default)"
	@echo ""
	@echo "For example"
	@echo ""
	@echo "$ make tests-all             run all tests with test compliance v1"
	@echo "$ make test_v=2 tests-all    run all tests with test compliance v2"
	@echo "$ make coverage=1 tests-all  run all tests with code coverage report"
	@echo "$ make debug=1 hello         run hello with waveform dump"
	@echo "$ make rv32e=1 dhrystone     run dhrystone for RV32E config"
	@echo ""

tests-all: tests-sw tests all all-sw

all:
	$(MAKE) clean
	for i in $(SUBDIRS); do \
		$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) $$i || exit 1; \
	done

all-sw:
	for i in $(SUBDIRS); do \
		$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) -C tools $$i.elf; \
	done

# Architectural test: v1 needs 256MB, v2 needs 1716MB
tests:
	$(MAKE) coverage=$(coverage) clean
	$(MAKE) $(MAKE_FLAGS) memsize=$(if $(filter 1,$(test_v)),256,1716) -C sim
	$(MAKE) $(MAKE_FLAGS) memsize=$(if $(filter 1,$(test_v)),256,1716) -C tools
	$(MAKE) $(MAKE_FLAGS) memsize=$(if $(filter 1,$(test_v)),256,1716) test_v=$(test_v) -C tests tests

tests-sw:
	$(MAKE) coverage=$(coverage) clean
	$(MAKE) $(MAKE_FLAGS) memsize=$(if $(filter 1,$(test_v)),256,1716) -C tools
	$(MAKE) $(MAKE_FLAGS) memsize=$(if $(filter 1,$(test_v)),256,1716) test_v=$(test_v) -C tests tests-sw

build:
	for i in sw sim tools; do \
		$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) -C $$i; \
	done

$(SUBDIRS):
	@$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) -C sw $@
	@$(MAKE) $(if $(_verilator), verilator=1) \
			 $(if $(_coverage), coverate=1) \
			 $(if $(_top), top=1) $(MAKE_FLAGS) memsize=$(memsize) debug=$(debug) -C sim $@.elf
	@$(MAKE) $(if $(_top), top=1) $(MAKE_FLAGS) memsize=$(memsize) -C tools $@.elf
	@echo "Compare the trace between RTL and ISS simulator"
	@diff --brief sim/trace.log tools/trace.log
	@echo === Simulation passed ===

coverage: clean
	@$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) coverage=1 all
	@mv sim/*_cov.dat coverage/.
	@$(MAKE) $(MAKE_FLAGS) memsize=$(memsize) coverage=1 tests
	@if [ "$(test_v)" = "1" ]; then \
	    mv tests/riscv-arch-test.v1/work/rv32i/*_cov.dat coverage/.; \
	    mv tests/riscv-arch-test.v1/work/rv32im/*_cov.dat coverage/.; \
	    mv tests/riscv-arch-test.v1/work/rv32Zicsr/*_cov.dat coverage/.; \
	else \
	    mv tests/riscv-arch-test.v2/work/rv32i_m/I/*_cov.dat coverage/.; \
	    mv tests/riscv-arch-test.v2/work/rv32i_m/M/*_cov.dat coverage/.; \
	    if [ "$(rv32b)" = "1" ]; then \
	        mv tests/riscv-arch-test.v2/work/rv32i_m/B/*_cov.dat coverage/.; \
	    fi; \
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

