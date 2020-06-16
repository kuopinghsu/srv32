FLAGS = -f filelist.txt
TARGET = riscsim

all: $(TARGET)

$(TARGET): $(SRC)
	iverilog $(FLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	@$(RM) $(TARGET) testbench.vcd trace.log
