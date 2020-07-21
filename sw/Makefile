include common/Makefile.common

SUBDIRS = hello dhrystone coremark qsort

.PHONY: $(SUBDIRS) common clean

all: $(SUBDIRS)

$(SUBDIRS): common
	$(MAKE) -C $@

common:
	$(MAKE) -C $@

%.update:
	if [ -d $* ]; then \
		pushd $*; \
		$(OBJCOPY) -j .text -O binary $*.elf imem.bin; \
		$(OBJCOPY) -j .data -O binary $*.elf dmem.bin; \
		$(OBJCOPY) -O binary $*.elf memory.bin; \
		$(OBJDUMP) -d $*.elf > $*.dis; \
		popd; \
	fi

clean:
	for i in $(SUBDIRS) common; do \
		$(MAKE) -C $$i clean; \
	done

distclean: clean

