# Build flags
BFLAGS = -f filelist.txt

# Run flags
RFLAGS = +trace

TARGET = riscsim

all: $(TARGET)

$(TARGET): $(SRC)
	iverilog $(BFLAGS) -o $(TARGET)

run: $(TARGET) checkcode.awk
	./$(TARGET) $(RFLAGS) | awk -f $(filter %.awk, $^)

clean:
	@$(RM) $(TARGET) testbench.vcd trace.log
