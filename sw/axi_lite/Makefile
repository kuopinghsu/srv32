include ../common/Makefile.common

.PHONY: all clean

all: axi_lite.hex

axi_lite.elf: axi_lite.S
	$(CC) -march=rv32i -mabi=ilp32 -nostdlib -Wl,-Ttext=0,--no-relax,--build-id=none -o $@ $<

axi_lite.hex: axi_lite.elf
	$(OBJCOPY) -O verilog --verilog-data-width=4 -j .text $< $@

clean:
	$(RM) axi_lite.elf axi_lite.hex
