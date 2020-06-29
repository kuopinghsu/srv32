CC       = gcc
SYS     := $(shell gcc -dumpmachine)

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
	@if [ -f ../sw/$*/$* ]; then \
		./$(TARGET) -l trace.log ../sw/$*/$*; \
	else \
		echo "file ../sw/$*/$* not found"; \
	fi

clean:
	@$(RM) $(OBJECTS) trace.log $(TARGET)

