DEBUG      ?= 0
top        ?= 0
CC          = gcc
SYS        := $(shell gcc -dumpmachine)

ifneq (, $(findstring darwin, $(SYS)))
CFLAGS  += -DMACOX
endif

ifeq ($(DEBUG), 1)
CFLAGS  += -O0 -g -Wall
else
CFLAGS  += -O3 -g -Wall
endif

ifeq ($(top), 1)
CFLAGS  += -DSINGLE_RAM
endif

SRC      = gentrace.c elfread.c
OBJECTS  = $(SRC:.c=.o)
TARGET   = gentrace

.SUFFIXS: .c .o

.PHONY: clean

%.o: %.c opcode.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS)

%.run: $(TARGET)
	@if [ ! -f ../sw/$*/$*.elf ]; then \
		make -C ../sw $*; \
	fi
	./$(TARGET) -l trace.log ../sw/$*/$*.elf

clean:
	@$(RM) $(OBJECTS) dump.txt trace.log $(TARGET)

distclean: clean

