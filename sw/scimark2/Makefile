include ../common/Makefile.common

EXE      = .elf
SRC      = FFT.c LU.c MonteCarlo.c Random.c SOR.c SparseCompRow.c Stopwatch.c \
           array.c kernel.c scimark2.c
CFLAGS  += -DCLOCKS_PER_SEC=100000 -L../common
CFLAGS  += -Wno-return-type -Wno-implicit-function-declaration -Wno-implicit-int
LDFLAGS += -T ../common/default.ld -lm
TARGET   = scimark2
OUTPUT   = $(TARGET)$(EXE)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SRC) $(LDFLAGS)
	$(OBJDUMP) -d $(OUTPUT) > $(TARGET).dis
	$(READELF) -a $(OUTPUT) > $(TARGET).symbol

clean:
	$(RM) *.o $(OUTPUT) $(TARGET).dis $(TARGET).symbol
