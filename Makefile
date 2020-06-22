SUBDIRS = hello dhrystone coremark qsort

.PHONY: $(SUBDIRS) tools

help:
	@echo "make all         build all diags and run the RTL sim"
	@echo "make build       build all diags and the RTL"
	@echo "make hello       build hello diag and run the RTL sim"
	@echo "make dhrystone   build Dhrystone diag and run the RTL sim"
	@echo "make coremark    build Coremark diag and run the RTL sim"
	@echo "make qsort       build qosrt diag and run the RTL sim"
	@echo "make clean       clean"

all:
	for i in $(SUBDIRS); do \
		$(MAKE) $$i; \
	done

build:
	for i in sw sim tools; do \
		$(MAKE) -C $$i; \
	done

$(SUBDIRS): clean
	@$(MAKE) -C sw $@ && $(MAKE) -C sim run && $(MAKE) -C tools run
	@echo "Compare the trace between RTL and software simulator"
	@diff --brief sim/trace.log tools/trace.log
	@echo === Simulation passed ===

clean:
	for i in sw sim tools; do \
		$(MAKE) -C $$i clean; \
	done

