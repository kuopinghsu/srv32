DEBUG    ?= 0
top      ?= 0
coverage ?= 0
tracelog ?= 0
memsize  ?= 256
rv32m    ?= 1
rv32c    ?= 0
rv32e    ?= 0
rv32b    ?= 0
CC        = gcc
SYS      := $(shell gcc -dumpmachine)

CFLAGS  += -DGDBSTUB

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

ifeq ($(rv32m), 1)
CFLAGS  += -DRV32M_ENABLED=1
endif

ifeq ($(rv32c), 1)
CFLAGS  += -DRV32C_ENABLED=1
endif

ifeq ($(rv32e), 1)
CFLAGS  += -DRV32E_ENABLED=1
endif

ifeq ($(rv32b), 1)
CFLAGS  += -DRV32B_ENABLED=1
endif

ifeq ($(tracelog), 1)
TRACELOG = -l trace.log
else
TRACELOG =
endif

LDFLAGS += -Lmini-gdbstub/build -lgdbstub

SRC      = rvsim.c decompress.c syscall.c elfloader.c getch.c htif.c \
           debug.c riscv-disas.c gdbstub.c map.c
OBJECTS  = $(SRC:.c=.o)
RVSIM   = rvsim

.SUFFIXS: .c .o

.PHONY: clean

all: mini-gdbstub $(RVSIM)

%.o: %.c opcode.h
	$(CC) -DMEMSIZE=$(memsize) -c -o $@ $< $(CFLAGS)

$(RVSIM): $(OBJECTS) libgdbstub.a
	$(CC) $(CFLAGS) -o $(RVSIM) $(OBJECTS) $(LDFLAGS)

mini-gdbstub:
	@if [ ! -d mini-gdbstub ]; then \
		echo "clone mini-gdbstub"; \
		git clone https://github.com/RinHizakura/mini-gdbstub.git; \
	fi

libgdbstub.a:
	CC=gcc make -C mini-gdbstub

%.elf: $(RVSIM)
	@if [ ! -f ../sw/$*/$*.elf ]; then \
		$(MAKE) rv32m=$(rv32m) rv32c=$(rv32c) rv32e=$(rv32e) rv32b=$(rv32b) memsize=$(memsize) -C ../sw $*; \
	fi
	@rm -rf trace.log
	./$(RVSIM) --memsize $(memsize) $(TRACELOG) ../sw/$*/$*.elf
	@if [ -f trace.log ]; then ./log2dis.pl -q trace.log ../sw/$*/$*.elf; fi

coverage: coverage_extra
	@gcov *.c
	@lcov -c -o coverage.info -d .
	@genhtml coverage.info -o html

coverage_extra:
	-@$(MAKE) coverage=1
	-@$(MAKE) coverage=1 irq.elf
	-@$(MAKE) coverage=1 sem.elf
	-@$(MAKE) coverage=1 exception.elf
	-@$(MAKE) coverage=1 test_cov.elf
	-@$(MAKE) coverage=1 -C ../sw/_io
	-@echo Hello | $(MAKE) coverage=1 _io.elf
	-@$(MAKE) coverage=1 -C ../sw/_file
	-@$(MAKE) coverage=1 _file.elf
	-@./$(RVSIM) -q
	-@./$(RVSIM) -h
	-@./$(RVSIM) -d
	-@./$(RVSIM) dummy
	-@./$(RVSIM) -m 0 -n 0x20000 -b 1 -s -p -l trace.log
	-@./$(RVSIM) -m 0 -n 0x20000 -b 1 -s -p -l trace.log ../sw/hello/hello.elf
	-@./$(RVSIM) -m 0x0 -n 131072 -b 1 -s -p -l trace.log ../sw/hello/hello.elf

clean:
	@if [ -d mini-gdbstub ]; then make -C mini-gdbstub clean; fi
	-$(RM) $(OBJECTS) dump.txt trace.log trace.log.dis $(RVSIM) out.bin
	-@if [ $(coverage) = 0 ]; then \
		$(RM) -rf html coverage.info *.gcda *.gcno *.gcov; \
	fi

distclean: clean

