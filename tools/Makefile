CC          = gcc
SYS        := $(shell gcc -dumpmachine)
SINGLE_RAM ?= 0

ifneq (, $(findstring darwin, $(SYS)))
# to enable bfd library, install the binutils with
# "--prefix=/usr/local/Cellar/binutils/2.34 --enable-install-libiberty"
# on MacOS.
BFD_PATH = /usr/local/Cellar/binutils/2.34
CFLAGS   = -DMACOX -DPACKAGE -I$(BFD_PATH)/include
LDFLAGS  = -L$(BFD_PATH)/lib -liberty -undefined dynamic_lookup
else
CFLAGS   = -Wno-stringop-truncation
endif

CFLAGS  += -O2 -g -Wall
LDFLAGS += -lbfd

ifeq ($(SINGLE_RAM), 1)
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
	@$(RM) $(OBJECTS) trace.log $(TARGET)

distclean: clean

