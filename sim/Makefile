verilator  ?= 1
top        ?= $(if $(filter 1,$(verilator)),axi,0)
rv32m      ?= 1
rv32e      ?= 0
rv32b      ?= 0
rv32c      ?= 0
debug      ?= 0
coverage   ?= 0
memsize    ?= 256
branch_predictor ?= 1
uart_clocks_per_bit ?= 16

# Run flags
RFLAGS      = +trace $(if $(filter 1,$(debug)),+dump)

TARGET      = $(if $(_top),sim_single,sim)
SIM_DIR     = $(if $(_top),sim_cc_single,sim_cc)

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

ifeq ($(rv32m),1)
    _rv32m := 1
endif

ifeq ($(rv32e),1)
    _rv32e := 1
endif

ifeq ($(rv32b),1)
    _rv32b := 1
endif

ifeq ($(rv32c),1)
    _rv32c := 1
endif

ifeq ($(verilator),1)
ELFLOADER_OBJ = $(SIM_DIR)/elfloader_c.o
BFLAGS      = -O3 -cc --top-module testbench --prefix Vriscv -Wall -Wno-STMTDLY -Wno-UNUSED \
              $(CORE_PARAMS) -GMEMORY_MODE=$(if $(_top),1,0) \
              $(if $(_coverage), --coverage) \
              --trace-fst --Mdir $(SIM_DIR) --build --exe sim_main.cpp getch.cpp $(abspath $(ELFLOADER_OBJ))
TARGET_SIM  = verilator
else
BFLAGS      = -s testbench -Ptestbench.BRANCH_PREDICTOR=$(branch_predictor) -Ptestbench.MEMORY_MODE=$(if $(_top),1,0) $(if $(_top), -D SINGLE_RAM=1) \
              $(if $(_rv32m), -DRV32M_ENABLED=1) \
              $(if $(_rv32e), -DRV32E_ENABLED=1) \
              $(if $(_rv32b), -DRV32B_ENABLED=1) \
              $(if $(_rv32c), -DRV32C_ENABLED=1)
endif

AXI_SCENARIOS = ../testbench/axi_lite_scenarios.vh

FILELIST    = -f filelist.txt ../rtl/top.v

AXI_FILELIST = ../rtl/axi_lite.f
AXI_SOURCES  = $(addprefix ../rtl/,$(filter %.v,$(shell cat $(AXI_FILELIST)))) ../rtl/opcode.vh

ifeq ($(top),axi)
all: sim_axi
else
all: $(TARGET)
endif

CORE_PARAMS = -GRV32M=$(rv32m) -GRV32E=$(rv32e) -GRV32B=$(rv32b) -GRV32C=$(rv32c) \
              -GBRANCH_PREDICTOR=$(branch_predictor) +define+MEMSIZE=$(memsize)
AXI_PARAMS = $(CORE_PARAMS) -GUART_CLOCKS_PER_BIT=$(uart_clocks_per_bit)

sim_axi: .sim_axi.config Makefile $(AXI_FILELIST) $(AXI_SOURCES) $(AXI_SCENARIOS) ../testbench/testbench.v ../testbench/memmodel.v getch.cpp
	verilator --binary --timing --timescale-override 1ns/1ps \
	    --Mdir sim_cc_axi --top-module testbench -GMEMORY_MODE=2 +define+AXI_LITE_REGRESSION \
	    $(AXI_PARAMS) -o $@ \
	    -I../testbench -F $(AXI_FILELIST) ../testbench/testbench.v ../testbench/memmodel.v getch.cpp
	cp sim_cc_axi/$@ .

.PHONY: check check-branch
check: sim_axi check-branch
	$(MAKE) -C ../sw/axi_lite
	./sim_axi
	./sim_axi +redirect
	./sim_axi +store_irq
	./sim_axi +image=../sw/axi_lite/axi_lite.hex

# Preserve the stamp timestamp when options are unchanged, but rebuild the
# simulator whenever a command-line option changes its compiled configuration.
.PHONY: FORCE_SIM_CONFIG
FORCE_SIM_CONFIG:

.sim_axi.config: FORCE_SIM_CONFIG
	@printf '%s\n' '$(AXI_PARAMS)' > $@.tmp
	@cmp -s $@.tmp $@ && rm -f $@.tmp || mv -f $@.tmp $@

.$(TARGET).config: FORCE_SIM_CONFIG
	@printf '%s\n' '$(TARGET_SIM) $(BFLAGS)' '$(CC) $(CPPFLAGS) $(CFLAGS)' 'memsize=$(memsize) single=$(_top)' > $@.tmp
	@cmp -s $@.tmp $@ && rm -f $@.tmp || mv -f $@.tmp $@

ifeq ($(verilator),1)
# Verilator compiles --exe sources with CXX, so build the C loader separately.
$(ELFLOADER_OBJ): elfloader.c ../tools/elf.h Makefile .$(TARGET).config
	@mkdir -p $(SIM_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -O3 -c -o $@ $<
endif

$(TARGET): .$(TARGET).config Makefile filelist.txt ../rtl/top.v ../rtl/riscv.v ../rtl/clint.v ../testbench/testbench.v ../testbench/memmodel.v sim_main.cpp getch.cpp $(ELFLOADER_OBJ)
	CXXFLAGS="-DMEMSIZE=$(memsize) $(if $(_top),-DSINGLE_RAM)" $(TARGET_SIM) $(BFLAGS) -o $(TARGET) $(FILELIST)
	@if [ "$(verilator)" = "1" ]; then \
		cp $(SIM_DIR)/$(TARGET) .; \
	fi

%.elf: $(TARGET) checkcode.awk
	@if [ ! -f ../sw/$*/$*.elf ]; then \
		make -C ../sw $*; \
	fi
	@$(STDBUF) ./$(TARGET) $(RFLAGS) ../sw/$*/$*.elf | awk -f $(filter %.awk, $^)
	@if [ -f coverage.dat ]; then \
		mv coverage.dat $*_cov.dat; \
	fi

clean:
	@$(RM) sim sim_single sim_axi wave.* trace.log dump.txt .sim.config .sim_single.config .sim_axi.config .sim.config.tmp .sim_single.config.tmp .sim_axi.config.tmp
	@$(RM) -rf sim_cc sim_cc_single sim_cc_axi *_cov.dat

distclean: clean

# Use the merged ELF testbench and compare RTL/ISS traces in all four modes.
check-branch:
	@set -e; for ram_mode in 0 1; do \
	    for predict in 0 1; do \
	        $(MAKE) -j1 -C .. top=$$ram_mode branch_predictor=$$predict \
	            memsize=$(memsize) rv32c=$(rv32c) rv32e=$(rv32e) rv32b=$(rv32b) \
	            coremark irq exception csr; \
	    done; \
	done
