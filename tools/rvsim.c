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
#include "rvsim.h"

#ifdef GDBSTUB
extern struct target_ops gdbstub_ops;
#endif

#define PRINT_TIMELOG 1
#define MAXLEN      1024

#define TIME_LOG    if (rv->ft && PRINT_TIMELOG) fprintf(rv->ft, "%10d ", rv->csr.cycle.d.lo)
#define TRACE_LOG   if (rv->ft) fprintf(rv->ft,
#define TRACE_END   )

#define TRAP(cause,val) { \
    CYCLE_ADD(rv->branch_penalty); \
    rv->csr.mcause = cause; \
    rv->csr.mstatus = (rv->csr.mstatus &  (1<<MIE)) ? (rv->csr.mstatus | (1<<MPIE)) : (rv->csr.mstatus & ~(1<<MPIE)); \
    rv->csr.mstatus = (rv->csr.mstatus & ~(1<<MIE)); \
    rv->csr.mepc = rv->prev_pc; \
    rv->csr.mtval = (val); \
    rv->pc = (rv->csr.mtvec & 1) ? (rv->csr.mtvec & 0xfffffffe) + cause * 4 : rv->csr.mtvec; \
}

#define INT(cause,src) { \
    /* When the branch instruction is interrupted, do not accumulate cycles, */ \
    /* which has been added when the branch instruction is executed. */ \
    if (rv->pc == (compressed ? rv->prev_pc+2 : rv->prev_pc+4)) CYCLE_ADD(rv->branch_penalty); \
    rv->csr.mcause = cause; \
    rv->csr.mstatus = (rv->csr.mstatus &  (1<<MIE)) ? (rv->csr.mstatus | (1<<MPIE)) : (rv->csr.mstatus & ~(1<<MPIE)); \
    rv->csr.mstatus = (rv->csr.mstatus & ~(1<<MIE)); \
    rv->csr.mip = rv->csr.mip | (1 << src); \
    rv->csr.mepc = rv->pc; \
    rv->pc = (rv->csr.mtvec & 1) ? (rv->csr.mtvec & 0xfffffffe) + (cause & (~(1<<31))) * 4 : rv->csr.mtvec; \
}

#define CYCLE_ADD(count) { \
    rv->csr.cycle.c = rv->csr.cycle.c + count; \
    if (!mtime_update) rv->csr.mtime.c = rv->csr.mtime.c + count; \
}

int singleram = 0;
int mtime_update = 0;
struct timeval time_start;
struct timeval time_end;

#ifdef RV32C_ENABLED
int overhead = 0;
#endif // RV32C_ENABLED

int quiet = 0;

const char *regname[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0(fp)", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

int elfloader(char *file, struct rv *rv);
int getch(void);
void debug(struct rv *rv);

void usage(void) {
// LCOV_EXCL_START
    printf(
"Instruction Set Simulator for RV32IM, (c) 2020 Kuoping Hsu\n"
"Usage: rvsim [-h] [-d] [-g port] [-m n] [-n n] [-b n] [-p] [-l logfile] file\n\n"
"       --help, -h              help\n"
"       --debug, -d             interactive debug mode\n"
"       --gdb port, -g port     enable gdb debugger with port\n"
"       --quiet, -q             quite\n"
"       --membase n, -m n       memory base\n"
"       --memsize n, -n n       memory size for iram and dram each (in Kb)\n"
"       --branch n, -b n        branch penalty (default 2)\n"
"       --single, -s            single RAM\n"
"       --predict, -p           static branch prediction\n"
"       --log file, -l file     generate log file\n"
"\n"
"       file                    the elf executable file\n"
"\n"
    );
// LCOV_EXCL_STOP
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

void prog_exit(struct rv *rv, int exitcode) {
    double diff;
    gettimeofday(&time_end, NULL);

    diff = (double)(time_end.tv_sec-time_start.tv_sec) +
                   (time_end.tv_usec-time_start.tv_usec)/1000000.0;

    if (!quiet && rv) {
#ifdef RV32C_ENABLED
        printf("\nExcuting %lld instructions, %lld cycles, %1.3f CPI, %1.3f%% overhead\n",
               rv->csr.instret.c, rv->csr.cycle.c,
               ((float)rv->csr.cycle.c)/rv->csr.instret.c, (overhead*100.0)/rv->csr.instret.c);
#else
        printf("\nExcuting %lld instructions, %lld cycles, %1.3f CPI\n", rv->csr.instret.c,
               rv->csr.cycle.c, ((float)rv->csr.cycle.c)/rv->csr.instret.c);
#endif // RV32C_ENABLED

        printf("Program terminate\n");

        printf("\n");
        printf("Simulation statistics\n");
        printf("=====================\n");
        printf("Simulation time  : %0.3f s\n", (float)diff);
        printf("Simulation cycles: %lld\n", rv->csr.cycle.c);
        printf("Simulation speed : %0.3f MHz\n", (float)(rv->csr.cycle.c / diff / 1000000.0));
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

int csr_rw(struct rv *rv, int regs, int mode, int val, int update, int *legal) {
    COUNTER counter;
    int result = 0;
    *legal = 1;
    switch(regs) {
        case CSR_RDCYCLE    : counter.c = rv->csr.cycle.c - 1;
                              result = counter.d.lo;
                              // UPDATE_CSR(update, mode, rv->csr.cycle.d.lo, val);
                              break;
        case CSR_RDCYCLEH   : counter.c = rv->csr.cycle.c - 1;
                              result = counter.d.hi;
                              // UPDATE_CSR(update, mode, rv->csr.cycle.d.hi, val);
                              break;
        /*
        case CSR_RDTIME     : counter.c = rv->csr.time.c - 1;
                              result = counter.d.lo;
                              // UPDATE_CSR(update, mode, rv->csr.time.d.lo, val);
                              break;
        case CSR_RDTIMEH    : counter.c = rv->csr.time.c - 1;
                              result = counter.d.hi;
                              // UPDATE_CSR(update, mode, rv->csr.time.d.hi, val);
                              break;
        */
        case CSR_RDINSTRET  : counter.c = rv->csr.instret.c - 1;
                              result = counter.d.lo;
                              // UPDATE_CSR(update, mode, rv->csr.instret.d.lo, val);
                              break;
        case CSR_RDINSTRETH : counter.c = rv->csr.instret.c - 1;
                              result = counter.d.hi;
                              // UPDATE_CSR(update, mode, rv->csr.instret.d.hi, val);
                              break;
        case CSR_MVENDORID  : result = rv->csr.mvendorid;
                              // UPDATE_CSR(update, mode, rv->csr.mvendorid, val);
                              break;
        case CSR_MARCHID    : result = rv->csr.marchid;
                              // UPDATE_CSR(update, mode, rv->csr.marchid, val);
                              break;
        case CSR_MIMPID     : result = rv->csr.mimpid;
                              // UPDATE_CSR(update, mode, rv->csr.mimpid, val);
                              break;
        case CSR_MHARTID    : result = rv->csr.mhartid;
                              // UPDATE_CSR(update, mode, rv->csr.mhartid, val);
                              break;
        case CSR_MSCRATCH   : result = rv->csr.mscratch;
                              UPDATE_CSR(update, mode, rv->csr.mscratch, val);
                              break;
        case CSR_MSTATUS    : result = rv->csr.mstatus;
                              UPDATE_CSR(update, mode, rv->csr.mstatus, val);
                              break;
        case CSR_MSTATUSH   : result = rv->csr.mstatush;
                              UPDATE_CSR(update, mode, rv->csr.mstatush, val);
                              break;
        case CSR_MISA       : result = rv->csr.misa;
                              UPDATE_CSR(update, mode, rv->csr.misa, val);
                              break;
        case CSR_MIE        : result = rv->csr.mie;
                              UPDATE_CSR(update, mode, rv->csr.mie, val);
                              break;
        case CSR_MIP        : result = rv->csr.mip;
                              UPDATE_CSR(update, mode, rv->csr.mip, val);
                              break;
        case CSR_MTVEC      : result = rv->csr.mtvec;
                              UPDATE_CSR(update, mode, rv->csr.mtvec, val);
                              break;
        case CSR_MEPC       : result = rv->csr.mepc;
                              UPDATE_CSR(update, mode, rv->csr.mepc, val);
                              break;
        case CSR_MCAUSE     : result = rv->csr.mcause;
                              UPDATE_CSR(update, mode, rv->csr.mcause, val);
                              break;
        case CSR_MTVAL      : result = rv->csr.mtval;
                              UPDATE_CSR(update, mode, rv->csr.mtval, val);
                              break;
#ifdef XV6_SUPPORT
        case CSR_MEDELEG    : result = rv->csr.medeleg;
                              UPDATE_CSR(update, mode, rv->csr.medeleg, val);
                              break;
        case CSR_MIDELEG    : result = rv->csr.mideleg;
                              UPDATE_CSR(update, mode, rv->csr.mideleg, val);
                              break;
        case CSR_MCOUNTEREN : result = rv->csr.mcounteren;
                              UPDATE_CSR(update, mode, rv->csr.mcounteren, val);
                              break;
        case CSR_SSTATUS    : result = rv->csr.sstatus;
                              UPDATE_CSR(update, mode, rv->csr.sstatus, val);
                              break;
        case CSR_SIE        : result = rv->csr.sie;
                              UPDATE_CSR(update, mode, rv->csr.sie, val);
                              break;
        case CSR_STVEC      : result = rv->csr.stvec;
                              UPDATE_CSR(update, mode, rv->csr.stvec, val);
                              break;
        case CSR_SSCRATCH   : result = rv->csr.sscratch;
                              UPDATE_CSR(update, mode, rv->csr.sscratch, val);
                              break;
        case CSR_SEPC       : result = rv->csr.sepc;
                              UPDATE_CSR(update, mode, rv->csr.sepc, val);
                              break;
        case CSR_SCAUSE     : result = rv->csr.scause;
                              UPDATE_CSR(update, mode, rv->csr.scause, val);
                              break;
        case CSR_STVAL      : result = rv->csr.stval;
                              UPDATE_CSR(update, mode, rv->csr.stval, val);
                              break;
        case CSR_SIP        : result = rv->csr.sip;
                              UPDATE_CSR(update, mode, rv->csr.sip, val);
                              break;
        case CSR_SATP       : result = rv->csr.satp;
                              UPDATE_CSR(update, mode, rv->csr.satp, val);
                              break;
#endif // XV6_SUPPORT
        default: result = 0;
                 printf("Unsupport CSR register 0x%03x at PC 0x%08x\n", regs, rv->pc);
                 *legal = 0;
    }
    return result;
}

int32_t srv32_read_regs(struct rv *rv, int n) {
    if (n >= REGNUM) {
        printf("RV32E: can not access registers %d\n", n);
        return 0;
    } else {
        return rv->regs[n];
    }
}

void srv32_write_regs(struct rv *rv, int n, int32_t v) {
    if (n >= REGNUM) {
        printf("RV32E: can not access registers %d\n", n);
    } else {
        rv->regs[n] = v;
    }
}

void *srv32_get_memptr(struct rv *rv, int32_t addr) {
    if (addr < rv->mem_base || addr > (rv->mem_base + rv->mem_size))
        return NULL;

    return (void*)&((char*)rv->mem)[addr - rv->mem_base];
}

bool srv32_write_mem(struct rv *rv, int32_t addr, int32_t len, void *ptr)
{
    int i;
    char *src = (char*)ptr;
    char *dst = (char*)&((char*)rv->mem)[addr - rv->mem_base];

    if (addr < rv->mem_base || (addr + len) > (rv->mem_base + rv->mem_size))
        return false;

    for(i = 0; i < len; i++)
        dst[i] = src[i];

    return true;
}

bool srv32_read_mem(struct rv *rv, int32_t addr, int32_t len, void *ptr)
{
    int i;
    char *src = (char*)&((char*)rv->mem)[addr - rv->mem_base];
    char *dst = (char*)ptr;

    if (addr < rv->mem_base || (addr + len) > (rv->mem_base + rv->mem_size))
        return false;

    for(i = 0; i < len; i++)
        dst[i] = src[i];

    return true;
}

static int memrw(struct rv *rv, int type, int op, int32_t address, int32_t *val) {
    COUNTER counter;

    if (type == OP_LOAD) {
        int32_t data = 0;
        int32_t len = 0;
        *val = 0;

        switch(op) {
            case OP_LB:
                len = 1;
                break;
            case OP_LBU:
                len = 1;
                break;
            case OP_LH:
                if (address & 1) {
                    printf("Unalignment address 0x%08x to read at PC 0x%08x\n",
                            address, rv->pc);
                    return TRAP_LD_ALIGN;
                }
                len = 2;
                break;
            case OP_LHU:
                if (address & 1) {
                    printf("Unalignment address 0x%08x to read at PC 0x%08x\n",
                            address, rv->pc);
                    return TRAP_LD_ALIGN;
                }
                len = 2;
                break;
            case OP_LW:
                if (address & 3) {
                    printf("Unalignment address 0x%08x to read at PC 0x%08x\n",
                            address, rv->pc);
                    return TRAP_LD_ALIGN;
                }
                len = 4;
                break;
            default:
                printf("Illegal load instruction at PC 0x%08x\n", rv->pc);
                return TRAP_INST_ILL;
        }

        if (!srv32_read_mem(rv, address, len, (void*)&data)) {
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
                case MMIO_FROMHOST:
                    data = srv32_fromhost(rv);
                    break;
                case MMIO_MTIME:
                    counter.c = rv->csr.mtime.c - 1;
                    data = counter.d.lo;
                    break;
                case MMIO_MTIME+4:
                    counter.c = rv->csr.mtime.c - 1;
                    data = counter.d.hi;
                    break;
                case MMIO_MTIMECMP:
                    //rv->csr.mip = rv->csr.mip & ~(1 << MTIP);
                    data = rv->csr.mtimecmp.d.lo;
                    break;
                case MMIO_MTIMECMP+4:
                    //rv->csr.mip = rv->csr.mip & ~(1 << MTIP);
                    data = rv->csr.mtimecmp.d.hi;
                    break;
                case MMIO_MSIP:
                    data = rv->csr.msip;
                    break;
                default:
                    printf("Unknown address 0x%08x to read at PC 0x%08x\n",
                           address, rv->pc);
                    return TRAP_LD_FAIL;
            }
        }

        switch(op) {
            case OP_LB:
                if (data & 0x80) data |= 0xffffff00;
                break;
            case OP_LH:
                if (data & 0x8000) data |= 0xffff0000;
                break;
            case OP_LBU:
            case OP_LHU:
            case OP_LW:
            default: // Illegal instruction. This has been checked in the beginning.
                break;
        }
        *val = data;
    }

    if (type == OP_STORE) {
        int data = *val;
        int len  = 0;
        int mask = (op == OP_SB) ? 0xff :
                   (op == OP_SH) ? 0xffff :
                   (op == OP_SW) ? 0xffffffff :
                   0xffffffff;

        switch(op) {
            case OP_SB:
                len = 1;
                break;
            case OP_SH:
                if (address&1) {
                   printf("Unalignment address 0x%08x to write at PC 0x%08x\n",
                           address, rv->pc);
                   return TRAP_ST_ALIGN;
                }
                len = 2;
                break;
            case OP_SW:
                if (address&3) {
                   printf("Unalignment address 0x%08x to write at PC 0x%08x\n",
                           address, rv->pc);
                   return TRAP_ST_ALIGN;
                }
                len = 4;
                break;
            default:
                printf("Illegal store instruction at PC 0x%08x\n", rv->pc);
                return TRAP_INST_ILL;
        }

        if (!srv32_write_mem(rv, address, len, (void*)&data)) {
            switch(address) {
                case MMIO_PUTC:
                    putchar((char)data);
                    fflush(stdout);
                    break;
                case MMIO_GETC:
                    break;
                case MMIO_EXIT:
                    TRACE_LOG " write 0x%08x <= 0x%08x\n",
                              address, (data & mask)
                    TRACE_END;
                    prog_exit(rv, data);
                    break;
                case MMIO_TOHOST:
                    {
                        int htif_mem = 0;
                        srv32_read_mem(rv, data, sizeof(int), (void*)&htif_mem);
                        if (htif_mem == SYS_EXIT) {
                            TRACE_LOG " write 0x%08x <= 0x%08x\n",
                                      address, (data & mask)
                            TRACE_END;
                        }
                    }
                    srv32_tohost(rv, (int32_t)data);
                    break;
                case MMIO_MTIME:
                    rv->csr.mtime.d.lo = (rv->csr.mtime.d.lo & ~mask) | data;
                    rv->csr.mtime.c--;
                    mtime_update = 1;
                    break;
                case MMIO_MTIME+4:
                    rv->csr.mtime.d.hi = (rv->csr.mtime.d.hi & ~mask) | data;
                    rv->csr.mtime.c--;
                    mtime_update = 1;
                    break;
                case MMIO_MTIMECMP:
                    rv->csr.mtimecmp.d.lo = (rv->csr.mtimecmp.d.lo & ~mask) | data;
                    break;
                case MMIO_MTIMECMP+4:
                    rv->csr.mtimecmp.d.hi = (rv->csr.mtimecmp.d.hi & ~mask) | data;
                    break;
                case MMIO_MSIP:
                    rv->csr.msip = (rv->csr.msip & ~mask) | data;
                    break;
                default:
                    printf("Unknown address 0x%08x to write at PC 0x%08x\n",
                           address, rv->pc);
                    return TRAP_ST_FAIL;
            }
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    int i;
    struct rv *rv = NULL;
    char *file = NULL;
    char *tfile = NULL;

    #ifdef GDBSTUB
    int gdbport = 0;
    #endif

    const char *optstring = "hdg:b:pl:qm:n:s";
    int c;
    struct option opts[] = {
        {"help", 0, NULL, 'h'},
        {"debug", 0, NULL, 'd'},
        {"gdb", 1, NULL, 'g'},
        {"branch", 1, NULL, 'b'},
        {"predict", 0, NULL, 'p'},
        {"log", 1, NULL, 'l'},
        {"quiet", 0, NULL, 'q'},
        {"membase", 1, NULL, 'm'},
        {"memsize", 1, NULL, 'n'},
        {"single", 0, NULL, 's'}
    };

    if ((rv = (struct rv*)aligned_malloc(sizeof(int), sizeof(struct rv))) == NULL) {
        // LCOV_EXCL_START
        printf("malloc fail\n");
        exit(1);
        // LCOV_EXCL_STOP
    }

    // clear rv data structure
    memset(rv, 0, sizeof(struct rv));

    // set default value
    rv->branch_penalty = BRANCH_PENALTY;
    rv->mem_size = (MEMSIZE)*2*1024; // default memory size
    rv->mem_base = MEMBASE;

    while((c = getopt_long(argc, argv, optstring, opts, NULL)) != -1) {
        switch(c) {
            case 'h':
                usage();
                return 1;
            case 'g':
                #ifdef GDBSTUB
                gdbport = atoi(optarg);
                #else
                fprintf(stderr, "gdbstub does not support.\n");
                return 1;
                #endif
                break;
            case 'd':
                rv->debug_en = 1;
                break;
            case 'b':
                rv->branch_penalty = atoi(optarg);
                break;
            case 'p':
                rv->branch_predict = 1;
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
                sscanf(optarg, "%i", (int32_t*)&rv->mem_base);
                break;
            case 'n':
                sscanf(optarg, "%i", (int32_t*)&rv->mem_size);
                // assume instruction and data RAM are the same size
                // the total size is mem_size * 2 * 1024
                rv->mem_size *= (2*1024);
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
        if ((rv->ft=fopen(tfile, "w")) == NULL) {
            // LCOV_EXCL_START
            printf("can not open file %s\n", tfile);
            exit(1);
            // LCOV_EXCL_STOP
        }
    }

    if ((rv->mem = (int*)aligned_malloc(sizeof(int), rv->mem_size)) == NULL) {
        // LCOV_EXCL_START
        printf("malloc fail\n");
        exit(1);
        // LCOV_EXCL_STOP
    }

    // clear the data memory
    memset(rv->mem, 0, rv->mem_size);

    // load elf file
    if (!elfloader(file, rv)) {
        // LCOV_EXCL_START
        printf("Can not read elf file %s\n", file);
        exit(1);
        // LCOV_EXCL_STOP
    }

    // Registers initialize
    for(i=0; i<REGNUM; i++) {
        srv32_write_regs(rv, i, 0);
    }

    rv->csr.mvendorid  = MVENDORID;
    rv->csr.marchid    = MARCHID;
    rv->csr.mimpid     = MIMPID;
    rv->csr.mhartid    = MHARTID;
    rv->csr.misa       = MISA;
    rv->csr.time.c     = 0;
    rv->csr.cycle.c    = 0;
    rv->csr.instret.c  = 0;
    rv->csr.mtime.c    = 0;
    rv->csr.mtimecmp.c = 0;
    rv->pc             = rv->mem_base;
    rv->prev_pc        = rv->pc;

    gettimeofday(&time_start, NULL);

    #ifdef GDBSTUB
    // LCOV_EXCL_START
    if (gdbport != 0) {
        static char gdbstub_port[] = "127.0.0.1:xxxxxx";

        snprintf(gdbstub_port, sizeof(gdbstub_port)-1, "127.0.0.1:%d", gdbport);
        gdbstub_port[sizeof(gdbstub_port)-1] = 0; // ensure that the string is end of NULL

        fprintf(stderr, "start gdbstub at %s...\n", gdbstub_port);

        if (!gdbstub_init(&rv->gdbstub, &gdbstub_ops,
                  (arch_info_t){
                      .reg_num = REGNUM+1,
                      .reg_byte = 4,
                      .target_desc = TARGET_RV32,
                  }, gdbstub_port)) {
            fprintf(stderr, "Fail to create socket.\n");
            goto main_exit;
        }

        if (!gdbstub_run(&rv->gdbstub, (void *) rv))
            goto main_exit;

        gdbstub_close(&rv->gdbstub);

        goto main_exit;
    }
    // LCOV_EXCL_STOP
    #endif // GDBSTUB

    // Execution loop
    while(1) {
        if (rv->debug_en)
            debug(rv);
        srv32_step(rv);
    }

main_exit:
    aligned_free(rv->mem);
    if (rv->ft) fclose(rv->ft);
    aligned_free(rv);
}

void srv32_step(struct rv *rv) {
    int compressed = 0;
#ifdef RV32C_ENABLED
    static int compressed_prev = 0;
#endif // RV32C_ENABLED
    static int timer_irq = 0;
    static int sw_irq = 0;
    static int sw_irq_next = 0;
    static int ext_irq = 0;
    static int ext_irq_next = 0;

    INST inst;

#ifdef RV32C_ENABLED
    INSTC instc;
    int illegal;

    illegal = 0;
#endif // RV32C_ENABLED

    mtime_update = 0;

    // keep x0 always zero
    srv32_write_regs(rv, 0, 0);

    if (timer_irq && (rv->csr.mstatus & (1 << MIE))) {
        INT(INT_MTIME, MTIP);
    }

    // software interrupt
    if (sw_irq_next && (rv->csr.mstatus & (1 << MIE))) {
        INT(INT_MSI, MSIP);
    }

    // external interrupt
    if (ext_irq_next && (rv->csr.mstatus & (1 << MIE))) {
        INT(INT_MEI, MEIP);
    }

    if (rv->pc >= rv->mem_base + rv->mem_size || rv->pc < rv->mem_base) {
        printf("PC 0x%08x out of range\n", rv->pc);
        TRAP(TRAP_INST_FAIL, rv->pc);
    }

#ifdef RV32C_ENABLED
    if ((rv->pc & 1) != 0) {
        printf("PC 0x%08x alignment error\n", rv->pc);
        TRAP(TRAP_INST_ALIGN, rv->pc);
    }
#else
    if ((rv->pc & 3) != 0) {
        printf("PC 0x%08x alignment error\n", rv->pc);
        TRAP(TRAP_INST_ALIGN, rv->pc);
    }
#endif // RV32C_ENABLED

    srv32_read_mem(rv, rv->pc, sizeof(int32_t), (void*)&inst.inst);

#ifdef RV32C_ENABLED
    srv32_read_mem(rv, rv->pc, sizeof(int16_t), (void*)&instc.inst);
#endif // RV32C_ENABLED

    if ((rv->csr.mtime.c >= rv->csr.mtimecmp.c) &&
        (rv->csr.mstatus & (1 << MIE)) && (rv->csr.mie & (1 << MTIE)) &&
        (inst.r.op != OP_SYSTEM)) { // do not interrupt when system call and CSR R/W
        timer_irq = 1;
    } else {
        timer_irq = 0;
    }

    if (sw_irq &&
        (rv->csr.mstatus & (1 << MIE)) && (rv->csr.mie & (1 << MSIE)) &&
        (inst.r.op != OP_SYSTEM)) { // do not interrupt when system call and CSR R/W
        sw_irq_next = 1;
    } else {
        sw_irq_next = 0;
    }
    sw_irq = (rv->csr.msip & (1<<0)) ? 1 : 0;

    if (ext_irq &&
        (rv->csr.mstatus & (1 << MIE)) && (rv->csr.mie & (1 << MEIE)) &&
        (inst.r.op != OP_SYSTEM)) { // do not interrupt when system call and CSR R/W
        ext_irq_next = 1;
    } else {
        ext_irq_next = 0;
    }
    ext_irq = (rv->csr.msip & (1<<16)) ? 1 : 0;

    rv->csr.time.c++;
    rv->csr.instret.c++;
    CYCLE_ADD(1);

    rv->prev_pc = rv->pc;

#ifdef RV32C_ENABLED
    compressed = compressed_decoder(instc, &inst, &illegal);

    // one more cycle when the instruction type changes
    if (compressed_prev != compressed) {
        CYCLE_ADD(1);
        overhead++;
    }

    compressed_prev = compressed;

    if (compressed && 0)
        TRACE_LOG "           Translate 0x%04x => 0x%08x\n", (uint16_t)instc.inst, inst.inst
        TRACE_END;

    if (illegal) {
        TRAP(TRAP_INST_ILL, (int)instc.inst);
        return;
    }
#endif // RV32C_ENABLED

    switch(inst.r.op) {
        case OP_AUIPC: { // U-Type
            srv32_write_regs(rv, inst.u.rd, rv->pc + to_imm_u(inst.u.imm));
            TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n", rv->pc, inst.inst,
                       inst.u.rd, regname[inst.u.rd], srv32_read_regs(rv, inst.u.rd)
            TRACE_END;
            break;
        }
        case OP_LUI: { // U-Type
            srv32_write_regs(rv, inst.u.rd, to_imm_u(inst.u.imm));
            TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n", rv->pc, inst.inst,
                      inst.u.rd, regname[inst.u.rd], srv32_read_regs(rv, inst.u.rd)
            TRACE_END;
            break;
        }
        case OP_JAL: { // J-Type
            int pc_old = rv->pc;
            int pc_off = to_imm_j(inst.j.imm);

            TIME_LOG; TRACE_LOG "%08x %08x", rv->pc, inst.inst
            TRACE_END;

            rv->pc += pc_off;

            // LCOV_EXCL_START
            if (pc_off == 0) {
                printf("Warning: forever loop detected at PC 0x%08x\n", rv->pc);
                prog_exit(rv, 1);
            }
            // LCOV_EXCL_STOP

            rv->pc = rv->pc & ~1; // setting the least-signicant bit of the result to zero

            #ifndef RV32C_ENABLED
            if ((rv->pc & 3) != 0) {
                // Instruction address misaligned
                TRACE_LOG "\n" TRACE_END;
                return;
            }
            #endif // RV32C_ENABLED

            srv32_write_regs(rv, inst.j.rd, compressed ? pc_old + 2 : pc_old + 4);
            TRACE_LOG " x%02u (%s) <= 0x%08x\n",
                      inst.j.rd, regname[inst.j.rd], srv32_read_regs(rv, inst.j.rd)
            TRACE_END;

            CYCLE_ADD(rv->branch_penalty);
            return;
        }
        case OP_JALR: { // I-Type
            int pc_old = rv->pc;
            int pc_new = srv32_read_regs(rv, inst.i.rs1) + to_imm_i(inst.i.imm);

            TIME_LOG; TRACE_LOG "%08x %08x", rv->pc, inst.inst
            TRACE_END;

            rv->pc = pc_new;

            // LCOV_EXCL_START
            if (pc_new == pc_old) {
                TRACE_LOG "\n" TRACE_END;
                printf("Warning: forever loop detected at PC 0x%08x\n", rv->pc);
                prog_exit(rv, 1);
            }
            // LCOV_EXCL_STOP

            rv->pc = rv->pc & ~1; // setting the least-signicant bit of the result to zero

            #ifndef RV32C_ENABLED
            if ((rv->pc & 3) != 0) {
                // Instruction address misaligned
                TRACE_LOG "\n" TRACE_END;
                return;
            }
            #endif // RV32C_ENABLED

            srv32_write_regs(rv, inst.i.rd, compressed ? pc_old + 2 : pc_old + 4);
            TRACE_LOG " x%02u (%s) <= 0x%08x\n",
                      inst.i.rd, regname[inst.i.rd], srv32_read_regs(rv, inst.i.rd)
            TRACE_END;

            CYCLE_ADD(rv->branch_penalty);
            return;
        }
        case OP_BRANCH: { // B-Type
            TIME_LOG; TRACE_LOG "%08x %08x\n", rv->pc, inst.inst
            TRACE_END;
            int offset = to_imm_b(inst.b.imm2, inst.b.imm1);
            switch(inst.b.func3) {
                case OP_BEQ:
                    if (srv32_read_regs(rv, inst.b.rs1) == srv32_read_regs(rv, inst.b.rs2)) {
                        rv->pc += offset;
                        if ((!rv->branch_predict || offset > 0) && (rv->pc & 3) == 0)
                            CYCLE_ADD(rv->branch_penalty);
                        return;
                    }
                    break;
                case OP_BNE:
                    if (srv32_read_regs(rv, inst.b.rs1) != srv32_read_regs(rv, inst.b.rs2)) {
                        rv->pc += offset;
                        if ((!rv->branch_predict || offset > 0) && (rv->pc & 3) == 0)
                            CYCLE_ADD(rv->branch_penalty);
                        return;
                    }
                    break;
                case OP_BLT:
                    if (srv32_read_regs(rv, inst.b.rs1) < srv32_read_regs(rv, inst.b.rs2)) {
                        rv->pc += offset;
                        if ((!rv->branch_predict || offset > 0) && (rv->pc & 3) == 0)
                            CYCLE_ADD(rv->branch_penalty);
                        return;
                    }
                    break;
                case OP_BGE:
                    if (srv32_read_regs(rv, inst.b.rs1) >= srv32_read_regs(rv, inst.b.rs2)) {
                        rv->pc += offset;
                        if ((!rv->branch_predict || offset > 0) && (rv->pc & 3) == 0)
                            CYCLE_ADD(rv->branch_penalty);
                        return;
                    }
                    break;
                case OP_BLTU:
                    if (((uint32_t)srv32_read_regs(rv, inst.b.rs1)) <
                        ((uint32_t)srv32_read_regs(rv, inst.b.rs2))) {
                        rv->pc += offset;
                        if ((!rv->branch_predict || offset > 0) && (rv->pc & 3) == 0)
                            CYCLE_ADD(rv->branch_penalty);
                        return;
                    }
                    break;
                case OP_BGEU:
                    if (((uint32_t)srv32_read_regs(rv, inst.b.rs1)) >=
                        ((uint32_t)srv32_read_regs(rv, inst.b.rs2))) {
                        rv->pc += offset;
                        if ((!rv->branch_predict || offset > 0) && (rv->pc & 3) == 0)
                            CYCLE_ADD(rv->branch_penalty);
                        return;
                    }
                    break;
                default:
                    printf("Illegal branch instruction at PC 0x%08x\n", rv->pc);
                    TRAP(TRAP_INST_ILL, inst.inst);
                    return;
            }
            break;
        }
        case OP_LOAD: { // I-Type
            int32_t data;
            int32_t address = srv32_read_regs(rv, inst.i.rs1) + to_imm_i(inst.i.imm);

            TIME_LOG; TRACE_LOG "%08x %08x", rv->pc, inst.inst
            TRACE_END;

            int result = memrw(rv, OP_LOAD, inst.i.func3, address, &data);

            if (singleram) CYCLE_ADD(1);

            switch(result) {
                case TRAP_LD_FAIL:
                     TRACE_LOG "\n" TRACE_END;
                     TRAP(TRAP_LD_FAIL, address);
                     return;
                case TRAP_LD_ALIGN:
                     TRACE_LOG "\n" TRACE_END;
                     TRAP(TRAP_LD_ALIGN, address);
                     return;
                case TRAP_INST_ILL:
                     TRACE_LOG " read 0x%08x, x%02u (%s) <= 0x%08x\n",
                                 address, inst.i.rd,
                                 regname[inst.i.rd], 0
                     TRACE_END;
                     TRAP(TRAP_INST_ILL, inst.inst);
                     return;
            }

            srv32_write_regs(rv, inst.i.rd, data);
            TRACE_LOG " read 0x%08x, x%02u (%s) <= 0x%08x\n",
                      address, inst.i.rd,
                      regname[inst.i.rd], srv32_read_regs(rv, inst.i.rd)
            TRACE_END;
            break;
        }
        case OP_STORE: { // S-Type
            int address = srv32_read_regs(rv, inst.s.rs1) +
                          to_imm_s(inst.s.imm2, inst.s.imm1);
            int data = srv32_read_regs(rv, inst.s.rs2);

            int mask = (inst.i.func3 == OP_SB) ? 0xff :
                       (inst.i.func3 == OP_SH) ? 0xffff :
                       (inst.i.func3 == OP_SW) ? 0xffffffff :
                       0xffffffff;

            TIME_LOG; TRACE_LOG "%08x %08x", rv->pc, inst.inst
            TRACE_END;

            int result = memrw(rv, OP_STORE, inst.i.func3, address, &data);

            if (singleram) CYCLE_ADD(1);

            switch(result) {
                case TRAP_ST_FAIL:
                     TRACE_LOG "\n" TRACE_END;
                     TRAP(TRAP_ST_FAIL, address);
                     return;
                case TRAP_ST_ALIGN:
                     TRACE_LOG "\n" TRACE_END;
                     TRAP(TRAP_ST_ALIGN, address);
                     return;
                case TRAP_INST_ILL:
                     TRACE_LOG "\n" TRACE_END;
                     TRAP(TRAP_INST_ILL, inst.inst);
                     return;
            }

            TRACE_LOG " write 0x%08x <= 0x%08x\n", address, (data & mask)
            TRACE_END;

            break;
        }
        case OP_ARITHI: { // I-Type
            switch(inst.i.func3) {
                case OP_ADD:
                    srv32_write_regs(rv, inst.i.rd,
                            srv32_read_regs(rv, inst.i.rs1) + to_imm_i(inst.i.imm));
                    break;
                case OP_SLT:
                    srv32_write_regs(rv, inst.i.rd,
                            srv32_read_regs(rv, inst.i.rs1) < to_imm_i(inst.i.imm) ? 1 : 0);
                    break;
                case OP_SLTU:
                    //FIXME: to pass compliance test, the IMM should be singed
                    //extension, and compare with unsigned.
                    //srv32_write_regs(rv, inst.i.rd,
                    //      ((uint32_t)srv32_read_regs(rv, inst.i.rs1)) <
                    //      ((uint32_t)to_imm_iu(inst.i.imm)) ? 1 : 0);
                    srv32_write_regs(rv, inst.i.rd,
                            ((uint32_t)srv32_read_regs(rv, inst.i.rs1)) <
                            ((uint32_t)to_imm_i(inst.i.imm)) ? 1 : 0);
                    break;
                case OP_XOR:
                    srv32_write_regs(rv, inst.i.rd,
                            srv32_read_regs(rv, inst.i.rs1) ^ to_imm_i(inst.i.imm));
                    break;
                case OP_OR:
                    srv32_write_regs(rv, inst.i.rd,
                            srv32_read_regs(rv, inst.i.rs1) | to_imm_i(inst.i.imm));
                    break;
                case OP_AND:
                    srv32_write_regs(rv, inst.i.rd,
                            srv32_read_regs(rv, inst.i.rs1) & to_imm_i(inst.i.imm));
                    break;
                case OP_SLL:
                    switch (inst.r.func7) {
                        case FN_RV32I:
                            srv32_write_regs(rv, inst.i.rd,
                                srv32_read_regs(rv, inst.i.rs1) << (inst.i.imm&0x1f));
                            break;
                        #ifdef RV32B_ENABLED
                        case FN_BSET:
                            srv32_write_regs(rv, inst.i.rd,
                                srv32_read_regs(rv, inst.i.rs1) | (1 << (inst.i.imm&0x1f)));
                            break;
                        case FN_BCLR:
                            srv32_write_regs(rv, inst.i.rd,
                                srv32_read_regs(rv, inst.i.rs1) & ~(1 << (inst.i.imm&0x1f)));
                            break;
                        case FN_CLZ:
                            switch (inst.r.rs2) {
                                case 0: // CLZ
                                    {
                                        int32_t r = 0;
                                        int32_t x = srv32_read_regs(rv, inst.i.rs1);
                                        if (!x) {
                                            r = 32;
                                        } else {
                                            if (!(x & 0xffff0000)) { x <<= 16; r += 16; }
                                            if (!(x & 0xff000000)) { x <<=  8; r +=  8; }
                                            if (!(x & 0xf0000000)) { x <<=  4; r +=  4; }
                                            if (!(x & 0xc0000000)) { x <<=  2; r +=  2; }
                                            if (!(x & 0x80000000)) {           r +=  1; }
                                        }
                                        srv32_write_regs(rv, inst.i.rd, r);
                                    }
                                    break;
                                case 2: // CPOP
                                    {
                                        uint32_t c = 0;
                                        int32_t n = srv32_read_regs(rv, inst.i.rs1);
                                        while (n) {
                                            n &= (n - 1);
                                            c++;
                                        }
                                        srv32_write_regs(rv, inst.i.rd, c);
                                    }
                                    break;
                                case 1: // CTZ
                                    {
                                        int32_t x = srv32_read_regs(rv, inst.i.rs1);
                                        static const uint8_t table[32] = {
                                          0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
                                          31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
                                        };
                                        int32_t n = (!x) ? 32 :
                                            (int32_t)table[((uint32_t)((x & -x) * 0x077CB531U)) >> 27];
                                        srv32_write_regs(rv, inst.i.rd, n);
                                    }
                                    break;
                                case 4: // SEXT.B
                                    {
                                        uint32_t n = srv32_read_regs(rv, inst.i.rs1) & 0xff;
                                        if (n&0x80)
                                            n |= 0xffffff00;
                                        srv32_write_regs(rv, inst.i.rd, n);
                                    }
                                    break;
                                case 5: // SEXT.H
                                    {
                                        uint32_t n = srv32_read_regs(rv, inst.i.rs1) & 0xffff;
                                        if (n&0x8000)
                                            n |= 0xffff0000;
                                        srv32_write_regs(rv, inst.i.rd, n);
                                    }
                                    break;
                                default:
                                    printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                                    TRAP(TRAP_INST_ILL, inst.inst);
                                    return;
                            }
                            break;
                        case FN_BINV:
                            srv32_write_regs(rv, inst.i.rd,
                                srv32_read_regs(rv, inst.i.rs1) ^ (1 << (inst.i.imm&0x1f)));
                            break;
                        #endif // RV32B_ENABLED
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            return;
                    }
                    break;
                case OP_SR:
                    switch (inst.r.func7) {
                        case FN_SRL: // SRLI
                            srv32_write_regs(rv, inst.i.rd,
                                ((uint32_t)srv32_read_regs(rv, inst.i.rs1)) >> (inst.i.imm&0x1f));
                            break;
                        case FN_SRA: // SRAI
                            srv32_write_regs(rv, inst.i.rd,
                                srv32_read_regs(rv, inst.i.rs1) >> (inst.i.imm&0x1f));
                            break;
                        #ifdef RV32B_ENABLED
                        case FN_BSET:
                            if (inst.r.rs2 == 7) { // ORC.B
                                int32_t n = 0;
                                int32_t v = srv32_read_regs(rv, inst.i.rs1);
                                if (v & 0x000000ff) n |= 0x000000ff;
                                if (v & 0x0000ff00) n |= 0x0000ff00;
                                if (v & 0x00ff0000) n |= 0x00ff0000;
                                if (v & 0xff000000) n |= 0xff000000;
                                srv32_write_regs(rv, inst.i.rd, n);
                            } else {
                                printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                                TRAP(TRAP_INST_ILL, inst.inst);
                                return;
                            }
                            break;
                        case FN_BCLR: // BCLRI
                            srv32_write_regs(rv, inst.i.rd,
                                (srv32_read_regs(rv, inst.i.rs1) >> (inst.i.imm&0x1f)) & 1);
                            break;
                        case FN_CLZ: // RORI
                            {
                                uint32_t n = srv32_read_regs(rv, inst.i.rs1);
                                srv32_write_regs(rv, inst.i.rd, (n >> (inst.i.imm&0x1f)) |
                                                  (n << (32 - (inst.i.imm&0x1f))));
                            }
                            break;
                        case FN_REV:
                            switch(inst.i.imm&0x1f) {
                                case 0x18: // REV.8
                                    {
                                        uint32_t n = srv32_read_regs(rv, inst.i.rs1);
                                        srv32_write_regs(rv, inst.i.rd,
                                               ((n >> 24) & 0x000000ff) |
                                               ((n >>  8) & 0x0000ff00) |
                                               ((n <<  8) & 0x00ff0000) |
                                               ((n << 24) & 0xff000000));
                                    }
                                    break;
                                default:
                                    printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                                    TRAP(TRAP_INST_ILL, inst.inst);
                                    return;
                            }
                            break;
                        #endif // RV32B_ENABLED
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            return;
                    }
                    break;
                default:
                    printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                    TRAP(TRAP_INST_ILL, inst.inst);
                    return;
            }
            TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n",
                      rv->pc, inst.inst, inst.i.rd, regname[inst.i.rd],
                      srv32_read_regs(rv, inst.i.rd)
            TRACE_END;
            break;
        }
        case OP_ARITHR: { // R-Type
            switch (inst.r.func7) {
                #ifdef RV32M_ENABLED
                case FN_RV32M: // RV32M Multiply Extension
                    switch(inst.r.func3) {
                        case OP_MUL:
                            srv32_write_regs(rv, inst.r.rd, srv32_read_regs(rv, inst.r.rs1) *
                                              srv32_read_regs(rv, inst.r.rs2));
                            break;
                        case OP_MULH:
                            {
                            union {
                                int64_t l;
                                struct { int32_t l, h; } n;
                            } a, b, r;
                            a.l = (int64_t)srv32_read_regs(rv, inst.r.rs1);
                            b.l = (int64_t)srv32_read_regs(rv, inst.r.rs2);
                            r.l = a.l * b.l;
                            srv32_write_regs(rv, inst.r.rd, r.n.h);
                            }
                            break;
                        case OP_MULSU:
                            {
                            union {
                                int64_t l;
                                struct { int32_t l, h; } n;
                            } a, b, r;
                            a.l = (int64_t)srv32_read_regs(rv, inst.r.rs1);
                            b.n.l = srv32_read_regs(rv, inst.r.rs2);
                            b.n.h = 0;
                            r.l = a.l * b.l;
                            srv32_write_regs(rv, inst.r.rd, r.n.h);
                            }
                            break;
                        case OP_MULU:
                            {
                            union {
                                int64_t l;
                                struct { int32_t l, h; } n;
                            } a, b, r;
                            a.n.l = srv32_read_regs(rv, inst.r.rs1); a.n.h = 0;
                            b.n.l = srv32_read_regs(rv, inst.r.rs2); b.n.h = 0;
                            r.l = ((uint64_t)a.l) *
                                  ((uint64_t)b.l);
                            srv32_write_regs(rv, inst.r.rd, r.n.h);
                            }
                            break;
                        case OP_DIV:
                            if (srv32_read_regs(rv, inst.r.rs2))
                                srv32_write_regs(rv, inst.r.rd,
                                    (int32_t)(((int64_t)srv32_read_regs(rv, inst.r.rs1)) /
                                    srv32_read_regs(rv, inst.r.rs2)));
                            else
                                srv32_write_regs(rv, inst.r.rd, 0xffffffff);
                            break;
                        case OP_DIVU:
                            if (srv32_read_regs(rv, inst.r.rs2))
                                srv32_write_regs(rv, inst.r.rd,
                                    (int32_t)(((uint32_t)srv32_read_regs(rv, inst.r.rs1)) /
                                    ((uint32_t)srv32_read_regs(rv, inst.r.rs2))));
                            else
                                srv32_write_regs(rv, inst.r.rd, 0xffffffff);
                            break;
                        case OP_REM:
                            if (srv32_read_regs(rv, inst.r.rs2))
                                srv32_write_regs(rv, inst.r.rd,
                                    (int32_t)(((int64_t)srv32_read_regs(rv, inst.r.rs1)) %
                                    srv32_read_regs(rv, inst.r.rs2)));
                            else
                                srv32_write_regs(rv, inst.r.rd, srv32_read_regs(rv, inst.r.rs1));
                            break;
                        case OP_REMU:
                            if (srv32_read_regs(rv, inst.r.rs2))
                                srv32_write_regs(rv, inst.r.rd,
                                    (int32_t)(((uint32_t)srv32_read_regs(rv, inst.r.rs1)) %
                                    ((uint32_t)srv32_read_regs(rv, inst.r.rs2))));
                            else
                                srv32_write_regs(rv, inst.r.rd, srv32_read_regs(rv, inst.r.rs1));
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            return;
                    }
                break;
                #endif // RV32M_ENABLED

                case FN_RV32I:
                    switch(inst.r.func3) {
                        case OP_ADD:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) + srv32_read_regs(rv, inst.r.rs2));
                            break;
                        case OP_SLL:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) << srv32_read_regs(rv, inst.r.rs2));
                            break;
                        case OP_SLT:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) < srv32_read_regs(rv, inst.r.rs2) ?
                                1 : 0);
                            break;
                        case OP_SLTU:
                            srv32_write_regs(rv, inst.r.rd,
                                ((uint32_t)srv32_read_regs(rv, inst.r.rs1)) <
                                ((uint32_t)srv32_read_regs(rv, inst.r.rs2)) ? 1 : 0);
                            break;
                        case OP_XOR:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) ^ srv32_read_regs(rv, inst.r.rs2));
                            break;
                        case OP_SR:
                            srv32_write_regs(rv, inst.r.rd,
                                ((uint32_t)srv32_read_regs(rv, inst.r.rs1)) >>
                                srv32_read_regs(rv, inst.r.rs2));
                            break;
                        case OP_OR:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) | srv32_read_regs(rv, inst.r.rs2));
                            break;
                        case OP_AND:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) & srv32_read_regs(rv, inst.r.rs2));
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            return;
                    }
                break;

                case FN_ANDN:
                    switch(inst.r.func3) {
                        case OP_ADD: // SUB
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) - srv32_read_regs(rv, inst.r.rs2));
                            break;
                        case OP_SR: // SRA
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) >> srv32_read_regs(rv, inst.r.rs2));
                            break;
                        #ifdef RV32B_ENABLED
                        case OP_AND: // ANDN
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) & ~(srv32_read_regs(rv, inst.r.rs2)));
                            break;
                        case OP_OR: // ORN
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) | ~(srv32_read_regs(rv, inst.r.rs2)));
                            break;
                        case OP_XOR: // XNOR
                            srv32_write_regs(rv, inst.r.rd,
                                ~(srv32_read_regs(rv, inst.r.rs1) ^ srv32_read_regs(rv, inst.r.rs2)));
                            break;
                        #endif // RV32B_ENABLED
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            return;
                    }
                    break;

                #ifdef RV32B_ENABLED
                case FN_ZEXT:
                    srv32_write_regs(rv, inst.r.rd, srv32_read_regs(rv, inst.r.rs1) & 0xffff);
                    break;

                case FN_MINMAX:
                    switch(inst.r.func3) {
                        case OP_CLMUL:
                            {
                                int32_t a = srv32_read_regs(rv, inst.r.rs1);
                                int32_t b = srv32_read_regs(rv, inst.r.rs2);
                                int32_t n = 0;

                                for(int i = 0; i <= 31; i++)
                                    if ((b >> i) & 1) n ^= (a << i);

                                srv32_write_regs(rv, inst.r.rd, n);
                            }
                            break;
                        case OP_CLMULH:
                            {
                                uint32_t a = srv32_read_regs(rv, inst.r.rs1);
                                uint32_t b = srv32_read_regs(rv, inst.r.rs2);
                                int32_t n = 0;

                                for(int i = 1; i < 32; i++)
                                    if ((b >> i) & 1) n ^= (a >> (32 - i));

                                srv32_write_regs(rv, inst.r.rd, n);
                            }
                            break;
                        case OP_CLMULR:
                            {
                                uint32_t a = srv32_read_regs(rv, inst.r.rs1);
                                uint32_t b = srv32_read_regs(rv, inst.r.rs2);
                                int32_t n = 0;

                                for(int i = 0; i < 32; i++)
                                    if ((b >> i) & 1) n ^= (a >> (32 - i - 1));

                                srv32_write_regs(rv, inst.r.rd, n);
                            }
                            break;
                        case OP_MAX:
                            {
                                int32_t a = srv32_read_regs(rv, inst.r.rs1);
                                int32_t b = srv32_read_regs(rv, inst.r.rs2);
                                srv32_write_regs(rv, inst.r.rd, a > b ? a : b);
                            }
                            break;
                        case OP_MAXU:
                            {
                                uint32_t a = srv32_read_regs(rv, inst.r.rs1);
                                uint32_t b = srv32_read_regs(rv, inst.r.rs2);
                                srv32_write_regs(rv, inst.r.rd, a > b ? a : b);
                            }
                            break;
                        case OP_MIN:
                            {
                                int32_t a = srv32_read_regs(rv, inst.r.rs1);
                                int32_t b = srv32_read_regs(rv, inst.r.rs2);
                                srv32_write_regs(rv, inst.r.rd, a < b ? a : b);
                            }
                            break;
                        case OP_MINU:
                            {
                                uint32_t a = srv32_read_regs(rv, inst.r.rs1);
                                uint32_t b = srv32_read_regs(rv, inst.r.rs2);
                                srv32_write_regs(rv, inst.r.rd, a < b ? a : b);
                            }
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            return;
                    }
                    break;

                case FN_SHADD:
                    switch(inst.r.func3) {
                        case OP_SH1ADD:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs2) +
                                (srv32_read_regs(rv, inst.r.rs1) << 1));
                            break;
                        case OP_SH2ADD:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs2) +
                                (srv32_read_regs(rv, inst.r.rs1) << 2));
                            break;
                        case OP_SH3ADD:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs2) +
                                (srv32_read_regs(rv, inst.r.rs1) << 3));
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            return;
                    }
                    break;

                case FN_BSET:
                    srv32_write_regs(rv, inst.r.rd,
                        srv32_read_regs(rv, inst.r.rs1) |
                        (1 << (srv32_read_regs(rv, inst.r.rs2) & 0x1f)));
                    break;

                case FN_BCLR:
                    switch(inst.r.func3) {
                        case OP_BCLR:
                            srv32_write_regs(rv, inst.r.rd,
                                srv32_read_regs(rv, inst.r.rs1) &
                                ~(1 << (srv32_read_regs(rv, inst.r.rs2) & 0x1f)));
                            break;
                        case OP_BEXT:
                            srv32_write_regs(rv, inst.r.rd,
                                (srv32_read_regs(rv, inst.r.rs1) >>
                                (srv32_read_regs(rv, inst.r.rs2) & 0x1f)) & 1);
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            return;
                    }
                    break;

                case FN_CLZ:
                    switch(inst.r.func3) {
                        case OP_ROL:
                            {
                                uint32_t n = srv32_read_regs(rv, inst.r.rs2) & 0x1f;
                                srv32_write_regs(rv, inst.r.rd,
                                    (srv32_read_regs(rv, inst.r.rs1) << n) |
                                    ((uint32_t)srv32_read_regs(rv, inst.r.rs1) >> (32 - n)));
                            }
                            break;
                        case OP_ROR:
                            {
                                uint32_t n = srv32_read_regs(rv, inst.r.rs2) & 0x1f;
                                srv32_write_regs(rv, inst.r.rd,
                                    ((uint32_t)srv32_read_regs(rv, inst.r.rs1) >> n) |
                                    (srv32_read_regs(rv, inst.r.rs1) << (32 - n)));
                            }
                            break;
                        default:
                            printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                            TRAP(TRAP_INST_ILL, inst.inst);
                            return;
                    }
                    break;

                case FN_BINV:
                    srv32_write_regs(rv, inst.r.rd,
                        srv32_read_regs(rv, inst.r.rs1) ^
                        (1 << (srv32_read_regs(rv, inst.r.rs2) & 0x1f)));
                    break;
                #endif // RV32B_ENABLED

                default:
                    printf("Unknown instruction at PC 0x%08x\n", rv->pc);
                    TRAP(TRAP_INST_ILL, inst.inst);
                    return;
            }
            TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n",
                      rv->pc, inst.inst, inst.r.rd, regname[inst.r.rd],
                      srv32_read_regs(rv, inst.r.rd)
            TRACE_END;
            break;
        }
        case OP_FENCE: {
            TIME_LOG; TRACE_LOG "%08x %08x\n", rv->pc, inst.inst
            TRACE_END;
            break;
        }
        case OP_SYSTEM: { // I-Type
            int val;
            int update;
            int csr_op = 0;
            int csr_type;
            // RDCYCLE, RDTIME and RDINSTRET are read only
            switch(inst.i.func3) {
                case OP_ECALL:
                    TIME_LOG; TRACE_LOG "%08x %08x\n", rv->pc, inst.inst
                    TRACE_END;
                    switch (inst.i.imm & 3) {
                       case 0: // ecall
                           if (1) { // syscall, to compatible FreeRTOS usage, don't use it.
                               int res;
                               res = srv32_syscall(rv,
                                                   srv32_read_regs(rv, SYS),
                                                   srv32_read_regs(rv, A0),
                                                   srv32_read_regs(rv, A1),
                                                   srv32_read_regs(rv, A2),
                                                   srv32_read_regs(rv, A3),
                                                   srv32_read_regs(rv, A4),
                                                   srv32_read_regs(rv, A5));
                               // Notes: FreeRTOS will use ecall to perform context switching.
                               // The syscall of newlib will confict with the syscall of
                               // FreeRTOS.
                               #if 0
                               // FIXME: if it is prefined syscall, excute it.
                               // otherwise raising a trap
                               if (res != -1) {
                                    srv32_write_regs(rv, A0, res);
                               } else {
                                    TRAP(TRAP_ECALL, 0);
                                    return;
                               }
                               break;
                               #else
                               if (res != -1)
                                    srv32_write_regs(rv, A0, res);
                               TRAP(TRAP_ECALL, 0);
                               return;
                               #endif
                           } else {
                               TRAP(TRAP_ECALL, 0);
                               return;
                           }
                       case 1: // ebreak
                           TRAP(TRAP_BREAK, rv->pc);
                           return;
                       case 2: // mret
                           rv->pc = rv->csr.mepc;
                           // rv->csr.mstatus.mie = rv->csr.mstatus.mpie
                           rv->csr.mstatus = (rv->csr.mstatus & (1 << MPIE)) ?
                                              (rv->csr.mstatus | (1 << MIE)) :
                                              (rv->csr.mstatus & ~(1 << MIE));
                           // rv->csr.mstatus.mpie = 1

                           #ifndef RV32C_ENABLED
                           if ((rv->pc & 3) != 0) {
                               // Instruction address misaligned
                               return;
                           }
                           #endif // RV32C_ENABLED
                           CYCLE_ADD(rv->branch_penalty);
                           return;
                       default:
                           printf("Illegal system call at PC 0x%08x\n", rv->pc);
                           TRAP(TRAP_INST_ILL, 0);
                           return;
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
                    val      = srv32_read_regs(rv, inst.i.rs1);
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
                    val      = srv32_read_regs(rv, inst.i.rs1);
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
                    val      = srv32_read_regs(rv, inst.i.rs1);
                    update   = (inst.i.rs1 == 0) ? 0 : 1;
                    csr_type = OP_CSRRC;
                    break;
                default:
                    printf("Unknown system instruction at PC 0x%08x\n", rv->pc);
                    TIME_LOG; TRACE_LOG "%08x %08x\n", rv->pc, inst.inst
                    TRACE_END;
                    TRAP(TRAP_INST_ILL, inst.inst);
                    return;
            }
            if (csr_op) {
                int legal = 0;
                int result = csr_rw(rv, inst.i.imm, csr_type, val, update, &legal);
                if (legal) {
                    srv32_write_regs(rv, inst.i.rd, result);
                }
                TIME_LOG; TRACE_LOG "%08x %08x",
                          rv->pc, inst.inst
                TRACE_END;
                if (!legal) {
                   TRACE_LOG "\n" TRACE_END;
                   TRAP(TRAP_INST_ILL, 0);
                   return;
                }
                TRACE_LOG " x%02u (%s) <= 0x%08x\n",
                          inst.i.rd,
                          regname[inst.i.rd], srv32_read_regs(rv, inst.i.rd)
                TRACE_END;
            }
            break;
        }
        default: {
            printf("Illegal instruction at PC 0x%08x\n", rv->pc);
            TIME_LOG; TRACE_LOG "%08x %08x\n", rv->pc, inst.inst
            TRACE_END;
            TRAP(TRAP_INST_ILL, inst.inst);
            return;
        }
    } // end of switch(inst.r.op)

    rv->pc = compressed ? rv->pc + 2 : rv->pc + 4;

}

