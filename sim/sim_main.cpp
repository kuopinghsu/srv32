#include "Vriscv.h"
#include "verilated.h"
int main(int argc, char** argv, char** env) {
    Verilated::commandArgs(argc, argv);
    Vriscv* top = new Vriscv;
    while (!Verilated::gotFinish()) { top->eval(); }
    delete top;
    exit(0);
}
