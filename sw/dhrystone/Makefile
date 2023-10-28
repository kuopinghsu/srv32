include ../common/Makefile.common

EXE      = .elf
SRC      = dhry_1.c dhry_2.c syscall.c
CFLAGS  += -DTIME -DRISCV -DHZ=100000000 -L../common
CFLAGS  += -Wno-return-type -Wno-implicit-function-declaration -Wno-implicit-int
CFLAGS  += -fno-common -funroll-loops -finline-functions -falign-functions=16
CFLAGS  += -falign-jumps=4 -falign-loops=4 -finline-limit=1000
#CFLAGS += -fno-if-conversion2 -fselective-scheduling -fno-tree-dominator-opts
#CFLAGS += -fno-reg-struct-return -fno-rename-registers
#CFLAGS += --param case-values-threshold=8 -fno-crossjumping
#CFLAGS += -freorder-blocks-and-partition -fno-tree-loop-if-convert
#CFLAGS  += -fno-tree-sink -fgcse-sm -fno-strict-overflow
LDFLAGS += -T ../common/default.ld
TARGET   = dhrystone
OUTPUT   = $(TARGET)$(EXE)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SRC) $(LDFLAGS)
	$(OBJDUMP) -d $(OUTPUT) > $(TARGET).dis
	$(READELF) -a $(OUTPUT) > $(TARGET).symbol

clean:
	$(RM) *.o $(OUTPUT) $(TARGET).dis $(TARGET).symbol
