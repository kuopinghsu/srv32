
include ../common/Makefile.common

SRC      = qsort.c
CFLAGS  += -L../common
LDFLAGS += -T ../common/default.ld
TARGET   = qsort

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
