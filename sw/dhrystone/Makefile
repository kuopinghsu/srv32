include ../common/Makefile.common

SRC      = dhry_1.c dhry_2.c syscall.c
CFLAGS  += -DTIME -DRISCV -DHZ=100000000 -L../common
CFLAGS  += -Wno-return-type -Wno-implicit-function-declaration -Wno-implicit-int
LDFLAGS += -T ../common/default.ld
TARGET   = dhrystone

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	$(OBJCOPY) -j .text -O binary $(TARGET) imem.bin
	$(OBJCOPY) -j .data -O binary $(TARGET) dmem.bin
	$(OBJCOPY) -O binary $(TARGET) memory.bin
	$(OBJDUMP) -d $(TARGET) > $(TARGET).dis
	$(READELF) -a $(TARGET) > $(TARGET).symbol

clean:
	$(RM) *.o $(TARGET) $(TARGET).dis $(TARGET).symbol [id]mem.bin memory.bin
