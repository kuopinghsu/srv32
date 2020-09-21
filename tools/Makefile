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

SRC      = rvsim.c elfread.c
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

coverage:
	@gcov *.c
	@lcov -c -o coverage.info -d .
	@genhtml coverage.info -o html

clean:
	@$(RM) $(OBJECTS) dump.txt trace.log $(TARGET)
	@$(RM) -rf html coverage.info *.gcda *.gcno

distclean: clean

