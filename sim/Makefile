# use verilator
VERILATOR  ?= 0
DEBUG_DUMP ?= 1

# Run flags
RFLAGS      = +trace

TARGET      = riscsim

TARGET_SIM  = iverilog

ifeq ($(VERILATOR),1)
BFLAGS      = -O3 -sc -Wall -Wno-STMTDLY -Wno-UNUSED \
              $(if $(DEBUG_DUMP), --trace,) \
              $(if $(DEBUG_DUMP), -DDUMP=1,) \
              --Mdir sim --build --exe sim_main.cpp
TARGET_SIM  = verilator
endif

# Build flags
BFLAGS     += -f filelist.txt

all: $(TARGET)

$(TARGET):
	$(TARGET_SIM) $(BFLAGS) -o $(TARGET)
	@if [ "$(VERILATOR)" = "1" ]; then \
		mv sim/riscsim .; \
	fi

%.run: $(TARGET) checkcode.awk
	@if [ ! -f ../sw/$*/memory.bin ]; then \
		make -C ../sw $*; \
	fi
	cp ../sw/$*/*.bin .
	./$(TARGET) $(RFLAGS) | awk -f $(filter %.awk, $^)

clean:
	@$(RM) $(TARGET) testbench.vcd trace.log *.bin dump.txt
	@$(RM) -rf sim

distclean: clean

