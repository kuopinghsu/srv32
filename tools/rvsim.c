// Copyright © 2020 Kuoping Hsu
// rvsim.c: Instruction Set Simulator for RISC-V RV32I instruction sets
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "opcode.h"

int mem_size = 128*1024; // default memory size

#define PRINT_TIMELOG 1
#define MAXLEN      1024

#define TIME_LOG    if (ft && PRINT_TIMELOG) fprintf(ft, "%10d ", csr.cycle.d.lo)
#define TRACE_LOG   if (ft) fprintf(ft,
#define TRACE_END   )

#define TRAP(cause,val) { \
    CYCLE_ADD(branch_penalty); \
    csr.mcause = cause; \
    csr.mstatus = (csr.mstatus &  (1<<MIE)) ? (csr.mstatus | (1<<MPIE)) : (csr.mstatus & ~(1<<MPIE)); \
    csr.mstatus = (csr.mstatus & ~(1<<MIE)); \
    csr.mepc = prev_pc; \
    csr.mtval = (val); \
    pc = (csr.mtvec & 1) ? (csr.mtvec & 0xfffffffe) + cause * 4 : csr.mtvec; \
}

#define INT(cause,src) { \
    /* When the branch instruction is interrupted, do not accumulate cycles, */ \
    /* which has been added when the branch instruction is executed. */ \
    if (pc == (compressed ? prev_pc+2 : prev_pc+4)) CYCLE_ADD(branch_penalty); \
    csr.mcause = cause; \
    csr.mstatus = (csr.mstatus &  (1<<MIE)) ? (csr.mstatus | (1<<MPIE)) : (csr.mstatus & ~(1<<MPIE)); \
    csr.mstatus = (csr.mstatus & ~(1<<MIE)); \
    csr.mip = csr.mip | (1 << src); \
    csr.mepc = pc; \
    pc = (csr.mtvec & 1) ? (csr.mtvec & 0xfffffffe) + (cause & (~(1<<31))) * 4 : csr.mtvec; \
}

#define CYCLE_ADD(count) { \
    csr.cycle.c = csr.cycle.c + count; \
    if (!mtime_update) csr.mtime.c = csr.mtime.c + count; \
}

typedef struct _CSR {
    COUNTER time;
    COUNTER cycle;
    COUNTER instret;
    COUNTER mtime;
    COUNTER mtimecmp;
    int32_t mvendorid;
    int32_t marchid;
    int32_t mimpid;
    int32_t mhartid;
    int32_t mscratch;
    int32_t mstatus;
    int32_t misa;
    int32_t mie;
    int32_t mtvec;
    int32_t mepc;
    int32_t mcause;
    int32_t mip;
    int32_t mtval;
    int32_t msip;
#ifdef XV6_SUPPORT
    int32_t medeleg;
    int32_t mideleg;
    int32_t mcounteren;
    int32_t sstatus;
    int32_t sie;
    int32_t stvec;
    int32_t sscratch;
    int32_t sepc;
    int32_t scause;
    int32_t stval;
    int32_t sip;
    int32_t satp;
#endif // XV6_SUPPORT
} CSR;

CSR csr;
int32_t pc = 0;

int mode = MMODE;
int mem_base = 0;
int singleram = 0;
int branch_penalty = BRANCH_PENALTY;
int mtime_update = 0;
struct timeval time_start;
struct timeval time_end;

int *mem;
int *imem;
int *dmem;

#ifdef RV32C_ENABLED
int overhead = 0;
#endif // RV32C_ENABLED

int quiet = 0;

char *regname[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0(fp)", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

int srv32_syscall(int func, int a0, int a1, int a2, int a3, int a4, int a5);
int elfloader(char *file, char *mem, int imem_base, int dmem_base, int imem_size, int dmem_size);
int getch(void);

void usage(void) {
    printf(
"Instruction Set Simulator for RV32IM, (c) 2020 Kuoping Hsu\n"
"Usage: rvsim [-h] [-b n] [-m n] [-n n] [-p] [-l logfile] file\n\n"
"       --help, -h              help\n"
"       --membase n, -m n       memory base\n"
"       --memsize n, -n n       memory size (in Kb)\n"
"       --branch n, -b n        branch penalty (default 2)\n"
"       --single, -s            single RAM\n"
"       --predict, -p           static branch prediction\n"
"       --log file, -l file     generate log file\n"
"\n"
"       file                    the elf executable file\n"
"\n"
    );
}

#ifdef MACOX
void *aligned_malloc(int align, size_t size )
{
    void *mem = malloc( size + (align-1) + sizeof(void*) );

    char *amem = ((char*)mem) + sizeof(void*);
    amem += (align - ((uintptr_t)amem & (align - 1)) & (align-1));

    ((void**)amem)[-1] = mem;
    return amem;
}

void aligned_free( void *mem )
{
    free( ((void**)mem)[-1] );
}
#else
#include <malloc.h>
#define aligned_malloc memalign
#define aligned_free free
#endif // MACOX

#ifndef __STDC_WANT_LIB_EXT1__ 
char *strncpy_s(char *dest, size_t n, const char *src, size_t count) {
    int len = (int)strnlen(src, count);
    if (len > n) len = n;
    memcpy(dest, src, len);
    dest[len] = 0;
    return dest;
}
#endif // __STDC_WANT_LIB_EXT1__

static inline int to_imm_i(uint32_t n) {
    return (int)((n & (1<<11)) ? (n | 0xfffff000) : n);
}

/*
static inline int to_imm_iu(uint32_t n) {
    return (int)(n);
}
*/

static inline int to_imm_s(uint32_t n1, uint32_t n2) {
    uint32_t n = (n1 << 5) + n2;
    return (int)((n & (1<<11)) ? (n | 0xfffff000) : n);
}

static inline int to_imm_b(uint32_t n1, uint32_t n2) {
    uint32_t m;
    union {
        uint32_t n;
        struct {
            uint32_t a0 : 1;
            uint32_t a1 : 4;
            uint32_t a2 : 6;
            uint32_t a3 : 1;
            //uint32_t a4 : 20; // no used
        } m;
    } r;
    r.n = (n1 << 5) + n2;
    m = (r.m.a3 << 12) | (r.m.a2 << 5) | (r.m.a1 << 1) | (r.m.a0 << 11);
    return (int)((m & (1<<12)) ? (m | 0xffffe000) : m);
}

static inline int to_imm_j(uint32_t n) {
    uint32_t m;
    union {
        uint32_t n;
        struct {
            uint32_t a0 : 8;
            uint32_t a1 : 1;
            uint32_t a2 : 10;
            uint32_t a3 : 1;
            // uint32_t a4 : 12; // no used
        } m;
    } r;
    r.n = n;
    m = (r.m.a3 << 20) | (r.m.a2 << 1) | (r.m.a1 << 11) | (r.m.a0 << 12);
    return (int)((m & (1<<20)) ? (m | 0xffe00000) : m);
}

static inline int to_imm_u(uint32_t n) {
    return (int)(n << 12);
}

void prog_exit(int exitcode) {
    double diff;
    gettimeofday(&time_end, NULL);

    diff = (double)(time_end.tv_sec-time_start.tv_sec) + (time_end.tv_usec-time_start.tv_usec)/1000000.0;

    if (!quiet) {
#ifdef RV32C_ENABLED
        printf("\nExcuting %lld instructions, %lld cycles, %1.3f CPI, %1.3f%% overhead\n", csr.instret.c,
               csr.cycle.c, ((float)csr.cycle.c)/csr.instret.c, (overhead*100.0)/csr.instret.c);
#else
        printf("\nExcuting %lld instructions, %lld cycles, %1.3f CPI\n", csr.instret.c,
               csr.cycle.c, ((float)csr.cycle.c)/csr.instret.c);
#endif // RV32C_ENABLED

        printf("Program terminate\n");

        printf("\n");
        printf("Simulation statistics\n");
        printf("=====================\n");
        printf("Simulation time  : %0.3f s\n", (float)diff);
        printf("Simulation cycles: %lld\n", csr.cycle.c);
        printf("Simulation speed : %0.3f MHz\n", (float)(csr.cycle.c / diff / 1000000.0));
        printf("\n");
    }
    exit(exitcode);
}

#define UPDATE_CSR(update,mode,reg,val) { \
    if (update) { \
        if ((mode) == OP_CSRRW) reg = (val); \
        if ((mode) == OP_CSRRS) reg = (reg) |  (val); \
        if ((mode) == OP_CSRRC) reg = (reg) & ~(val); \
    } \
}

int csr_rw(int regs, int mode, int val, int update, int *legal) {
    COUNTER counter;
    int result = 0;
    *legal = 1;
    switch(regs) {
        case CSR_RDCYCLE    : counter.c = csr.cycle.c - 1;
                              result = counter.d.lo; // UPDATE_CSR(update, mode, csr.cycle.d.lo, val);
                              break;
        case CSR_RDCYCLEH   : counter.c = csr.cycle.c - 1;
                              result = counter.d.hi; // UPDATE_CSR(update, mode, csr.cycle.d.hi, val);
                              break;
        /*
        case CSR_RDTIME     : counter.c = csr.time.c - 1;
                              result = counter.d.lo; // UPDATE_CSR(update, mode, csr.time.d.lo, val);
                              break;
        case CSR_RDTIMEH    : counter.c = csr.time.c - 1;
                              result = counter.d.hi; // UPDATE_CSR(update, mode, csr.time.d.hi, val);
                              break;
        */
        case CSR_RDINSTRET  : counter.c = csr.instret.c - 1;
                              result = counter.d.lo; // UPDATE_CSR(update, mode, csr.instret.d.lo, val);
                              break;
        case CSR_RDINSTRETH : counter.c = csr.instret.c - 1;
                              result = counter.d.hi; // UPDATE_CSR(update, mode, csr.instret.d.hi, val);
                              break;
        case CSR_MVENDORID  : result = csr.mvendorid; // UPDATE_CSR(update, mode, csr.mvendorid, val);
                              break;
        case CSR_MARCHID    : result = csr.marchid; // UPDATE_CSR(update, mode, csr.marchid, val);
                              break;
        case CSR_MIMPID     : result = csr.mimpid; // UPDATE_CSR(update, mode, csr.mimpid, val);
                              break;
        case CSR_MHARTID    : result = csr.mhartid; // UPDATE_CSR(update, mode, csr.mhartid, val);
                              break;
        case CSR_MSCRATCH   : result = csr.mscratch; UPDATE_CSR(update, mode, csr.mscratch, val);
                              break;
        case CSR_MSTATUS    : result = csr.mstatus; UPDATE_CSR(update, mode, csr.mstatus, val);
                              break;
        case CSR_MISA       : result = csr.misa; UPDATE_CSR(update, mode, csr.misa, val);
                              break;
        case CSR_MIE        : result = csr.mie; UPDATE_CSR(update, mode, csr.mie, val);
                              break;
        case CSR_MIP        : result = csr.mip; UPDATE_CSR(update, mode, csr.mip, val);
                              break;
        case CSR_MTVEC      : result = csr.mtvec; UPDATE_CSR(update, mode, csr.mtvec, val);
                              break;
        case CSR_MEPC       : result = csr.mepc; UPDATE_CSR(update, mode, csr.mepc, val);
                              break;
        case CSR_MCAUSE     : result = csr.mcause; UPDATE_CSR(update, mode, csr.mcause, val);
                              break;
        case CSR_MTVAL      : result = csr.mtval; UPDATE_CSR(update, mode, csr.mtval, val);
                              break;
#ifdef XV6_SUPPORT
        case CSR_MEDELEG    : result = csr.medeleg; UPDATE_CSR(update, mode, csr.medeleg, val);
                              break;
        case CSR_MIDELEG    : result = csr.mideleg; UPDATE_CSR(update, mode, csr.mideleg, val);
                              break;
        case CSR_MCOUNTEREN : result = csr.mcounteren; UPDATE_CSR(update, mode, csr.mcounteren, val);
                              break;
        case CSR_SSTATUS    : result = csr.sstatus; UPDATE_CSR(update, mode, csr.sstatus, val);
                              break;
        case CSR_SIE        : result = csr.sie; UPDATE_CSR(update, mode, csr.sie, val);
                              break;
        case CSR_STVEC      : result = csr.stvec; UPDATE_CSR(update, mode, csr.stvec, val);
                              break;
        case CSR_SSCRATCH   : result = csr.sscratch; UPDATE_CSR(update, mode, csr.sscratch, val);
                              break;
        case CSR_SEPC       : result = csr.sepc; UPDATE_CSR(update, mode, csr.sepc, val);
                              break;
        case CSR_SCAUSE     : result = csr.scause; UPDATE_CSR(update, mode, csr.scause, val);
                              break;
        case CSR_STVAL      : result = csr.stval; UPDATE_CSR(update, mode, csr.stval, val);
                              break;
        case CSR_SIP        : result = csr.sip; UPDATE_CSR(update, mode, csr.sip, val);
                              break;
        case CSR_SATP       : result = csr.satp; UPDATE_CSR(update, mode, csr.satp, val);
                              break;
#endif // XV6_SUPPORT
        default: result = 0; // FIXME
                 printf("Unsupport CSR register 0x%03x at PC 0x%08x\n", regs, pc);
                 *legal = 0;
    }
    return result;
}

#ifdef RV32E_ENABLED
static int32_t regs[16];
#else
static int32_t regs[32];
#endif // RV32E_ENABLED

#ifdef RV32E_ENABLED
static inline int32_t REGS(int n) {
    if (n > 15) {
        printf("RV32E: can not access registers %d\n", n);
        return 0;
    } else {
        return regs[n];
    }
}
static inline void REGS_W(int n, int32_t v) {
    if (n > 15) {
        printf("RV32E: can not access registers %d\n", n);
    } else {
        regs[n] = v;
    }
}
#else
#  define REGS(n)      regs[n]
#  define REGS_W(n, v) regs[n] = (v)
#endif // RV32E_ENABLED

int main(int argc, char **argv) {
    FILE *ft = NULL;

    int i;
    int result;
    char *file = NULL;
    char *tfile = NULL;
    int branch_predict = 0;
    int prev_pc;
    int timer_irq;
    int sw_irq;
    int sw_irq_next;
    int ext_irq;
    int ext_irq_next;
    int compressed = 0;
#ifdef RV32C_ENABLED
    int compressed_prev = 0;
#endif // RV32C_ENABLED

    const char *optstring = "hb:pl:qm:n:s";
    int c;
    struct option opts[] = {
        {"help", 0, NULL, 'h'},
        {"branch", 1, NULL, 'b'},
        {"predict", 0, NULL, 'p'},
        {"log", 1, NULL, 'l'},
        {"quiet", 0, NULL, 'q'},
        {"membase", 1, NULL, 'm'},
        {"memsize", 1, NULL, 'n'},
        {"single", 0, NULL, 's'}
    };

    while((c = getopt_long(argc, argv, optstring, opts, NULL)) != -1) {
        switch(c) {
            case 'h':
                usage();
                return 1;
            case 'b':
                branch_penalty = atoi(optarg);
                break;
            case 'p':
                branch_predict = 1;
                break;
            case 'l':
                if ((tfile = malloc(MAXLEN)) == NULL) {
                    // LCOV_EXCL_START
                    printf("malloc fail\n");
                    exit(1);
                    // LCOV_EXCL_STOP
                }
                strncpy_s(tfile, MAXLEN-1, optarg, MAXLEN-1);
                break;
            case 'q':
                quiet = 1;
                break;
            case 'm':
                sscanf(optarg, "%i", (uint32_t*)&mem_base);
                break;
            case 'n':
                sscanf(optarg, "%i", (uint32_t*)&mem_size);
                mem_size *= 1024;
                break;
            case 's':
                singleram = 1;
                break;
            default:
                usage();
                return 1;
        }
    }

    if (optind < argc) {
        if ((file = malloc(MAXLEN)) == NULL) {
            // LCOV_EXCL_START
            printf("malloc fail\n");
            exit(1);
            // LCOV_EXCL_STOP
        }
        strncpy_s(file, MAXLEN-1, argv[optind], MAXLEN-1);
    } else {
        usage();
        printf("Error: missing input file.\n\n");
        return 1;
    }

    if (!file) {
        usage();
        return 1;
    }

    if (tfile) {
        if ((ft=fopen(tfile, "w")) == NULL) {
            // LCOV_EXCL_START
            printf("can not open file %s\n", tfile);
            exit(1);
            // LCOV_EXCL_STOP
        }
    }

    if ((mem = (int*)aligned_malloc(sizeof(int), IMEM_SIZE+DMEM_SIZE)) == NULL) {
        // LCOV_EXCL_START
        printf("malloc fail\n");
        exit(1);
        // LCOV_EXCL_STOP
    }

    imem = (int*)&mem[0];
    dmem = (int*)&mem[IMEM_SIZE/sizeof(int)];

    // clear the data memory
    memset(dmem, 0, DMEM_SIZE);

    // load elf file
    if ((result = elfloader(file, (char*)mem, IMEM_BASE, DMEM_BASE, IMEM_SIZE, DMEM_SIZE)) == 0) {
        // LCOV_EXCL_START
        printf("Can not read elf file %s\n", file);
        exit(1);
        // LCOV_EXCL_STOP
    }

    // Registers initialize
    for(i=0; i<sizeof(regs)/sizeof(int); i++) {
        REGS_W(i, 0);
    }

    csr.mvendorid  = MVENDORID;
    csr.marchid    = MARCHID;
    csr.mimpid     = MIMPID;
    csr.mhartid    = MHARTID;
    csr.time.c     = 0;
    csr.cycle.c    = 0;
    csr.instret.c  = 0;
    csr.mtime.c    = 0;
    csr.mtimecmp.c = 0;
    pc             = mem_base;
    prev_pc        = pc;
    timer_irq      = 0;
    sw_irq         = 0;
    sw_irq_next    = 0;
    ext_irq        = 0;
    ext_irq_next   = 0;
    mode           = MMODE;

    gettimeofday(&time_start, NULL);

    // Execution loop
    while(1) {
        INST inst;

#ifdef RV32C_ENABLED
        INSTC instc;
        int illegal;

        illegal = 0;
#endif // RV32C_ENABLED

        mtime_update = 0;

        // keep x0 always zero
        REGS_W(0, 0);

        if (timer_irq && (csr.mstatus & (1 << MIE))) {
            INT(INT_MTIME, MTIP);
        }

        // software interrupt
        if (sw_irq_next && (csr.mstatus & (1 << MIE))) {
            INT(INT_MSI, MSIP);
        }

        // external interrupt
        if (ext_irq_next && (csr.mstatus & (1 << MIE))) {
            INT(INT_MEI, MEIP);
        }

        if (IVA2PA(pc) >= IMEM_SIZE || IVA2PA(pc) < 0) {
            printf("PC 0x%08x out of range 0x%08x\n", pc, IPA2VA(IMEM_SIZE));
            TRAP(TRAP_INST_FAIL, pc);
        }

#ifdef RV32C_ENABLED
        if ((pc&1) != 0) {
            printf("PC 0x%08x alignment error\n", pc);
            TRAP(TRAP_INST_ALIGN, pc);
        }
#else
        if ((pc&3) != 0) {
            printf("PC 0x%08x alignment error\n", pc);
            TRAP(TRAP_INST_ALIGN, pc);
        }
#endif // RV32C_ENABLED

        inst.inst = (IVA2PA(pc) & 2) ?
                     (imem[IVA2PA(pc)/4+1] << 16) | ((imem[IVA2PA(pc)/4] >> 16) & 0xffff) :
                     imem[IVA2PA(pc)/4];

#ifdef RV32C_ENABLED
        instc.inst = (IVA2PA(pc) & 2) ?
                     (short)(imem[IVA2PA(pc)/4] >> 16) :
                     (short)imem[IVA2PA(pc)/4];
#endif // RV32C_ENABLED

        if ((csr.mtime.c >= csr.mtimecmp.c) &&
            (csr.mstatus & (1 << MIE)) && (csr.mie & (1 << MTIE)) &&
            (inst.r.op != OP_SYSTEM)) { // do not interrupt when system call and CSR R/W
            timer_irq = 1;
        } else {
            timer_irq = 0;
        }

        if (sw_irq &&
            (csr.mstatus & (1 << MIE)) && (csr.mie & (1 << MSIE)) &&
            (inst.r.op != OP_SYSTEM)) { // do not interrupt when system call and CSR R/W
            sw_irq_next = 1;
        } else {
            sw_irq_next = 0;
        }
        sw_irq = (csr.msip & (1<<0)) ? 1 : 0;

        if (ext_irq &&
            (csr.mstatus & (1 << MIE)) && (csr.mie & (1 << MEIE)) &&
            (inst.r.op != OP_SYSTEM)) { // do not interrupt when system call and CSR R/W
            ext_irq_next = 1;
        } else {
            ext_irq_next = 0;
        }
        ext_irq = (csr.msip & (1<<16)) ? 1 : 0;

        csr.time.c++;
        csr.instret.c++;
        CYCLE_ADD(1);

        prev_pc = pc;

#ifdef RV32C_ENABLED
        compressed = compressed_decoder(instc, &inst, &illegal);

        // one more cycle when the instruction type changes
        if (compressed_prev != compressed) {
            CYCLE_ADD(1);
            overhead++;
        }

        compressed_prev = compressed;

        if (compressed && 0)
            TRACE_LOG "           Translate 0x%04x => 0x%08x\n", (uint16_t)instc.inst, inst.inst TRACE_END;

        if (illegal) {
            TRAP(TRAP_INST_ILL, (int)instc.inst);
        }
#endif // RV32C_ENABLED

        switch(inst.r.op) {
        case OP_AUIPC: { // U-Type
            REGS_W(inst.u.rd, pc + to_imm_u(inst.u.imm));
            TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n", pc, inst.inst,
                       inst.u.rd, regname[inst.u.rd], REGS(inst.u.rd) TRACE_END;
            break;
        }
        case OP_LUI: { // U-Type
            REGS_W(inst.u.rd, to_imm_u(inst.u.imm));
            TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n", pc, inst.inst,
                      inst.u.rd, regname[inst.u.rd], REGS(inst.u.rd) TRACE_END;
            break;
        }
        case OP_JAL: { // J-Type
            REGS_W(inst.j.rd, compressed ? pc + 2 : pc + 4);
            TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n", pc, inst.inst,
                      inst.j.rd, regname[inst.j.rd], REGS(inst.j.rd) TRACE_END;
            pc += to_imm_j(inst.j.imm);
            if (to_imm_j(inst.j.imm) == 0) {
                printf("Warning: forever loop detected at PC 0x%08x\n", pc);
                prog_exit(1);
            }
            if ((pc&3) == 0)
                CYCLE_ADD(branch_penalty);
            continue;
        }
        case OP_JALR: { // I-Type
            int new_pc = compressed ? pc + 2 : pc + 4;
            TIME_LOG; TRACE_LOG "%08x ", pc TRACE_END;
            pc = REGS(inst.i.rs1) + to_imm_i(inst.i.imm);
            REGS_W(inst.i.rd, new_pc);
            TRACE_LOG "%08x x%02u (%s) <= 0x%08x\n", inst.inst, inst.i.rd,
                      regname[inst.i.rd], REGS(inst.i.rd) TRACE_END;
            if ((pc&3) == 0)
                CYCLE_ADD(branch_penalty);
            continue;
        }
        case OP_BRANCH: { // B-Type
            TIME_LOG; TRACE_LOG "%08x %08x\n", pc, inst.inst TRACE_END;
            int offset = to_imm_b(inst.b.imm2, inst.b.imm1);
            switch(inst.b.func3) {
                case OP_BEQ:
                    if (REGS(inst.b.rs1) == REGS(inst.b.rs2)) {
                        pc += offset;
                        if ((!branch_predict || offset > 0) && (pc&3) == 0)
                            CYCLE_ADD(branch_penalty);
                        continue;
                    }
                    break;
                case OP_BNE:
                    if (REGS(inst.b.rs1) != REGS(inst.b.rs2)) {
                        pc += offset;
                        if ((!branch_predict || offset > 0) && (pc&3) == 0)
                            CYCLE_ADD(branch_penalty);
                        continue;
                    }
                    break;
                case OP_BLT:
                    if (REGS(inst.b.rs1) < REGS(inst.b.rs2)) {
                        pc += offset;
                        if ((!branch_predict || offset > 0) && (pc&3) == 0)
                            CYCLE_ADD(branch_penalty);
                        continue;
                    }
                    break;
                case OP_BGE:
                    if (REGS(inst.b.rs1) >= REGS(inst.b.rs2)) {
                        pc += offset;
                        if ((!branch_predict || offset > 0) && (pc&3) == 0)
                            CYCLE_ADD(branch_penalty);
                        continue;
                    }
                    break;
                case OP_BLTU:
                    if (((uint32_t)REGS(inst.b.rs1)) < ((uint32_t)REGS(inst.b.rs2))) {
                        pc += offset;
                        if ((!branch_predict || offset > 0) && (pc&3) == 0)
                            CYCLE_ADD(branch_penalty);
                        continue;
                    }
                    break;
                case OP_BGEU:
                    if (((uint32_t)REGS(inst.b.rs1)) >= ((uint32_t)REGS(inst.b.rs2))) {
                        pc += offset;
                        if ((!branch_predict || offset > 0) && (pc&3) == 0)
                            CYCLE_ADD(branch_penalty);
                        continue;
                    }
                    break;
                default:
                    printf("Illegal branch instruction at PC 0x%08x\n", pc);
                    TRAP(TRAP_INST_ILL, inst.inst);
                    continue;
            }
            break;
        }
        case OP_LOAD: { // I-Type
            COUNTER counter;
            int memaddr;
            int data;
            int address = REGS(inst.i.rs1) + to_imm_i(inst.i.imm);
            memaddr = address;
            if (singleram) CYCLE_ADD(1);
            TIME_LOG; TRACE_LOG "%08x %08x", pc, inst.inst TRACE_END;
            if (address < DMEM_BASE || address > DMEM_BASE+DMEM_SIZE) {
                switch(address) {
                    case MMIO_PUTC:
                        data = 0;
                        break;
                    case MMIO_GETC:
                        data = getch();
                        break;
                    case MMIO_EXIT:
                        data = 0;
                        break;
                    case MMIO_MTIME:
                        counter.c = csr.mtime.c - 1;
                        data = counter.d.lo;
                        break;
                    case MMIO_MTIME+4:
                        counter.c = csr.mtime.c - 1;
                        data = counter.d.hi;
                        break;
                    case MMIO_MTIMECMP:
                        //csr.mip = csr.mip & ~(1 << MTIP);
                        data = csr.mtimecmp.d.lo;
                        break;
                    case MMIO_MTIMECMP+4:
                        //csr.mip = csr.mip & ~(1 << MTIP);
                        data = csr.mtimecmp.d.hi;
                        break;
                    case MMIO_MSIP:
                        data = csr.msip;
                        break;
                    default:
                        // Check whether it is legal, if not,
                        // leave it to the later judgment.
                        if (inst.i.func3 == OP_LB ||
                            inst.i.func3 == OP_LH ||
                            inst.i.func3 == OP_LW ||
                            inst.i.func3 == OP_LBU ||
                            inst.i.func3 == OP_LHU) {
                            TRACE_LOG "\n" TRACE_END;
                            printf("Unknown address 0x%08x to read at PC 0x%08x\n",
                                   address, pc);
                            TRAP(TRAP_LD_FAIL, address);
                            continue;
                        } else {
                            address = DVA2PA(address);
                            data = 0;
                        }
                }
            } else {
                address = DVA2PA(address);
                data = dmem[address/4];
            }
            switch(inst.i.func3) {
                case OP_LB:
                    // fall through
                case OP_LBU:
                    data = (data >> ((address&3)*8))&0xff;
                    if (inst.i.func3 == OP_LB && (data&0x80))
                      data |= 0xffffff00;
                    break;
                case OP_LH:
                    // fall through
                case OP_LHU:
                    if (address&1) {
                        TRACE_LOG "\n" TRACE_END;
                        printf("Unalignment address 0x%08x to read at PC 0x%08x\n",
                                DPA2VA(address), pc);
                        TRAP(TRAP_LD_ALIGN, DPA2VA(address));
                        continue;
                    }
                    data = (address&2) ? ((data>>16)&0xffff) : (data &0xffff);
                    if (inst.i.func3 == OP_LH && (data&0x8000))
                        data |= 0xffff0000;
                    break;
                case OP_LW:
                    if (address&3) {
                        TRACE_LOG "\n" TRACE_END;
                        printf("Unalignment address 0x%08x to read at PC 0x%08x\n",
                                DPA2VA(address), pc);
                        TRAP(TRAP_LD_ALIGN, DPA2VA(address));
                        continue;
                    }
                    break;
                default:
                    TRACE_LOG " read 0x%08x, x%02u (%s) <= 0x%08x\n",
                              memaddr, inst.i.rd,
                              regname[inst.i.rd], 0 TRACE_END;
                    printf("Illegal load instruction at PC 0x%08x\n", pc);
                    TRAP(TRAP_INST_ILL, inst.inst);
                    continue;
            }
            REGS_W(inst.i.rd, data);
            TRACE_LOG " read 0x%08x, x%02u (%s) <= 0x%08x\n",
                      memaddr, inst.i.rd,
                      regname[inst.i.rd], REGS(inst.i.rd) TRACE_END;
            break;
        }
        case OP_STORE: { // S-Type
            int address = REGS(inst.s.rs1) +
                          to_imm_s(inst.s.imm2, inst.s.imm1);
            int data = REGS(inst.s.rs2);
            int mask = (inst.s.func3 == OP_SB) ? 0xff :
                       (inst.s.func3 == OP_SH) ? 0xffff :
                       (inst.s.func3 == OP_SW) ? 0xffffffff :
                       0xffffffff;
            if (singleram) CYCLE_ADD(1);
            if (address < DMEM_BASE || address > DMEM_BASE+DMEM_SIZE) {
                if (inst.i.func3 == OP_SB ||
                    inst.i.func3 == OP_SH ||
                    inst.i.func3 == OP_SW) {
                    TIME_LOG; TRACE_LOG "%08x %08x", pc, inst.inst TRACE_END;
                }
                switch(address) {
                    case MMIO_PUTC:
                        putchar((char)data);
                        fflush(stdout);
                        break;
                    case MMIO_GETC:
                        break;
                    case MMIO_EXIT:
                        TRACE_LOG " write 0x%08x <= 0x%08x\n",
                                  address, (data & mask) TRACE_END;
                        prog_exit(data);
                        break;
                    case MMIO_MTIME:
                        csr.mtime.d.lo = (csr.mtime.d.lo & ~mask) | data;
                        csr.mtime.c--;
                        mtime_update = 1;
                        break;
                    case MMIO_MTIME+4:
                        csr.mtime.d.hi = (csr.mtime.d.hi & ~mask) | data;
                        mtime_update = 1;
                        break;
                    case MMIO_MTIMECMP:
                        csr.mtimecmp.d.lo = (csr.mtimecmp.d.lo & ~mask) | data;
                        break;
                    case MMIO_MTIMECMP+4:
                        csr.mtimecmp.d.hi = (csr.mtimecmp.d.hi & ~mask) | data;
                        break;
                    case MMIO_MSIP:
                        csr.msip = (csr.msip & ~mask) | data;
                        break;
                    default:
                        // Check whether it is legal, if not,
                        // leave it to the later judgment.
                        if (inst.i.func3 == OP_SB ||
                            inst.i.func3 == OP_SH ||
                            inst.i.func3 == OP_SW) {
                            printf("Unknown address 0x%08x to write at PC 0x%08x\n",
                                   address, pc);
                            TRACE_LOG "\n" TRACE_END;
                            TRAP(TRAP_ST_FAIL, address);
                            continue;
                        }
                }
                if (inst.i.func3 == OP_SB ||
                    inst.i.func3 == OP_SH ||
                    inst.i.func3 == OP_SW) {
                    TRACE_LOG " write 0x%08x <= 0x%08x\n",
                          address, (data & mask) TRACE_END;
                    pc = compressed ? pc + 2 : pc + 4;
                    continue;
                }
            }
            TIME_LOG; TRACE_LOG "%08x %08x", pc, inst.inst TRACE_END;
            address = DVA2PA(address);
            switch(inst.s.func3) {
                case OP_SB:
                    dmem[address/4] =
                       (dmem[address/4]&~(0xff<<((address&3)*8))) |
                       ((data & 0xff)<<((address&3)*8));
                    break;
                case OP_SH:
                    if (address&1) {
                       TRACE_LOG "\n" TRACE_END;
                       printf("Unalignment address 0x%08x to write at PC 0x%08x\n",
                               DPA2VA(address), pc);
                       TRAP(TRAP_ST_ALIGN, DPA2VA(address));
                       continue;
                    }
                    dmem[address/4] = (address&2) ?
                       ((dmem[address/4]&0xffff)|(data << 16)) :
                       ((dmem[address/4]&0xffff0000)|(data&0xffff));
                    break;
                case OP_SW:
                    if (address&3) {
                       TRACE_LOG "\n" TRACE_END;
                       printf("Unalignment address 0x%08x to write at PC 0x%08x\n",
                               DPA2VA(address), pc);
                       TRAP(TRAP_ST_ALIGN, DPA2VA(address));
                       continue;
                    }
                    dmem[address/4] = data;
                    break;
                default:
                    printf("Illegal store instruction at PC 0x%08x\n", pc);
                    TRACE_LOG "\n" TRACE_END;
                    TRAP(TRAP_INST_ILL, inst.inst);
                    continue;
            }
            TRACE_LOG " write 0x%08x <= 0x%08x\n", DPA2VA(address), (data & mask) TRACE_END;
            break;
        }
        case OP_ARITHI: { // I-Type
            switch(inst.i.func3) {
                case OP_ADD:
                    REGS_W(inst.i.rd, REGS(inst.i.rs1) + to_imm_i(inst.i.imm));
                    break;
                case OP_SLT:
                    REGS_W(inst.i.rd, REGS(inst.i.rs1) < to_imm_i(inst.i.imm) ? 1 : 0);
                    break;
                case OP_SLTU:
                    //FIXME: to pass compliance test, the IMM should be singed
                    //extension, and compare with unsigned.
                    //REGS_W(inst.i.rd, ((uint32_t)REGS(inst.i.rs1)) <
                    //                ((uint32_t)to_imm_iu(inst.i.imm)) ? 1 : 0);
                    REGS_W(inst.i.rd, ((uint32_t)REGS(inst.i.rs1)) <
                                      ((uint32_t)to_imm_i(inst.i.imm)) ? 1 : 0);
                    break;
                case OP_XOR:
                    REGS_W(inst.i.rd, REGS(inst.i.rs1) ^ to_imm_i(inst.i.imm));
                    break;
                case OP_OR:
                    REGS_W(inst.i.rd, REGS(inst.i.rs1) | to_imm_i(inst.i.imm));
                    break;
                case OP_AND:
                    REGS_W(inst.i.rd, REGS(inst.i.rs1) & to_imm_i(inst.i.imm));
                    break;
                case OP_SLL:
                    switch (inst.r.func7) {
                        case FN_RV32I:
                            REGS_W(inst.i.rd, REGS(inst.i.rs1) << (inst.i.imm&0x1f));
                            break;
                        #ifdef RV32B_ENABLED
                        case FN_BSET:
                            REGS_W(inst.i.rd, REGS(inst.i.rs1) | (1 << (inst.i.imm&0x1f)));
                            break;
                        case FN_BCLR:
                            REGS_W(inst.i.rd, REGS(inst.i.rs1) & ~(1 << (inst.i.imm&0x1f)));
                            break;
                        case FN_CLZ:
                            switch (inst.r.rs2) {
                                case 0: // CLZ
                                    {
                                        int32_t r = 0;
                                        int32_t x = REGS(inst.i.rs1);
                                        if (!x) {
                                            r = 32;
                                        } else {
                                            if (!(x & 0xffff0000)) { x <<= 16; r += 16; }
                                            if (!(x & 0xff000000)) { x <<=  8; r +=  8; }
                                            if (!(x & 0xf0000000)) { x <<=  4; r +=  4; }
                                            if (!(x & 0xc0000000)) { x <<=  2; r +=  2; }
                                            if (!(x & 0x80000000)) { x <<=  1; r +=  1; }
                                        }
                                        REGS_W(inst.i.rd, r);
                                    }
                                    break;
                                case 2: // CPOP
                                    {
                                        uint32_t c = 0;
                                        int32_t n = REGS(inst.i.rs1);
                                        while (n) {
                                            n &= (n - 1);
                                            c++;
                                        }
                                        REGS_W(inst.i.rd, c);
                                    }
                                    break;
                                case 1: // CTZ
                                    {
                                        int32_t x = REGS(inst.i.rs1);
	                                    static const uint8_t table[32] = {
		                                    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		                                    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	                                    };
                                        int32_t n = (!x) ? 32 : (int32_t)table[((uint32_t)((x & -x) * 0x077CB531U)) >> 27];
	                                    REGS_W(inst.i.rd, n);
                                    }
                                    break;
                                case 4: // SEXT.B
                                    {
                                        uint32_t n = REGS(inst.i.rs1) & 0xff;
                                        if (n&0x80)
                                            n |= 0xffffff00;
                                        REGS_W(inst.i.rd, n);
                                    }
                                    break;
                                case 5: // SEXT.H
                                    {
                                        uint32_t n = REGS(inst.i.rs1) & 0xffff;
                                        if (n&0x8000)
                                            n |= 0xffff0000;
                                        REGS_W(inst.i.rd, n);
                                    }
                                    break;
                                default:
                                    printf("Unknown instruction at PC 0x%08x\n", pc);
                                    TRAP(TRAP_INST_ILL, inst.inst);
                                    continue;
                            }
                            break;
                        case FN_BINV:
                            REGS_W(inst.i.rd, REGS(inst.i.rs1) ^ (1 << (inst.i.imm&0x1f)));
                            break;
                        #endif // RV32B_ENABLED
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            continue;
                    }
                    break;
                case OP_SR:
                    switch (inst.r.func7) {
                        case FN_SRL: // SRLI
                            REGS_W(inst.i.rd, ((uint32_t)REGS(inst.i.rs1)) >>
                                               (inst.i.imm&0x1f));
                            break;
                        case FN_SRA: // SRAI
                            REGS_W(inst.i.rd, REGS(inst.i.rs1) >> (inst.i.imm&0x1f));
                            break;
                        #ifdef RV32B_ENABLED
                        case FN_BSET:
                            if (inst.r.rs2 == 7) { // ORC.B
                                int32_t n = 0;
                                int32_t v = REGS(inst.i.rs1);
                                if (v & 0x000000ff) n |= 0x000000ff;
                                if (v & 0x0000ff00) n |= 0x0000ff00;
                                if (v & 0x00ff0000) n |= 0x00ff0000;
                                if (v & 0xff000000) n |= 0xff000000;
                                REGS_W(inst.i.rd, n);
                            } else {
                                printf("Unknown instruction at PC 0x%08x\n", pc);
                                TRAP(TRAP_INST_ILL, inst.inst);
                                continue;
                            }
                            break;
                        case FN_BCLR: // BCLRI
                            REGS_W(inst.i.rd, (REGS(inst.i.rs1) >> (inst.i.imm&0x1f)) & 1);
                            break;
                        case FN_CLZ: // RORI
                            {
                                uint32_t n = REGS(inst.i.rs1);
                                REGS_W(inst.i.rd, (n >> (inst.i.imm&0x1f)) |
                                                  (n << (32 - (inst.i.imm&0x1f))));
                            }
                            break;
                        case FN_REV:
                            switch(inst.i.imm&0x1f) {
                                case 0x18: // REV.8
                                    {
                                        uint32_t n = REGS(inst.i.rs1);
                                        REGS_W(inst.i.rd,
                                               ((n >> 24) & 0x000000ff) |
                                               ((n >>  8) & 0x0000ff00) |
                                               ((n <<  8) & 0x00ff0000) |
                                               ((n << 24) & 0xff000000));
                                    }
                                    break;
                                default:
                                    printf("Unknown instruction at PC 0x%08x\n", pc);
                                    TRAP(TRAP_INST_ILL, inst.inst);
                                    continue;
                            }
                            break;
                        #endif // RV32B_ENABLED
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            continue;
                    }
                    break;
                default:
                    printf("Unknown instruction at PC 0x%08x\n", pc);
                    TRAP(TRAP_INST_ILL, inst.inst);
                    continue;
            }
            TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n",
                      pc, inst.inst, inst.i.rd, regname[inst.i.rd],
                      REGS(inst.i.rd) TRACE_END;
            break;
        }
        case OP_ARITHR: { // R-Type
            switch (inst.r.func7) {
                #ifdef RV32M_ENABLED
                case FN_RV32M: // RV32M Multiply Extension
                    switch(inst.r.func3) {
                        case OP_MUL:
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) *
                                              REGS(inst.r.rs2));
                            break;
                        case OP_MULH:
                            {
                            union {
                                int64_t l;
                                struct { int32_t l, h; } n;
                            } a, b, r;
                            a.l = (int64_t)REGS(inst.r.rs1);
                            b.l = (int64_t)REGS(inst.r.rs2);
                            r.l = a.l * b.l;
                            REGS_W(inst.r.rd, r.n.h);
                            }
                            break;
                        case OP_MULSU:
                            {
                            union {
                                int64_t l;
                                struct { int32_t l, h; } n;
                            } a, b, r;
                            a.l = (int64_t)REGS(inst.r.rs1);
                            b.n.l = REGS(inst.r.rs2);
                            b.n.h = 0;
                            r.l = a.l * b.l;
                            REGS_W(inst.r.rd, r.n.h);
                            }
                            break;
                        case OP_MULU:
                            {
                            union {
                                int64_t l;
                                struct { int32_t l, h; } n;
                            } a, b, r;
                            a.n.l = REGS(inst.r.rs1); a.n.h = 0;
                            b.n.l = REGS(inst.r.rs2); b.n.h = 0;
                            r.l = ((uint64_t)a.l) *
                                  ((uint64_t)b.l);
                            REGS_W(inst.r.rd, r.n.h);
                            }
                            break;
                        case OP_DIV:
                            if (REGS(inst.r.rs2))
                                REGS_W(inst.r.rd, (int32_t)(((int64_t)REGS(inst.r.rs1)) /
                                                            REGS(inst.r.rs2)));
                            else
                                REGS_W(inst.r.rd, 0xffffffff);
                            break;
                        case OP_DIVU:
                            if (REGS(inst.r.rs2))
                                REGS_W(inst.r.rd, (int32_t)(((uint32_t)REGS(inst.r.rs1)) /
                                                            ((uint32_t)REGS(inst.r.rs2))));
                            else
                                REGS_W(inst.r.rd, 0xffffffff);
                            break;
                        case OP_REM:
                            if (REGS(inst.r.rs2))
                                REGS_W(inst.r.rd, (int32_t)(((int64_t)REGS(inst.r.rs1)) %
                                                            REGS(inst.r.rs2)));
                            else
                                REGS_W(inst.r.rd, REGS(inst.r.rs1));
                            break;
                        case OP_REMU:
                            if (REGS(inst.r.rs2))
                                REGS_W(inst.r.rd, (int32_t)(((uint32_t)REGS(inst.r.rs1)) %
                                                            ((uint32_t)REGS(inst.r.rs2))));
                            else
                                REGS_W(inst.r.rd, REGS(inst.r.rs1));
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            continue;
                    }
                break;
                #endif // RV32M_ENABLED

                case FN_RV32I:
                    switch(inst.r.func3) {
                        case OP_ADD:
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) + REGS(inst.r.rs2));
                            break;
                        case OP_SLL:
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) << REGS(inst.r.rs2));
                            break;
                        case OP_SLT:
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) < REGS(inst.r.rs2) ?
                                              1 : 0);
                            break;
                        case OP_SLTU:
                            REGS_W(inst.r.rd, ((uint32_t)REGS(inst.r.rs1)) <
                                 ((uint32_t)REGS(inst.r.rs2)) ? 1 : 0);
                            break;
                        case OP_XOR:
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) ^ REGS(inst.r.rs2));
                            break;
                        case OP_SR:
                            REGS_W(inst.r.rd, ((uint32_t)REGS(inst.r.rs1)) >>
                                              REGS(inst.r.rs2));
                            break;
                        case OP_OR:
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) | REGS(inst.r.rs2));
                            break;
                        case OP_AND:
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) & REGS(inst.r.rs2));
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            continue;
                    }
                break;

                case FN_ANDN:
                    switch(inst.r.func3) {
                        case OP_ADD: // SUB
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) - REGS(inst.r.rs2));
                            break;
                        case OP_SR: // SRA
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) >> REGS(inst.r.rs2));
                            break;
                        #ifdef RV32B_ENABLED
                        case OP_AND: // ANDN
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) & ~(REGS(inst.r.rs2)));
                            break;
                        case OP_OR: // ORN
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) | ~(REGS(inst.r.rs2)));
                            break;
                        case OP_XOR: // XNOR
                            REGS_W(inst.r.rd, ~(REGS(inst.r.rs1) ^ REGS(inst.r.rs2)));
                            break;
                        #endif // RV32B_ENABLED
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            continue;
                    }
                    break;

                #ifdef RV32B_ENABLED
                case FN_ZEXT:
                    REGS_W(inst.r.rd, REGS(inst.r.rs1) & 0xffff);
                    break;

                case FN_MINMAX:
                    switch(inst.r.func3) {
                        case OP_CLMUL:
                            {
                                int32_t a = REGS(inst.r.rs1);
                                int32_t b = REGS(inst.r.rs2);
                                int32_t n = 0;

                                for(int i = 0; i <= 31; i++)
                                    if ((b >> i) & 1) n ^= (a << i);

                                REGS_W(inst.r.rd, n);
                            }
                            break;
                        case OP_CLMULH:
                            {
                                uint32_t a = REGS(inst.r.rs1);
                                uint32_t b = REGS(inst.r.rs2);
                                int32_t n = 0;

                                for(int i = 1; i < 32; i++)
                                    if ((b >> i) & 1) n ^= (a >> (32 - i));

                                REGS_W(inst.r.rd, n);
                            }
                            break;
                        case OP_CLMULR:
                            {
                                uint32_t a = REGS(inst.r.rs1);
                                uint32_t b = REGS(inst.r.rs2);
                                int32_t n = 0;

                                for(int i = 0; i < 32; i++)
                                    if ((b >> i) & 1) n ^= (a >> (32 - i - 1));

                                REGS_W(inst.r.rd, n);
                            }
                            break;
                        case OP_MAX:
                            {
                                int32_t a = REGS(inst.r.rs1);
                                int32_t b = REGS(inst.r.rs2);
                                REGS_W(inst.r.rd, a > b ? a : b);
                            }
                            break;
                        case OP_MAXU:
                            {
                                uint32_t a = REGS(inst.r.rs1);
                                uint32_t b = REGS(inst.r.rs2);
                                REGS_W(inst.r.rd, a > b ? a : b);
                            }
                            break;
                        case OP_MIN:
                            {
                                int32_t a = REGS(inst.r.rs1);
                                int32_t b = REGS(inst.r.rs2);
                                REGS_W(inst.r.rd, a < b ? a : b);
                            }
                            break;
                        case OP_MINU:
                            {
                                uint32_t a = REGS(inst.r.rs1);
                                uint32_t b = REGS(inst.r.rs2);
                                REGS_W(inst.r.rd, a < b ? a : b);
                            }
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            continue;
                    }
                    break;

                case FN_SHADD:
                    switch(inst.r.func3) {
                        case OP_SH1ADD:
                            REGS_W(inst.r.rd, REGS(inst.r.rs2) + (REGS(inst.r.rs1) << 1));
                            break;
                        case OP_SH2ADD:
                            REGS_W(inst.r.rd, REGS(inst.r.rs2) + (REGS(inst.r.rs1) << 2));
                            break;
                        case OP_SH3ADD:
                            REGS_W(inst.r.rd, REGS(inst.r.rs2) + (REGS(inst.r.rs1) << 3));
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            continue;
                    }
                    break;

                case FN_BSET:
                    REGS_W(inst.r.rd, REGS(inst.r.rs1) | (1 << (REGS(inst.r.rs2) & 0x1f)));
                    break;

                case FN_BCLR:
                    switch(inst.r.func3) {
                        case OP_BCLR:
                            REGS_W(inst.r.rd, REGS(inst.r.rs1) & ~(1 << (REGS(inst.r.rs2) & 0x1f)));
                            break;
                        case OP_BEXT:
                            REGS_W(inst.r.rd, (REGS(inst.r.rs1) >> (REGS(inst.r.rs2) & 0x1f)) & 1);
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            continue;
                    }
                    break;

                case FN_CLZ:
                    switch(inst.r.func3) {
                        case OP_ROL:
                            {
                                uint32_t n = REGS(inst.r.rs2) & 0x1f;
                                REGS_W(inst.r.rd, (REGS(inst.r.rs1) << n) |
                                                  ((uint32_t)REGS(inst.r.rs1) >> (32 - n)));
                            }
                            break;
                        case OP_ROR:
                            {
                                uint32_t n = REGS(inst.r.rs2) & 0x1f;
                                REGS_W(inst.r.rd, ((uint32_t)REGS(inst.r.rs1) >> n) |
                                                  (REGS(inst.r.rs1) << (32 - n)));
                            }
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            continue;
                    }
                    break;

                case FN_BINV:
                    REGS_W(inst.r.rd, REGS(inst.r.rs1) ^ (1 << (REGS(inst.r.rs2) & 0x1f)));
                    break;
                #endif // RV32B_ENABLED

                default:
                    printf("Unknown instruction at PC 0x%08x\n", pc);
                    TRAP(TRAP_INST_ILL, inst.inst);
                    continue;
            }
            TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n",
                      pc, inst.inst, inst.r.rd, regname[inst.r.rd],
                      REGS(inst.r.rd) TRACE_END;
            break;
        }
        case OP_FENCE: {
            TIME_LOG; TRACE_LOG "%08x %08x\n", pc, inst.inst TRACE_END;
            break;
        }
        case OP_SYSTEM: { // I-Type
            int val;
            int update;
            int csr_op = 0;
            int csr_type;
            int res;
            // RDCYCLE, RDTIME and RDINSTRET are read only
            switch(inst.i.func3) {
                case OP_ECALL:
                    TIME_LOG; TRACE_LOG "%08x %08x\n", pc, inst.inst TRACE_END;
                    switch (inst.i.imm & 3) {
                       case 0: // ecall
                           res = srv32_syscall(REGS(SYS), REGS(A0),
                                               REGS(A1), REGS(A2),
                                               REGS(A3), REGS(A4),
                                               REGS(A5));
                           // Notes: FreeRTOS will use ecall to perform context switching.
                           // The syscall of newlib will confict with the syscall of
                           // FreeRTOS.
                           #if 0
                           // FIXME: if it is prefined syscall, excute it.
                           // otherwise raising a trap
                           if (res != -1) {
                                REGS_W(A0, res);
                           } else {
                                TRAP(TRAP_ECALL, 0);
                                continue;
                           }
                           break;
                           #else
                           if (res != -1)
                                REGS_W(A0, res);
                           TRAP(TRAP_ECALL, 0);
                           continue;
                           #endif
                       case 1: // ebreak
                           TRAP(TRAP_BREAK, 0);
                           continue;
                       case 2: // mret
                           pc = csr.mepc;
                           // mstatus.mie = mstatus.mpie
                           csr.mstatus = (csr.mstatus & (1 << MPIE)) ?
                                         (csr.mstatus | (1 << MIE)) :
                                         (csr.mstatus & ~(1 << MIE));
                           // mstatus.mpie = 1
                           if ((pc&3) == 0)
                               CYCLE_ADD(branch_penalty);
                           continue;
                       default:
                           printf("Illegal system call at PC 0x%08x\n", pc);
                           TRAP(TRAP_INST_ILL, 0);
                           continue;
                    }
                    break;
                case OP_CSRRWI:
                    csr_op   = 1;
                    val      = inst.i.rs1;
                    update   = 1;
                    csr_type = OP_CSRRW;
                    break;
                // If the zimm[4:0] field is zero, then these instructions will not write
                // to the CSR
                case OP_CSRRW:
                    csr_op   = 1;
                    val      = REGS(inst.i.rs1);
                    update   = 1;
                    csr_type = OP_CSRRW;
                    break;
                // For both CSRRS and CSRRC, if rs1=x0, then the instruction will not
                // write to the CSR at all
                case OP_CSRRSI:
                    csr_op   = 1;
                    val      = inst.i.rs1;
                    update   = (inst.i.rs1 == 0) ? 0 : 1;
                    csr_type = OP_CSRRS;
                    break;
                case OP_CSRRS:
                    csr_op   = 1;
                    val      = REGS(inst.i.rs1);
                    update   = (inst.i.rs1 == 0) ? 0 : 1;
                    csr_type = OP_CSRRS;
                    break;
                case OP_CSRRCI:
                    csr_op   = 1;
                    val      = inst.i.rs1;
                    update   = (inst.i.rs1 == 0) ? 0 : 1;
                    csr_type = OP_CSRRC;
                    break;
                case OP_CSRRC:
                    csr_op   = 1;
                    val      = REGS(inst.i.rs1);
                    update   = (inst.i.rs1 == 0) ? 0 : 1;
                    csr_type = OP_CSRRC;
                    break;
                default:
                    printf("Unknown system instruction at PC 0x%08x\n", pc);
                    TIME_LOG; TRACE_LOG "%08x %08x\n", pc, inst.inst TRACE_END;
                    TRAP(TRAP_INST_ILL, inst.inst);
                    continue;
            }
            if (csr_op) {
                int legal = 0;
                int result = csr_rw(inst.i.imm, csr_type, val, update, &legal);
                //if (legal) { // FIXME
                    REGS_W(inst.i.rd, result);
                //}
                TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n",
                          pc, inst.inst, inst.i.rd,
                          regname[inst.i.rd], REGS(inst.i.rd) TRACE_END;
                if (!legal) {
                   TRAP(TRAP_INST_ILL, 0);
                   continue;
                }
            }
            break;
        }
        default: {
            printf("Illegal instruction at PC 0x%08x\n", pc);
            TIME_LOG; TRACE_LOG "%08x %08x\n", pc, inst.inst TRACE_END;
            TRAP(TRAP_INST_ILL, inst.inst);
            continue;
        }
        } // end of switch(inst.r.op)
        pc = compressed ? pc + 2 : pc + 4;
    }

    aligned_free(mem);
    if (ft) fclose(ft);
}

