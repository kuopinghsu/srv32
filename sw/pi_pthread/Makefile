
include ../common/Makefile.common

EXE      = .elf
TARGET   = $(shell basename `pwd`)
OUTPUT   = $(TARGET)$(EXE)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OUTPUT)
	$(OBJCOPY) -j .reset -j .text -O binary $(OUTPUT) imem.bin
	$(OBJCOPY) -R .reset -R .text -O binary $(OUTPUT) dmem.bin
	$(OBJCOPY) -O binary $(OUTPUT) memory.bin
	$(OBJDUMP) -d $(OUTPUT) > $(TARGET).dis
	$(READELF) -a $(OUTPUT) > $(TARGET).symbol

clean:
	$(RM) *.o $(TARGET).dis $(TARGET).symbol [id]mem.bin memory.bin
