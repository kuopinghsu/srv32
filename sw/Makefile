SUBDIRS = hello dhrystone coremark qsort

.PHONY: $(SUBDIRS) common clean

all: hello

$(SUBDIRS): common
	$(MAKE) -C $@

common:
	$(MAKE) -C $@

clean:
	for i in $(SUBDIRS) common; do \
		$(MAKE) -C $$i clean; \
	done
