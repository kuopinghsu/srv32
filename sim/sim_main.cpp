#include "Vriscv.h"
#include "verilated.h"

vluint64_t main_time = 0;

double sc_time_stamp(void)
{
    return main_time;
}

int main(int argc, char** argv)
{
    Verilated::commandArgs(argc,argv);
    Verilated::traceEverOn(true);

    Vriscv *top = new Vriscv;

    top->stall  = 1;
    top->resetb = 0;
    top->clk    = 0;

    while (!Verilated::gotFinish()) {
        if (main_time > 10) {
            top->resetb = 1;
        }
        if (main_time > 30) {
            top->stall = 0;
        }
        if ((main_time % 10) == 1) {
            top->clk = 1;
        }
        if ((main_time % 10) == 6) {
            top->clk = 0;
        }
        top->eval();
        main_time++;
    }

    #if VM_COVERAGE
    VerilatedCov::write("coverage.dat");
    #endif

    top->final();
    delete top;

    return 0;
}
