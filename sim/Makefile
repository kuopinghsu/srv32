# use verilator
VERILATOR  ?= 0
DEBUG_DUMP ?= 1
SINGLE_RAM ?= 0

# Run flags
RFLAGS      = +trace

TARGET      = riscsim

TARGET_SIM  = iverilog

ifeq ($(SINGLE_RAM),1)
    single_ram := 1
endif

ifeq ($(DEBUG_DUMP),1)
    debug_dump := 1
endif

ifeq ($(VERILATOR),1)
BFLAGS      = -O3 -sc -Wall -Wno-STMTDLY -Wno-UNUSED \
              $(if $(debug_dump), --trace) \
              $(if $(debug_dump), -DDUMP=1) \
              $(if $(single_ram), +define+SINGLE_RAM) \
              --Mdir sim --build --exe sim_main.cpp
TARGET_SIM  = verilator
else
BFLAGS      = $(if $(single_ram), -D SINGLE_RAM=1)
endif

FILELIST    = -f filelist.txt $(if $(single_ram), ../rtl/top.v)

all: $(TARGET)

$(TARGET):
	$(TARGET_SIM) $(BFLAGS) -o $(TARGET) $(FILELIST)
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

