#include <stdio.h>
#include <stdlib.h>
#include "rvconfig.h"

volatile int result[3] = {0, 0, 0};

void user_trap_handler(void) {
    int mcause = CSRR_MCAUSE();

    if (mcause & 0x80000000) {
        // machine software interrupt
        if ((mcause & 0x7fffffff) == 3) {
            CSRW_MIE(CSRR_MIE() & ~(1<<3));
            *(volatile int*)(MSIP_BASE) = *(volatile int*)(MSIP_BASE) & 0xfffffffe;
            result[0] = 1;
            return;
        }
        // machine timer interrupt
        if ((mcause & 0x7fffffff) == 7) {
            CSRW_MIE(CSRR_MIE() & ~(1<<7));
            // This doesn't make any sense, just toggle the MTIME and MTIMECMP registers
            // to improve code coverage.
            *(volatile int*)(MTIME_BASE)   = *(volatile int*)(MTIMECMP_BASE)+1000;
            *(volatile int*)(MTIME_BASE+4) = *(volatile int*)(MTIMECMP_BASE+4);
            result[1] = 1;
            return;
        }
        // machine external interrupt
        if ((mcause & 0x7fffffff) == 11) {
            CSRW_MIE(CSRR_MIE() & ~(1<<11));
            *(volatile int*)(MSIP_BASE) = *(volatile int*)(MSIP_BASE) & 0xfffefff0;
            result[2] = 1;
            return;
        }
    } else {
        CSRW_MEPC(CSRR_MEPC()+4);
    }
}

int main(void) {
    int i;
    *(volatile int*)(MTIMECMP_BASE)   = 32768+300;
    *(volatile int*)(MTIMECMP_BASE+4) = 0;
    *(volatile int*)(MTIME_BASE)      = 32768;
    *(volatile int*)(MTIME_BASE+4)    = 0;

    CSRW_MIE(1<<11|1<<7|1<<3);
    CSRW_MSTATUS(CSRR_MSTATUS() | 1<<3);

    for(i=0; i<20; i++) __asm volatile("nop");

    *(volatile int*)(MSIP_BASE)       = *(volatile int*)(MSIP_BASE) | 1<<16;

    for(i=0; i<20; i++) __asm volatile("nop");

    *(volatile int*)(MSIP_BASE)       = *(volatile int*)(MSIP_BASE) | 1<<0;

    while((result[0]+result[1]+result[2]) != 3);

    // test readonly CSR
    CSRW_RDCYCLE(CSRR_RDCYCLE()+1);
    CSRW_RDCYCLEH(CSRR_RDCYCLEH()+1);
    CSRW_RDINSTRET(CSRR_RDINSTRET()+1);
    CSRW_RDINSTRETH(CSRR_RDINSTRETH()+1);
    CSRW_MVENDORID(CSRR_MVENDORID()+1);
    CSRW_MARCHID(CSRR_MARCHID()+1);
    CSRW_MIMPID(CSRR_MIMPID()+1);
    CSRW_MHARTID(CSRR_MHARTID()+1);

    // test r/w CSR
    CSRW_MISA(CSRR_MISA()+1);
    CSRW_MCAUSE(CSRR_MCAUSE()+1);
    CSRW_MTVAL(CSRR_MTVAL()+1);
    CSRW_MIP(CSRR_MIP()|(1<<31));

    // test unimplemented CSR
    CSRW_DPC(CSRR_DPC()+1);

    // Illegal instruction
    asm volatile(".byte 0x00, 0x00, 0x00, 0x00");
    asm volatile(".byte 0x03, 0x30, 0x5a, 0x5a");
    asm volatile(".byte 0x03, 0x70, 0x5a, 0x5a");
    asm volatile(".byte 0x23, 0x30, 0x5a, 0x5a");
    asm volatile(".byte 0x63, 0x20, 0x5a, 0x5a");
    asm volatile(".byte 0x73, 0x00, 0x30, 0x5a");

    asm volatile("fence");
}

