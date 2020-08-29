dirs        = $(dir $(wildcard sw/*/))
SUBDIRS     = $(subst /,,$(subst sw/,,$(subst common,,$(dirs))))

verilator ?= 1
top       ?= 0
coverage  ?= 0

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
	@echo "make all-sw      build all diags and run the software sim"
	@echo "make tests-all   run all diags and compliance test"
	@echo "make coverage    generate code coverage report"
	@echo "make build       build all diags and the RTL"
	@echo "make dhrystone   build Dhrystone diag and run the RTL sim"
	@echo "make coremark    build Coremark diag and run the RTL sim"
	@echo "make clean       clean"
	@echo "make distclean   clean all"

tests-all: tests-sw tests all all-sw

all:
	for i in $(SUBDIRS); do \
		$(MAKE) $$i; \
	done

all-sw:
	for i in $(SUBDIRS); do \
		$(MAKE) -C tools $$i.run; \
	done

tests:
	make -C sim; make -C tools
	make -C tests tests

tests-sw:
	make clean && make -C sim; make -C tools
	make -C tests tests-sw

build:
	for i in sw sim tools; do \
		$(MAKE) -C $$i; \
	done

$(SUBDIRS):
	@$(MAKE) -C sw $@
	@$(MAKE) $(if $(_verilator), verilator=1) \
			 $(if $(_coverage), coverate=1) \
			 $(if $(_top), top=1) -C sim $@.run
	@$(MAKE) $(if $(_top), top=1) -C tools $@.run
	@echo "Compare the trace between RTL and software simulator"
	@diff --brief sim/trace.log tools/trace.log
	@echo === Simulation passed ===

coverage:
	@mkdir -p coverage
	@$(MAKE) coverage=1 all
	@mv sim/*_cov.dat coverage/.
	@$(MAKE) coverage=1 tests
	@mv tests/riscv-compliance/work/rv32i/*_cov.dat coverage/.
	@mv tests/riscv-compliance/work/rv32im/*_cov.dat coverage/.
	@$(MAKE) -C coverage

clean:
	for i in sw sim tools tests coverage; do \
		$(MAKE) -C $$i clean; \
	done

distclean:
	for i in sw sim tools tests coverage; do \
		$(MAKE) -C $$i distclean; \
	done

