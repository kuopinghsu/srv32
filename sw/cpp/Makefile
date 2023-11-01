
include ../common/Makefile.common

EXE      = .elf
SRC      = cpp.cc
CFLAGS  += -L../common -Oz -flto -fno-exceptions
LDFLAGS += -T ../common/default.ld -flto -fno-exceptions
TARGET   = cpp
OUTPUT   = $(TARGET)$(EXE)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CFLAGS) -o $(OUTPUT) $(SRC) $(LDFLAGS)
	$(OBJDUMP) -d $(OUTPUT) > $(TARGET).dis
	$(READELF) -a $(OUTPUT) > $(TARGET).symbol

clean:
	$(RM) *.o $(OUTPUT) $(TARGET).dis $(TARGET).symbol
