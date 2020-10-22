DEBUG    ?= 0
top      ?= 0
coverage ?= 0
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

SRC      = rvsim.c elfread.c getch.c
OBJECTS  = $(SRC:.c=.o)
TARGET   = rvsim

.SUFFIXS: .c .o

.PHONY: clean

%.o: %.c opcode.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJECTS)

%.run: $(TARGET)
	@if [ ! -f ../sw/$*/$*.elf ]; then \
		make -C ../sw $*; \
	fi
	./$(TARGET) -l trace.log ../sw/$*/$*.elf
	@./log2dis.pl trace.log ../sw/$*/$*.elf

coverage: coverage_extra
	@gcov *.c
	@lcov -c -o coverage.info -d .
	@genhtml coverage.info -o html

coverage_extra:
	-@./$(TARGET) -q
	-@./$(TARGET) -h
	-@./$(TARGET) dummy
	-@./$(TARGET) -m 0 -n 0x20000 -b 1 -s -p -l trace.log ../sw/hello/hello.elf
	-@./$(TARGET) -m 0x0 -n 131072 -b 1 -s -p -l trace.log ../sw/hello/hello.elf

clean:
	@$(RM) $(OBJECTS) dump.txt trace.log trace.log.dis $(TARGET)
	@$(RM) -rf html coverage.info *.gcda *.gcno *.gcov

distclean: clean

