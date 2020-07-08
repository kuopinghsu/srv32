#include "systemc.h"
#include "Vriscv.h"
#include "verilated.h"

int sc_main(int argc, char** argv) {
    Verilated::commandArgs(argc,argv);

#ifdef DUMP
    Verilated::traceEverOn(true);
#endif

    sc_time T(10,SC_NS);
    sc_clock clk ("clk", T);
    sc_signal<bool> resetb("resetb");
    sc_signal<bool> stall("stall");

    Vriscv *dut = new Vriscv("testbench");

    dut->clk(clk);
    dut->resetb(resetb);
    dut->stall(stall);

    resetb = 0;
    stall  = 1;
    sc_start(10*T);

    resetb = 1;
    sc_start(10*T);

    stall  = 0;

    while (!Verilated::gotFinish()) {
        sc_start(1, SC_NS);
    }
    delete dut;

    return 0;
}
