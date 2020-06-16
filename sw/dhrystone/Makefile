include ../common/Makefile.common

SRC      = dhry_1.c dhry_2.c syscall.c
CFLAGS  += -flto -DTIME -DRISCV -DHZ=100000000 -L../common
LDFLAGS += -T ../common/default.ld
TARGET   = dhrystone

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
