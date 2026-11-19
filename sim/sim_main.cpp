#include <signal.h>
#include "Vriscv.h"
#include "verilated.h"

#define HAVE_CHRONO

#ifdef HAVE_CHRONO
#include <chrono>
#include <iostream>
#endif

#define RESOLUTION 4

extern "C"
int elfloader(char *file, char *mem,
              int imem_base, int dmem_base,
              int imem_size, int dmem_size);

vluint64_t main_time = 0;

double sc_time_stamp(void)
{
    return main_time;
}

void elfread(char *filename)
{
    FILE *fp;
    char *mem;
    int memsize = MEMSIZE * 1024;

    if ((mem = (char*)malloc(memsize*2)) == NULL) {
        printf("memory allocate failure\n");
        exit(1);
    }
    if (elfloader(filename, (char*)mem, 0, memsize, memsize, memsize) == 0) {
        printf("Can not read elf file %s\n", filename);
        exit(1);
    }

#ifdef SINGLE_RAM
    if ((fp = fopen("memory.bin", "wb")) == NULL) {
        printf("file memory.bin creates fail\n");
        exit(1);
    }
    if (fwrite(mem, 1, memsize * 2, fp) != static_cast<size_t>(memsize * 2)) {
        printf("file write fail\n");
        exit(1);
    }
    fclose(fp);
#else
    if ((fp = fopen("imem.bin", "wb")) == NULL) {
        printf("file imem.bin creates fail\n");
        exit(1);
    }

    if (memsize != fwrite(mem, sizeof(char), memsize, fp)) {
        printf("file write fail\n");
        exit(1);
    }

    fclose(fp);

    if ((fp = fopen("dmem.bin", "wb")) == NULL) {
        printf("file dmem.bin creates fail\n");
        exit(1);
    }

    if (memsize != fwrite(&mem[memsize], sizeof(char), memsize, fp)) {
        printf("file write fail\n");
        exit(1);
    }

    fclose(fp);
#endif
    free(mem);
}

static void remove_memory_images()
{
#ifdef SINGLE_RAM
    unlink("memory.bin");
#else
    unlink("imem.bin");
    unlink("dmem.bin");
#endif
}

void finish(int dummy)
{
    remove_memory_images();
    puts("\nCtrl-C...\n");
    exit(-1);
}

int main(int argc, char** argv)
{
    int elf_loaded = 0;
    Verilated::commandArgs(argc,argv);
    Verilated::traceEverOn(true);

    #ifdef HAVE_CHRONO
    std::chrono::steady_clock::time_point time_begin;
    #endif

    signal(SIGINT, finish);

    if (argc >= 2 && argv[argc-1][0] != '+' && argv[argc-1][0] != '-') {
        elfread(argv[argc-1]);
        elf_loaded = 1;
    }

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

    if (elf_loaded) {
        remove_memory_images();
    }

    return 0;
}
