include common/Makefile.common

SUBDIRS = hello dhrystone coremark qsort
dirs = $(filter %/, $(wildcard */))

.PHONY: $(SUBDIRS) common clean

all: $(SUBDIRS)

$(SUBDIRS): common
	$(MAKE) -C $@

common:
	$(MAKE) -C $@

update:
	@for i in $(dirs:%/=%); do \
		if [ -f "$$i/$$i.elf" ]; then \
			echo $$i && cd $$i; \
			$(OBJCOPY) -j .text -O binary $$i.elf imem.bin; \
			$(OBJCOPY) -j .data -O binary $$i.elf dmem.bin; \
			$(OBJCOPY) -O binary $$i.elf memory.bin; \
			$(OBJDUMP) -d $$i.elf > $$i.dis; \
			cd ..; \
		fi \
	done

clean:
	for i in $(SUBDIRS) common; do \
		$(MAKE) -C $$i clean; \
	done

distclean: clean

