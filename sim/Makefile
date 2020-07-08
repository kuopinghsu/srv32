# use verilator
VERILATOR  ?= 0

# Run flags
RFLAGS      = +trace

TARGET      = riscsim

TARGET_SIM  = iverilog

ifeq ($(VERILATOR),1)
BFLAGS      = -O3 -cc -Wno-STMTDLY --exe --build sim_main.cpp
TARGET_SIM  = verilator
endif

# Build flags
BFLAGS     += -f filelist.txt

all: $(TARGET)

$(TARGET): $(SRC)
	$(TARGET_SIM) $(BFLAGS) -o $(TARGET)

%.run: $(TARGET) checkcode.awk
	@if [ ! -f ../sw/$*/memory.bin ]; then \
		make -C ../sw $*; \
	fi
	cp ../sw/$*/*.bin .
	./$(TARGET) $(RFLAGS) | awk -f $(filter %.awk, $^)

clean:
	@$(RM) $(TARGET) testbench.vcd trace.log *.bin dump.txt

distclean: clean

