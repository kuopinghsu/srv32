verilator  ?= 1
top        ?= $(if $(filter 1,$(verilator)),axi,0)
rv32m      ?= 1
rv32e      ?= 0
rv32b      ?= 0
rv32c      ?= 0
debug      ?= 0
coverage   ?= 0
memsize    ?= 256

# Run flags
RFLAGS      = +trace $(if $(debug), +dump)

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
BFLAGS      = -O3 -cc --top-module testbench --prefix Vriscv -Wall -Wno-STMTDLY -Wno-UNUSED \
              +define+MEMSIZE=$(memsize) \
              $(if $(_top), +define+SINGLE_RAM) \
              $(if $(_rv32m), +define+RV32M_ENABLED) \
              $(if $(_rv32e), +define+RV32E_ENABLED) \
              $(if $(_rv32b), +define+RV32B_ENABLED) \
              $(if $(_rv32c), +define+RV32C_ENABLED) \
              $(if $(_coverage), --coverage) \
              --trace-fst --Mdir $(SIM_DIR) --build --exe sim_main.cpp getch.cpp elfloader.c
TARGET_SIM  = verilator
else
BFLAGS      = -s testbench $(if $(_top), -D SINGLE_RAM=1) \
              $(if $(_rv32m), -DRV32M_ENABLED=1) \
              $(if $(_rv32e), -DRV32E_ENABLED=1) \
              $(if $(_rv32b), -DRV32B_ENABLED=1) \
              $(if $(_rv32c), -DRV32C_ENABLED=1)
endif

FILELIST    = -f filelist.txt ../rtl/top.v

AXI_FILELIST = ../rtl/axi_lite.f
AXI_SOURCES  = $(addprefix ../rtl/,$(filter %.v,$(shell cat $(AXI_FILELIST)))) ../rtl/opcode.vh

ifeq ($(top),axi)
all: sim_axi
else
all: $(TARGET)
endif

sim_axi: $(AXI_FILELIST) $(AXI_SOURCES) ../testbench/axi_lite_tb.v
	verilator --binary --timing --timescale-override 1ns/1ps \
	    --Mdir sim_cc_axi --top-module axi_lite_tb -o $@ \
	    -F $(AXI_FILELIST) ../testbench/axi_lite_tb.v
	mv sim_cc_axi/$@ .

.PHONY: check
check: sim_axi
	$(MAKE) -C ../sw/axi_lite
	./sim_axi
	./sim_axi +redirect
	./sim_axi +store_irq
	./sim_axi +image=../sw/axi_lite/axi_lite.hex

$(TARGET): filelist.txt ../rtl/top.v ../rtl/riscv.v ../rtl/clint.v ../testbench/testbench.v ../testbench/memmodel.v sim_main.cpp
	CXXFLAGS="-DMEMSIZE=$(memsize) $(if $(_top),-DSINGLE_RAM)" $(TARGET_SIM) $(BFLAGS) -o $(TARGET) $(FILELIST)
	@if [ "$(verilator)" = "1" ]; then \
		mv $(SIM_DIR)/$(TARGET) .; \
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
	@$(RM) sim sim_single sim_axi wave.* trace.log dump.txt
	@$(RM) -rf sim_cc sim_cc_single sim_cc_axi *_cov.dat

distclean: clean
