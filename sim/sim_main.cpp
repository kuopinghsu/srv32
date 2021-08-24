#include "Vriscv.h"
#include "verilated.h"

#define HAVE_CHRONO

#ifdef HAVE_CHRONO
#include <chrono>
#include <iostream>
#endif

#define RESOLUTION 4

vluint64_t main_time = 0;

double sc_time_stamp(void)
{
    return main_time;
}

int main(int argc, char** argv)
{
    Verilated::commandArgs(argc,argv);
    Verilated::traceEverOn(true);

    #ifdef HAVE_CHRONO
    std::chrono::steady_clock::time_point time_begin;
    #endif

    Vriscv *top = new Vriscv;

    top->stall  = 1;
    top->resetb = 0;
    top->clk    = 0;

    #ifdef HAVE_CHRONO
    time_begin = std::chrono::steady_clock::now();
    #endif

    while (!Verilated::gotFinish()) {
        if (main_time > RESOLUTION) {
            top->resetb = 1;
        }
        if (main_time > 30) {
            top->stall = 0;
        }
        if ((main_time % RESOLUTION) == 1) {
            top->clk = 1;
        }
        if ((main_time % RESOLUTION) == (RESOLUTION / 2 + 1)) {
            top->clk = 0;
        }
        top->eval();
        main_time++;
    }

    #ifdef HAVE_CHRONO
    {
          std::chrono::steady_clock::time_point time_end;
          float sec;
          int cycles = main_time / RESOLUTION;
          time_end = std::chrono::steady_clock::now();
          sec = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() / 1000.0;
          float speed_mhz = cycles / sec / 1000000.0;
          std::cout << std::endl;
          std::cout << "Simulation statistics" << std::endl;
          std::cout << "=====================" << std::endl;
          std::cout << "Simulation time  : " << sec << " s" << std::endl;
          std::cout << "Simulation cycles: " << cycles << std::endl;
          std::cout << "Simulation speed : " << speed_mhz << " MHz" << std::endl << std::endl;
    }
    #endif

    #if VM_COVERAGE
    VerilatedCov::write("coverage.dat");
    #endif

    top->final();
    delete top;

    return 0;
}
