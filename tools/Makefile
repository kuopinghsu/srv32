DEBUG      ?= 0
CC          = gcc
SYS        := $(shell gcc -dumpmachine)
top        ?= 0

ifneq (, $(findstring darwin, $(SYS)))
# to enable bfd library, install the binutils with
# "--prefix=/usr/local/Cellar/binutils/2.35 --enable-install-libiberty"
# on MacOS.
ifneq (,$(wildcard /opt/riscv/include/bfd.h))
BFD_PATH = /opt/riscv
else
BFD_PATH = /usr/local/Cellar/binutils/2.35
endif
CFLAGS   = -DMACOX -I$(BFD_PATH)/include
LDFLAGS  = -L$(BFD_PATH)/lib -liberty -lintl -lz #-undefined dynamic_lookup
else
CFLAGS   = -Wno-stringop-truncation
endif

ifeq ($(DEBUG), 1)
CFLAGS  += -O0 -g -Wall
else
CFLAGS  += -O3 -g -Wall
endif

LDFLAGS += -lbfd

ifeq ($(top), 1)
CFLAGS  += -DSINGLE_RAM
endif

SRC      = gentrace.c
OBJECTS  = $(SRC:.c=.o)
TARGET   = gentrace

.SUFFIXS: .c .o

.PHONY: clean

%.o: %.c opcode.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.run: $(TARGET)
	@if [ ! -f ../sw/$*/$*.elf ]; then \
		make -C ../sw $*; \
	fi
	./$(TARGET) -l trace.log ../sw/$*/$*.elf

clean:
	@$(RM) $(OBJECTS) dump.txt trace.log $(TARGET)

distclean: clean

