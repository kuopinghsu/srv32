# Build flags
BFLAGS = -f filelist.txt

# Run flags
RFLAGS = +trace

TARGET = riscsim

all: $(TARGET)

$(TARGET): $(SRC)
	iverilog $(BFLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(RFLAGS)

clean:
	@$(RM) $(TARGET) testbench.vcd trace.log
