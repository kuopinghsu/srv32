SUBDIRS = hello dhrystone coremark qsort

.PHONY: $(SUBDIRS) tools tests

help:
	@echo "make all         build all diags and run the RTL sim"
	@echo "make all-sw      build all diags and run the software sim"
	@echo "make tests-all   run the RTL and simulator compliance test"
	@echo "make build       build all diags and the RTL"
	@echo "make dhrystone   build Dhrystone diag and run the RTL sim"
	@echo "make coremark    build Coremark diag and run the RTL sim"
	@echo "make clean       clean"
	@echo "make distclean   clean all"

all:
	for i in $(SUBDIRS); do \
		$(MAKE) $$i; \
	done

all-sw:
	for i in $(SUBDIRS); do \
		$(MAKE) -C tools $$i.run; \
	done

tests:
	make clean && make -C sim; make -C tools
	make -C tests tests

tests-sw:
	make clean && make -C sim; make -C tools
	make -C tests tests-sw

tests-all: tests-sw tests

build:
	for i in sw sim tools; do \
		$(MAKE) -C $$i; \
	done

$(SUBDIRS): clean
	@$(MAKE) -C sw $@ && $(MAKE) -C sim $@.run && $(MAKE) -C tools $@.run
	@echo "Compare the trace between RTL and software simulator"
	@diff --brief sim/trace.log tools/trace.log
	@echo === Simulation passed ===

clean:
	for i in sw sim tools tests; do \
		$(MAKE) -C $$i clean; \
	done

distclean:
	for i in sw sim tools tests; do \
		$(MAKE) -C $$i distclean; \
	done

