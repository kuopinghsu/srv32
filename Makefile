dirs        = $(dir $(wildcard sw/[^_]*/))
SUBDIRS     = $(subst /,,$(subst sw/,,$(subst common,,$(dirs))))

verilator ?= 1
top       ?= 0
coverage  ?= 0

# set 1 for compliance test v1, 2 for v2
test_v    ?= 1

# set 1 to enable rv32c
rv32c     ?= 0

# memory size requirement for compliance test
ifeq ($(test_v), 1)
    memsize ?= 128
else
    memsize ?= 1716
endif

ifeq ($(verilator), 1)
    _verilator := 1
endif

ifeq ($(top), 1)
    _top := 1
endif

ifeq ($(coverage), 1)
    _coverage := 1
endif

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

tests-all: tests-sw tests all all-sw

all:
	$(MAKE) clean
	for i in $(SUBDIRS); do \
		$(MAKE) rv32c=$(rv32c) $$i || exit 1; \
	done

all-sw:
	for i in $(SUBDIRS); do \
		$(MAKE) rv32c=$(rv32c) -C tools $$i.run; \
	done

tests:
	$(MAKE) coverage=$(coverage) clean && $(MAKE) rv32c=$(rv32c) memsize=$(memsize) -C sim; $(MAKE) rv32c=$(rv32c) -C tools
	$(MAKE) memsize=$(memsize) test_v=$(test_v) rv32c=$(rv32c) -C tests tests

tests-sw:
	$(MAKE) coverage=$(coverage) clean && $(MAKE) rv32c=$(rv32c) memsize=$(memsize) -C sim; $(MAKE) rv32c=$(rv32c) -C tools
	$(MAKE) memsize=$(memsize) test_v=$(test_v) rv32c=$(rv32c) -C tests tests-sw

build:
	for i in sw sim tools; do \
		$(MAKE) rv32c=$(rv32c) -C $$i; \
	done

$(SUBDIRS):
	@$(MAKE) rv32c=$(rv32c) -C sw $@
	@$(MAKE) $(if $(_verilator), verilator=1) \
			 $(if $(_coverage), coverate=1) \
			 $(if $(_top), top=1) rv32c=$(rv32c) -C sim $@.run
	@$(MAKE) $(if $(_top), top=1) rv32c=$(rv32c) -C tools $@.run
	@echo "Compare the trace between RTL and ISS simulator"
	@diff --brief sim/trace.log tools/trace.log
	@echo === Simulation passed ===

coverage: clean
	@$(MAKE) rv32c=$(rv32c) coverage=1 all
	@mv sim/*_cov.dat coverage/.
	@$(MAKE) rv32c=$(rv32c) coverage=1 tests
	@if [ "$(test_v)" = "1" ]; then \
	    mv tests/riscv-compliance.v1/work/rv32i/*_cov.dat coverage/.; \
	    mv tests/riscv-compliance.v1/work/rv32im/*_cov.dat coverage/.; \
	    mv tests/riscv-compliance.v1/work/rv32Zicsr/*_cov.dat coverage/.; \
	else \
	    mv tests/riscv-compliance.v2/work/rv32i_m/I/*_cov.dat coverage/.; \
	    mv tests/riscv-compliance.v2/work/rv32i_m/M/*_cov.dat coverage/.; \
	fi
	@$(MAKE) rv32c=$(rv32c) -C coverage
	@$(MAKE) rv32c=$(rv32c) -C tools coverage

clean:
	for i in sw sim tools tests coverage; do \
		$(MAKE) -C $$i clean; \
	done

distclean:
	for i in sw sim tools tests coverage; do \
		$(MAKE) -C $$i distclean; \
	done

