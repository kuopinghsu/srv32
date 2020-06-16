CC       = gcc
SYS     := $(shell gcc -dumpmachine)

ifneq (, $(findstring darwin, $(SYS)))
CFLAGS   = -DMACOX
LDFLAGS  =
else
CFLAGS   = -Wno-stringop-truncation
endif

CFLAGS  += -O2 -Wall
LDFLAGS +=

SRC      = gentrace.c
TARGET   = gentrace

.PHONY: $(TARGET) run

all: $(TARGET)

$(TARGET): $(SRC) opcode.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET) -l trace.log

clean:
	@$(RM) *.o trace.log $(TARGET)
