DEBUG    ?= 0
top      ?= 0
coverage ?= 0
memsize  ?= 128
rv32c    ?= 0
CC        = gcc
SYS      := $(shell gcc -dumpmachine)

ifneq (, $(findstring darwin, $(SYS)))
CFLAGS  += -DMACOX
endif

ifeq ($(DEBUG), 1)
CFLAGS  += -O0 -g -Wall
else
CFLAGS  += -O3 -g -Wall
endif

ifeq ($(coverage), 1)
CFLAGS  += -fprofile-arcs -ftest-coverage
LDFLAGS += -fprofile-arcs -ftest-coverage
endif

ifeq ($(top), 1)
CFLAGS  += -DSINGLE_RAM
endif

ifeq ($(rv32c), 1)
CFLAGS  += -DRV32C_ENABLED
endif

SRC      = rvsim.c decompress.c syscall.c elfread.c getch.c
OBJECTS  = $(SRC:.c=.o)
RVSIM   = rvsim

.SUFFIXS: .c .o

.PHONY: clean

%.o: %.c opcode.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(RVSIM): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(RVSIM) $(OBJECTS)

%.run: $(RVSIM)
	@if [ ! -f ../sw/$*/$*.elf ]; then \
		make -C ../sw $*; \
	fi
	./$(RVSIM) --memsize $(memsize) -l trace.log ../sw/$*/$*.elf
	@./log2dis.pl -q trace.log ../sw/$*/$*.elf

coverage: coverage_extra
	@gcov *.c
	@lcov -c -o coverage.info -d .
	@genhtml coverage.info -o html

coverage_extra:
	-@$(MAKE) irq.run
	-@$(MAKE) sem.run
	-@./$(RVSIM) -q
	-@./$(RVSIM) -h
	-@./$(RVSIM) dummy
	-@./$(RVSIM) -m 0 -n 0x20000 -b 1 -s -p -l trace.log ../sw/hello/hello.elf
	-@./$(RVSIM) -m 0x0 -n 131072 -b 1 -s -p -l trace.log ../sw/hello/hello.elf

clean:
	-$(RM) $(OBJECTS) dump.txt trace.log trace.log.dis $(RVSIM) out.bin
	-if [ $(coverage) = 0 ]; then \
		$(RM) -rf html coverage.info *.gcda *.gcno *.gcov; \
	fi

distclean: clean

