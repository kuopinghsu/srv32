# Build flags
BFLAGS = -f filelist.txt

# Run flags
RFLAGS = +trace

TARGET = riscsim

all: $(TARGET)

$(TARGET): $(SRC)
	iverilog $(BFLAGS) -o $(TARGET)

%.run: $(TARGET) checkcode.awk
	@if [ ! -f ../sw/$*/memory.bin ]; then \
		echo "no init file found, it needs to build the diags on the sw directory."; exit 1; \
	else \
		cp ../sw/$*/*.bin .; \
	fi
	./$(TARGET) $(RFLAGS) | awk -f $(filter %.awk, $^)

clean:
	@$(RM) $(TARGET) testbench.vcd trace.log *.bin dump.txt

distclean: clean

