
include ../common/Makefile.common

SRC      = hello.c
CFLAGS  += -L../common
LDFLAGS += -T ../common/default.ld
TARGET   = hello

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	$(OBJCOPY) -j .text -O binary $(TARGET) ../imem.bin
	$(OBJCOPY) -j .data -O binary $(TARGET) ../dmem.bin
	$(OBJDUMP) -d $(TARGET) > $(TARGET).dis
	$(READELF) -a $(TARGET) > $(TARGET).symbol
	cd .. && ./common/bin2hex.pl

clean:
	$(RM) *.o $(TARGET) $(TARGET).dis $(TARGET).symbol ../[id]mem.*
