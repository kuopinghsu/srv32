# use verilator
VERILATOR  ?= 0
SINGLE_RAM ?= 0

# Run flags
RFLAGS      = +trace

TARGET      = sim

TARGET_SIM  = iverilog

ifeq ($(SINGLE_RAM),1)
    single_ram := 1
endif

ifeq ($(VERILATOR),1)
BFLAGS      = -O3 -cc -Wall -Wno-STMTDLY -Wno-UNUSED \
              $(if $(single_ram), +define+SINGLE_RAM) \
              --trace-fst --Mdir sim_cc --build --exe sim_main.cpp
TARGET_SIM  = verilator
else
BFLAGS      = $(if $(single_ram), -D SINGLE_RAM=1)
endif

FILELIST    = -f filelist.txt $(if $(single_ram), ../rtl/top.v)

all: $(TARGET)

$(TARGET):
	$(TARGET_SIM) $(BFLAGS) -o $(TARGET) $(FILELIST)
	@if [ "$(VERILATOR)" = "1" ]; then \
		mv sim_cc/sim .; \
	fi

%.run: $(TARGET) checkcode.awk
	@if [ ! -f ../sw/$*/memory.bin ]; then \
		make -C ../sw $*; \
	fi
	cp ../sw/$*/*.bin .
	./$(TARGET) $(RFLAGS) | awk -f $(filter %.awk, $^)

clean:
	@$(RM) $(TARGET) wave.* trace.log *.bin dump.txt
	@$(RM) -rf sim_cc

distclean: clean

