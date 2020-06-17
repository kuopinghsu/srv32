SUBDIRS = hello dhrystone coremark

.PHONY: $(SUBDIRS) tools

help:
	@echo "make all         build all diags and run the RTL sim"
	@echo "make hello       build hello diag and run the RTL sim"
	@echo "make dhrystone   build Dhrystone diag and run the RTL sim"
	@echo "make coremark    build Coremark diag and run the RTL sim"
	@echo "make clean       clean"

all:
	for i in $(SUBDIRS); do \
		$(MAKE) $$i; \
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

