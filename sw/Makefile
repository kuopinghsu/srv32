include common/Makefile.common

memsize ?= 256

MAKE_FLAGS = memsize=$(memsize)

dirs = $(filter %/, $(wildcard [^_]*/))
SUBDIRS = $(subst /,,$(subst common,,$(dirs)))

.PHONY: $(SUBDIRS) common clean

all: $(SUBDIRS)

$(SUBDIRS): common
	@if [ -f $@/Makefile ]; then \
		$(MAKE) $(MAKE_FLAGS) -C $@; \
	fi

common:
	$(MAKE) $(MAKE_FLAGS) -C $@

update:
	@for i in $(dirs:%/=%); do \
		if [ -f "$$i/$$i.elf" ]; then \
			echo $$i && cd $$i; \
			$(OBJDUMP) -d $$i.elf > $$i.dis; \
			$(READELF) -a $$i.elf > $$i.symbol; \
			cd ..; \
		fi \
	done

clean:
	@for i in $(SUBDIRS) common _*; do \
		if [ -f $$i/Makefile ]; then \
			$(MAKE) $(MAKE_FLAGS) -C $$i clean; \
		fi; \
	done

distclean: clean

