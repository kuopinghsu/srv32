
include ../common/Makefile.common

EXE      = .elf
TARGET   = $(shell basename `pwd`)
OUTPUT   = $(TARGET)$(EXE)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OUTPUT)
	$(OBJDUMP) -d $(OUTPUT) > $(TARGET).dis
	$(READELF) -a $(OUTPUT) > $(TARGET).symbol

clean:
	$(RM) *.o $(TARGET).dis $(TARGET).symbol
