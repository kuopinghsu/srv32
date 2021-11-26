verilator  ?= 1
top        ?= 0
rv32c      ?= 0
debug      ?= 0
coverage   ?= 0
memsize    ?= 128

# Run flags
RFLAGS      = +trace $(if $(debug), +dump)

TARGET      = sim

ifeq (, $(shell which stdbuf))
STDBUF      =
else
STDBUF      = stdbuf -o0 -e0
endif

TARGET_SIM  = iverilog

ifeq ($(top),1)
    _top := 1
endif

ifeq ($(coverage),1)
    _coverage := 1
endif

ifeq ($(rv32c),1)
    _rv32c := 1
endif

ifeq ($(verilator),1)
BFLAGS      = -O3 -cc -Wall -Wno-STMTDLY -Wno-UNUSED \
              +define+MEMSIZE=$(memsize) \
              $(if $(_top), +define+SINGLE_RAM) \
              $(if $(_rv32c), +define+RV32C_ENABLED) \
              $(if $(_coverage), --coverage) \
              --trace-fst --Mdir sim_cc --build --exe sim_main.cpp getch.cpp
TARGET_SIM  = verilator
else
BFLAGS      = $(if $(_top), -D SINGLE_RAM=1) $(if $(_rv32c), -DRV32C_ENABLED=1)
endif

FILELIST    = -f filelist.txt $(if $(_top), ../rtl/top_s.v, ../rtl/top.v)

all: $(TARGET)

$(TARGET):
	$(TARGET_SIM) $(BFLAGS) -o $(TARGET) $(FILELIST)
	@if [ "$(verilator)" = "1" ]; then \
		mv sim_cc/sim .; \
	fi

%.run: $(TARGET) checkcode.awk
	@if [ ! -f ../sw/$*/memory.bin ]; then \
		make -C ../sw $*; \
	fi
	@cp ../sw/$*/*.bin .
	@$(STDBUF) ./$(TARGET) $(RFLAGS) | awk -f $(filter %.awk, $^)
	@if [ -f coverage.dat ]; then \
		mv coverage.dat $*_cov.dat; \
	fi

clean:
	@$(RM) $(TARGET) wave.* trace.log *.bin dump.txt
	@$(RM) -rf sim_cc *_cov.dat

distclean: clean

