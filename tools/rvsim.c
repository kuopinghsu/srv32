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
#include <string.h>
#include <getopt.h>
#include "opcode.h"

int mem_size = 128*1024; // default memory size

#define PRINT_TIMELOG 1
#define MAXLEN      1024

#define IMEM_BASE   (0+mem_base)
#define DMEM_BASE   (mem_size+mem_base)
#define IMEM_SIZE   mem_size
#define DMEM_SIZE   mem_size

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
    int mvendorid;
    int marchid;
    int mimpid;
    int mhartid;
    int mscratch;
    int mstatus;
    int misa;
    int mie;
    int mtvec;
    int mepc;
    int mcause;
    int mip;
    int mtval;
    int msip;
#ifdef XV6_SUPPORT
    int medeleg;
    int mideleg;
    int mcounteren;
    int sstatus;
    int sie;
    int stvec;
    int sscratch;
    int sepc;
    int scause;
    int stval;
    int sip;
    int satp;
#endif
} CSR;

CSR csr;
int pc = 0;
int mode = MMODE;
int prev_pc = 0;
int mem_base = 0;
int singleram = 0;
int branch_penalty = BRANCH_PENALTY;
int mtime_update = 0;

int quiet = 0;

char *regname[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0(fp)", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

#define IVA2PA(addr) ((addr)-IMEM_BASE)
#define IPA2VA(addr) ((addr)+IMEM_BASE)
#define DVA2PA(addr) ((addr)-DMEM_BASE)
#define DPA2VA(addr) ((addr)+DMEM_BASE)

int elfread(char *file, char *imem, char *dmem, int *isize, int *dsize);
int getch(void);

void usage(void) {
    printf(
"Instruction Set Simulator for RV32IM, (c) 2020 Kuoping Hsu\n"
"Usage: rvsim [-h] [-b n] [-m n] [-n n] [-p] [-l logfile] file\n\n"
"       --help, -h              help\n"
"       --membase n, -m n       memory base (in hex)\n"
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
#endif

#ifndef __STDC_WANT_LIB_EXT1__ 
char *strncpy_s(char *dest, size_t n, const char *src, size_t count) {
    int len = (int)strnlen(src, count);
    if (len > n) len = n;
    memcpy(dest, src, len);
    dest[len] = 0;
    return dest;
}
#endif // __STDC_WANT_LIB_EXT1__

static inline int to_imm_i(unsigned int n) {
    return (int)((n & (1<<11)) ? (n | 0xfffff000) : n);
}

/*
static inline int to_imm_iu(unsigned int n) {
    return (int)(n);
}
*/

static inline int to_imm_s(unsigned int n1, unsigned int n2) {
    unsigned int n = (n1 << 5) + n2;
    return (int)((n & (1<<11)) ? (n | 0xfffff000) : n);
}

static inline int to_imm_b(unsigned int n1, unsigned int n2) {
    unsigned int m;
    union {
        unsigned int n;
        struct {
            unsigned int a0 : 1;
            unsigned int a1 : 4;
            unsigned int a2 : 6;
            unsigned int a3 : 1;
            //unsigned int a4 : 20; // no used
        } m;
    } r;
    r.n = (n1 << 5) + n2;
    m = (r.m.a3 << 12) | (r.m.a2 << 5) | (r.m.a1 << 1) | (r.m.a0 << 11);
    return (int)((m & (1<<12)) ? (m | 0xffffe000) : m);
}

static inline int to_imm_j(unsigned int n) {
    unsigned int m;
    union {
        unsigned int n;
        struct {
            unsigned int a0 : 8;
            unsigned int a1 : 1;
            unsigned int a2 : 10;
            unsigned int a3 : 1;
            // unsigned int a4 : 12; // no used
        } m;
    } r;
    r.n = n;
    m = (r.m.a3 << 20) | (r.m.a2 << 1) | (r.m.a1 << 11) | (r.m.a0 << 12);
    return (int)((m & (1<<20)) ? (m | 0xffe00000) : m);
}

static inline int to_imm_u(unsigned int n) {
    return (int)(n << 12);
}

static void prog_exit(int exitcode) {
    if (!quiet) {
        printf("\nExcuting %lld instructions, %lld cycles, %1.3f CPI\n", csr.instret.c,
               csr.cycle.c, ((float)csr.cycle.c)/csr.instret.c);
        printf("Program terminate\n");
    }
    exit(exitcode);
}

// Use libbfd to read the elf file.
#ifdef USE_LIBBFD
#define PACKAGE "riscv32-rvsim"
#define PACKAGE_VERSION "1.0"
#include <bfd.h>
static int elfread(char *file, char *imem, char *dmem, int *isize, int *dsize) {
    bfd *abfd = NULL;

    // init bfd
    bfd_init();

    // load binary file
    if ((abfd = bfd_openr(file, NULL)) == NULL) {
		printf("Failed to open file %s!\n", file);
		return 0;
    }

	// no section info is loaded unless we call bfd_check_format!:
	if (!bfd_check_format (abfd, bfd_object)) {
		printf("Failed to check file %s!\n", file);
		return 0;
    }

	asection *text = bfd_get_section_by_name (abfd, ".text");
	asection *data = bfd_get_section_by_name (abfd, ".data");

	// copy the contents of the data and executable sections into the allocated memory
	bfd_get_section_contents(abfd, text, imem, 0, text->size);
	bfd_get_section_contents(abfd, data, dmem, 0, data->size);

    *isize = text->size;
    *dsize = data->size;

    if (!quiet) printf("Load .text section %ld, .data section %ld bytes\n",
                       text->size, data->size);

    bfd_close(abfd);

    if (!text->size && !data->size) {
        printf("can not read the content of elf file\n");
        exit(-1);
    }

    return 1;
}
#endif

#define UPDATE_CSR(update,mode,reg,val) { \
    if (update) { \
        if ((mode) == OP_CSRRW) reg = (val); \
        if ((mode) == OP_CSRRS) reg = (reg) |  (val); \
        if ((mode) == OP_CSRRC) reg = (reg) & ~(val); \
    } \
}

int csr_rw(int regs, int mode, int val, int update, int *result) {
    COUNTER counter;
    switch(regs) {
        case CSR_RDCYCLE    : counter.c = csr.cycle.c - 1;
                              *result = counter.d.lo; // UPDATE_CSR(update, mode, csr.cycle.d.lo, val);
                              break;
        case CSR_RDCYCLEH   : counter.c = csr.cycle.c - 1;
                              *result = counter.d.hi; // UPDATE_CSR(update, mode, csr.cycle.d.hi, val);
                              break;
        /*
        case CSR_RDTIME     : counter.c = csr.time.c - 1;
                              *result = counter.d.lo; // UPDATE_CSR(update, mode, csr.time.d.lo, val);
                              break;
        case CSR_RDTIMEH    : counter.c = csr.time.c - 1;
                              *result = counter.d.hi; // UPDATE_CSR(update, mode, csr.time.d.hi, val);
                              break;
        */
        case CSR_RDINSTRET  : counter.c = csr.instret.c - 1;
                              *result = counter.d.lo; // UPDATE_CSR(update, mode, csr.instret.d.lo, val);
                              break;
        case CSR_RDINSTRETH : counter.c = csr.instret.c - 1;
                              *result = counter.d.hi; // UPDATE_CSR(update, mode, csr.instret.d.hi, val);
                              break;
        case CSR_MVENDORID  : *result = csr.mvendorid; // UPDATE_CSR(update, mode, csr.mvendorid, val);
                              break;
        case CSR_MARCHID    : *result = csr.marchid; // UPDATE_CSR(update, mode, csr.marchid, val);
                              break;
        case CSR_MIMPID     : *result = csr.mimpid; // UPDATE_CSR(update, mode, csr.mimpid, val);
                              break;
        case CSR_MHARTID    : *result = csr.mhartid; // UPDATE_CSR(update, mode, csr.mhartid, val);
                              break;
        case CSR_MSCRATCH   : *result = csr.mscratch; UPDATE_CSR(update, mode, csr.mscratch, val);
                              break;
        case CSR_MSTATUS    : *result = csr.mstatus; UPDATE_CSR(update, mode, csr.mstatus, val);
                              break;
        case CSR_MISA       : *result = csr.misa; UPDATE_CSR(update, mode, csr.misa, val);
                              break;
        case CSR_MIE        : *result = csr.mie; UPDATE_CSR(update, mode, csr.mie, val);
                              break;
        case CSR_MIP        : *result = csr.mip; UPDATE_CSR(update, mode, csr.mip, val);
                              break;
        case CSR_MTVEC      : *result = csr.mtvec; UPDATE_CSR(update, mode, csr.mtvec, val);
                              break;
        case CSR_MEPC       : *result = csr.mepc; UPDATE_CSR(update, mode, csr.mepc, val);
                              break;
        case CSR_MCAUSE     : *result = csr.mcause; UPDATE_CSR(update, mode, csr.mcause, val);
                              break;
        case CSR_MTVAL      : *result = csr.mtval; UPDATE_CSR(update, mode, csr.mtval, val);
                              break;
#ifdef XV6_SUPPORT
        case CSR_MEDELEG    : *result = csr.medeleg; UPDATE_CSR(update, mode, csr.medeleg, val);
                              break;
        case CSR_MIDELEG    : *result = csr.mideleg; UPDATE_CSR(update, mode, csr.mideleg, val);
                              break;
        case CSR_MCOUNTEREN : *result = csr.mcounteren; UPDATE_CSR(update, mode, csr.mcounteren, val);
                              break;
        case CSR_SSTATUS    : *result = csr.sstatus; UPDATE_CSR(update, mode, csr.sstatus, val);
                              break;
        case CSR_SIE        : *result = csr.sie; UPDATE_CSR(update, mode, csr.sie, val);
                              break;
        case CSR_STVEC      : *result = csr.stvec; UPDATE_CSR(update, mode, csr.stvec, val);
                              break;
        case CSR_SSCRATCH   : *result = csr.sscratch; UPDATE_CSR(update, mode, csr.sscratch, val);
                              break;
        case CSR_SEPC       : *result = csr.sepc; UPDATE_CSR(update, mode, csr.sepc, val);
                              break;
        case CSR_SCAUSE     : *result = csr.scause; UPDATE_CSR(update, mode, csr.scause, val);
                              break;
        case CSR_STVAL      : *result = csr.stval; UPDATE_CSR(update, mode, csr.stval, val);
                              break;
        case CSR_SIP        : *result = csr.sip; UPDATE_CSR(update, mode, csr.sip, val);
                              break;
        case CSR_SATP       : *result = csr.satp; UPDATE_CSR(update, mode, csr.satp, val);
                              break;
#endif
        default: *result = 0; // FIXME
                 printf("Unsupport CSR register 0x%03x at PC 0x%08x\n", regs, pc);
                 return 0;
    }
    return 1;
}

int compressed_decoder (
    INSTC instc,
    INST  *inst,
    int   *illegal
) {
    INST r;
    *illegal = 0;

    r.inst = 0;

    // Not compressed instructions
    if (instc.cr.op == 0x3) {
        return 0;
    }

    if (instc.cr.op == 0x0) {
        switch(instc.ci.func3) {
            case OP_CADDI4SPN : // c.addi4spn -> addi rd′, x2, nzuimm[9:2]
                if (instc.inst == 0) {
                    *illegal = 1;
                    break;
                }
                if (r.i.imm != 0) {
                    r.i.op    = OP_ARITHI;
                    r.i.rd    = 8 + instc.ciw.rd_;
                    r.i.func3 = OP_ADD;
                    r.i.rs1   = 2;
                    r.i.imm   = (((instc.ciw.imm << 2) & 0xf0) +
                                 ((instc.ciw.imm >> 4) & 0xc0) +
                                 ((instc.ciw.imm << 1) & 0x02) +
                                 ((instc.ciw.imm >> 1) & 0x01) ) * 4;
                } else {
                    *illegal = 1;
                    break;
                }
                break;
            case OP_CLW       : // c.lw -> lw rd′, offset[6:2](rs1′)
                r.i.op    = OP_LOAD;
                r.i.rd    = 8 + instc.cl.rd_;
                r.i.func3 = OP_LW;
                r.i.rs1   = 8 + instc.cl.rs1_;
                r.i.imm   = ((instc.cl.imm_h >> 1) +
                             ((instc.cl.imm_l >> 1) & 0x01) +
                             ((instc.cl.imm_l << 4) & 0x10) ) * 4;
                break;
            case OP_CSW       : // c.sw -> sw rs2′, offset[6:2](rs1′)
                r.i.op    = OP_STORE;
                r.i.rd    = 8 + instc.cl.rd_;
                r.i.func3 = OP_SW;
                r.i.rs1   = 8 + instc.cl.rs1_;
                r.i.imm   = ((instc.cl.imm_h >> 1) +
                             ((instc.cl.imm_l >> 1) & 0x01) +
                             ((instc.cl.imm_l << 4) & 0x10) ) * 4;
                break;
            default: *illegal = 1;
        }
        inst->inst = r.inst;
        return 1;
    }

    if (instc.cr.op == 0x1) {
        switch(instc.ci.func3) {
            // OP_CNOP, OP_CADDI
            case OP_CADDI     : 
                if (instc.ci.rd == 0) { // c.nop -> addi x0, x0, 0
                    r.i.op    = OP_ARITHI;
                    r.i.rd    = 0;
                    r.i.func3 = OP_ADD;
                    r.i.rs1   = 0;
                    r.i.imm   = 0;

                // c.addi -> addi rd, rd, nzimm[5:0]
                } else if (instc.ci.imm_l != 0 || instc.ci.imm_h != 0) {
                    r.i.op    = OP_ARITHI;
                    r.i.rd    = instc.ci.rd;
                    r.i.func3 = OP_ADD;
                    r.i.rs1   = instc.ci.rd;
                    r.i.imm   = (instc.ci.imm_h << 5) + instc.ci.imm_l;
                } else {
                    *illegal = 1;
                }
                break;
            case OP_CBEQZ     : // c.beqz -> beq rs1′, x0, offset[8:1]
                r.b.op    = OP_BRANCH;
                r.b.imm1  = (instc.cb.imm_l & 6) + ((instc.cb.imm_h >> 2) & 1) +
                            ((instc.cb.imm_h << 3) & 0x18);
                r.b.func3 = OP_BEQ;
                r.b.rs1   = instc.cb.rs1_ + 8;
                r.b.rs2   = 0;
                r.b.imm2  = ((instc.cb.imm_h & 4) ? 0x78 : 0) +
                            (instc.cb.imm_l & 1) +
                            ((instc.cb.imm_l >> 2) & 6);
                break;
            case OP_CBNEZ     : // c.bnez -> bne rs1′, x0, offset[8:1]
                r.b.op    = OP_BRANCH;
                r.b.imm1  = (instc.cb.imm_l & 6) + ((instc.cb.imm_h >> 2) & 1) +
                            ((instc.cb.imm_h << 3) & 0x18);
                r.b.func3 = OP_BNE;
                r.b.rs1   = instc.cb.rs1_ + 8;
                r.b.rs2   = 0;
                r.b.imm2  = ((instc.cb.imm_h & 4) ? 0x78 : 0) +
                            (instc.cb.imm_l & 1) +
                            ((instc.cb.imm_l >> 2) & 6);
                break;
            case OP_CJ        : // c.j -> jal x0, offset[11:1]
                r.j.op  = OP_JAL;
                r.j.rd  = 0;
                r.j.imm = ((instc.cj.offset & 0x400) ? 0x801ff : 0) +
                          ((instc.cj.offset & 0x001) << 13) +
                          ((instc.cj.offset & 0x007) << 9) +
                          ((instc.cj.offset & 0x010) << 15) +
                          ((instc.cj.offset & 0x020) << 14) +
                          ((instc.cj.offset & 0x040) << 18) +
                          ((instc.cj.offset & 0x180) << 16) +
                          ((instc.cj.offset & 0x200) << 12);
                break;
            case OP_CJAL      : // c.jal -> jal x1, offset[11:1]
                r.j.op  = OP_JAL;
                r.j.rd  = 1;
                r.j.imm = ((instc.cj.offset & 0x400) ? 0x801ff : 0) +
                          ((instc.cj.offset & 0x001) << 13) +
                          ((instc.cj.offset & 0x007) << 9) +
                          ((instc.cj.offset & 0x010) << 15) +
                          ((instc.cj.offset & 0x020) << 14) +
                          ((instc.cj.offset & 0x040) << 18) +
                          ((instc.cj.offset & 0x180) << 16) +
                          ((instc.cj.offset & 0x200) << 12);
                break;
            case OP_CLI       : // c.li -> addi rd, x0, imm[5:0]
                if (instc.ci.rd == 0) { // HINT (addi x0, x0, 0)
                    r.i.op    = OP_ARITHI;
                    r.i.rd    = 0;
                    r.i.func3 = OP_ADD;
                    r.i.rs1   = 0;
                    r.i.imm   = 0;
                } else { // addi rd, x0, imm[5:0]
                    r.i.op    = OP_ARITHI;
                    r.i.rd    = instc.ci.rd;
                    r.i.func3 = OP_ADD;
                    r.i.rs1   = 0;
                    r.i.imm   = (instc.ci.imm_h ? 0xfe0 : 0) + instc.ci.imm_l;
                }
                break;
            // OP_CADDI16SP, OP_CLUI
            case OP_CLUI      :
                if (instc.ci.rd == 2) { // c.addi16sp -> addi x2, x2, nzimm[9:4]
                    if (instc.ci.imm_h != 0 || instc.ci.imm_l != 0) {
                        r.i.op    = OP_ARITHI;
                        r.i.rd    = 2;
                        r.i.func3 = OP_ADD;
                        r.i.rs1   = 2;
                        r.i.imm   = ((instc.ci.imm_h ? 0xe0 : 0) +
                                     ((instc.ci.imm_l >> 4) & 0x01) +
                                     ((instc.ci.imm_l >> 1) & 0x04) +
                                     ((instc.ci.imm_l << 2) & 0x18) +
                                     ((instc.ci.imm_l << 1) & 0x02) ) * 16;
                    } else { // reserved
                        *illegal = 1;
                        break;
                    }
                } else if (instc.ci.rd != 0) { // c.lui -> lui rd, nzimm[17:12]
                    // TODO
                } else { // HINT (addi x0, x0, 0)
                    r.i.op    = OP_ARITHI;
                    r.i.rd    = 0;
                    r.i.func3 = OP_ADD;
                    r.i.rs1   = 0;
                    r.i.imm   = 0;
                }
                break;
            // OP_COR, OP_CXOR, OP_CSUB, OP_CAND
            // OP_CSRAI, OP_CANDI, OP_CSRLI
            case OP_CSRLI     :
                switch(instc.ca.func6 & 3) {
                    case 0: // c.srli -> srli rd′, rd′, shamt[5:0]
                        // TODO
                        break;
                    case 1: // c.srai -> srai rd′, rd′, shamt[5:0]
                        // TODO
                        break;
                    case 2: // c.andi -> andi rd′, rd′, imm[5:0]
                        // TODO
                        break;
                    case 3:
                        if (instc.ca.func6 == 0x27) { // Reserved
                            *illegal = 1;
                            break;
                        }
                        switch(instc.ca.func2) {
                            case 0: // c.sub -> sub rd′, rd′, rs2′
                                // TODO
                                break;
                            case 1: // c.xor -> xor rd′, rd′, rs2′
                                // TODO
                                break;
                            case 2: // c.or -> or rd′, rd′, rs2'
                                // TODO
                                break;
                            case 3: // c.and -> and rd′, rd′, rs2′
                                // TODO
                                break;
                        }
                        break;
                }
                break;
            default: *illegal = 1;
        }
        inst->inst = r.inst;
        return 1;
    }

    if (instc.cr.op == 0x2) {
        switch(instc.ci.func3) {
            // OP_CADD, OP_CEBREAK, OP_CJALR, OP_CMV, OP_CJR
            case OP_CADD    :
                if (instc.ci.imm_h == 0 && instc.ci.rd != 0 && instc.ci.imm_l == 0) { // c.jr -> jalr x0, 0(rs1)
                    // TODO
                    break;
                }
                if (instc.ci.imm_h == 0 && instc.ci.rd != 0 && instc.ci.imm_l != 0) { // c.mv -> add rd, x0, rs2
                    // TODO
                    break;
                }
                if (instc.ci.imm_h == 1 && instc.ci.rd == 0 && instc.ci.imm_l == 0) { // c.ebreak -> ebreak
                    // TODO
                    break;
                }
                if (instc.ci.imm_h == 1 && instc.ci.rd != 0 && instc.ci.imm_l == 0) { // c.jalr -> jalr x1, 0(rs1)
                    // TODO
                    break;
                }
                if (instc.ci.imm_h == 1 && instc.ci.rd != 0 && instc.ci.imm_l != 0) { // c.add -> add rd, rd, rs2
                    // TODO
                    break;
                }
                *illegal = 1;
                break;
            case OP_CLWSP   : // c.lwsp -> lw rd, offset[7:2](x2)
                // TODO
                break;
            case OP_CSLLI   : // c.slli -> slli rd, rd, shamt[5:0]
                // TODO
                break;
            case OP_CSWSP   : // c.swsp -> sw rs2, offset[7:2](x2)
                // TODO
                break;
            default: *illegal = 1;
        }
        inst->inst = r.inst;
        return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    FILE *ft = NULL;
    static int regs[32];

    int i;
    int result;
    int *imem;
    int *dmem;
    char *file = NULL;
    char *tfile = NULL;
    int isize;
    int dsize;
    int branch_predict = 0;
    int prev_pc;
    int timer_irq;
    int sw_irq;
    int sw_irq_next;
    int ext_irq;
    int ext_irq_next;
    int compressed = 0;

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
                    printf("malloc fail\n");
                    exit(1);
                }
                strncpy_s(tfile, MAXLEN-1, optarg, MAXLEN-1);
                break;
            case 'q':
                quiet = 1;
                break;
            case 'm':
                if (optarg[0] == '0' && optarg[1] == 'x')
                    sscanf(optarg, "%x", (unsigned int*)&mem_base);
                else
                    mem_base = atoi(optarg);
                break;
            case 'n':
                if (optarg[0] == '0' && optarg[1] == 'x')
                    sscanf(optarg, "%x", (unsigned int*)&mem_size);
                else
                    mem_size = atoi(optarg);
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
            printf("malloc fail\n");
            exit(1);
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
            printf("can not open file %s\n", tfile);
            exit(1);
        }
    }
    if ((imem = (int*)aligned_malloc(sizeof(int), IMEM_SIZE)) == NULL) {
        printf("malloc fail\n");
        exit(1);
    }
    if ((dmem = (int*)aligned_malloc(sizeof(int), DMEM_SIZE)) == NULL) {
        printf("malloc fail\n");
        exit(1);
    }

    // clear the data memory
    memset(dmem, 0, DMEM_SIZE);

    // load the .text and .data section
    if ((result = elfread(file, (char*)imem, (char*)dmem, &isize, &dsize)) == 0) {
        printf("Can not read elf file %s\n", file);
        exit(1);
    }

    // Registers initialize
    for(i=0; i<sizeof(regs)/sizeof(int); i++) {
        regs[i] = 0;
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

    // Execution loop
    while(1) {
        INST inst;
        INSTC instc;
        int illegal;

        illegal = 0;
        mtime_update = 0;

        //FIXME: to pass the compliance test, the destination PC should
        //       be aligned at short word.
        pc &= 0xfffffffe;

        // keep x0 always zero
        regs[0] = 0;

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

        if (IVA2PA(pc) >= isize || IVA2PA(pc) < 0) {
            printf("PC 0x%08x out of range 0x%08x\n", pc, IPA2VA(isize));
            TRAP(TRAP_INST_FAIL, pc);
        }

        if ((pc&2) != 0) {
            printf("PC 0x%08x alignment error\n", pc);
            TRAP(TRAP_INST_ALIGN, pc);
        }

        inst.inst = (IVA2PA(pc) & 2) ?
                     (imem[IVA2PA(pc)/4+1] << 16) | ((imem[IVA2PA(pc)] >> 16) & 0xffff) :
                     imem[IVA2PA(pc)/4];
        instc.inst = (IVA2PA(pc) & 2) ?
                     (short)(imem[IVA2PA(pc)/4] >> 16) :
                     (short)imem[IVA2PA(pc)/4];

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

        compressed = compressed_decoder(instc, &inst, &illegal);

        if (illegal) {
            TRAP(TRAP_INST_ILL, (int)instc.inst);
        }

        switch(inst.r.op) {
            case OP_AUIPC : { // U-Type
                regs[inst.u.rd] = pc + to_imm_u(inst.u.imm);
                TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n", pc, inst.inst,
                           inst.u.rd, regname[inst.u.rd], regs[inst.u.rd] TRACE_END;
                break;
            }
            case OP_LUI   : { // U-Type
                regs[inst.u.rd] = to_imm_u(inst.u.imm);
                TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n", pc, inst.inst,
                          inst.u.rd, regname[inst.u.rd], regs[inst.u.rd] TRACE_END;
                break;
            }
            case OP_JAL   : { // J-Type
                regs[inst.j.rd] = compressed ? pc + 2 : pc + 4;
                TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n", pc, inst.inst,
                          inst.j.rd, regname[inst.j.rd], regs[inst.j.rd] TRACE_END;
                pc += to_imm_j(inst.j.imm);
                if (to_imm_j(inst.j.imm) == 0) {
                    printf("Warning: forever loop detected at PC 0x%08x\n", pc);
                    prog_exit(1);
                }
                if ((pc&2) == 0)
                    CYCLE_ADD(branch_penalty);
                continue;
            }
            case OP_JALR  : { // I-Type
                int new_pc = compressed ? pc + 2 : pc + 4;
                TIME_LOG; TRACE_LOG "%08x ", pc TRACE_END;
                pc = regs[inst.i.rs1] + to_imm_i(inst.i.imm);
                regs[inst.i.rd] = new_pc;
                TRACE_LOG "%08x x%02u (%s) <= 0x%08x\n", inst.inst, inst.i.rd,
                          regname[inst.i.rd], regs[inst.i.rd] TRACE_END;
                if ((pc&2) == 0)
                    CYCLE_ADD(branch_penalty);
                continue;
            }
            case OP_BRANCH: { // B-Type
                TIME_LOG; TRACE_LOG "%08x %08x\n", pc, inst.inst TRACE_END;
                int offset = to_imm_b(inst.b.imm2, inst.b.imm1);
                switch(inst.b.func3) {
                    case OP_BEQ  :
                        if (regs[inst.b.rs1] == regs[inst.b.rs2]) {
                            pc += offset;
                            if ((!branch_predict || offset > 0) && (pc&2) == 0)
                                CYCLE_ADD(branch_penalty);
                            continue;
                        }
                        break;
                    case OP_BNE  :
                        if (regs[inst.b.rs1] != regs[inst.b.rs2]) {
                            pc += offset;
                            if ((!branch_predict || offset > 0) && (pc&2) == 0)
                                CYCLE_ADD(branch_penalty);
                            continue;
                        }
                        break;
                    case OP_BLT  :
                        if (regs[inst.b.rs1] < regs[inst.b.rs2]) {
                            pc += offset;
                            if ((!branch_predict || offset > 0) && (pc&2) == 0)
                                CYCLE_ADD(branch_penalty);
                            continue;
                        }
                        break;
                    case OP_BGE  :
                        if (regs[inst.b.rs1] >= regs[inst.b.rs2]) {
                            pc += offset;
                            if ((!branch_predict || offset > 0) && (pc&2) == 0)
                                CYCLE_ADD(branch_penalty);
                            continue;
                        }
                        break;
                    case OP_BLTU :
                        if (((unsigned int)regs[inst.b.rs1]) < ((unsigned int)regs[inst.b.rs2])) {
                            pc += offset;
                            if ((!branch_predict || offset > 0) && (pc&2) == 0)
                                CYCLE_ADD(branch_penalty);
                            continue;
                        }
                        break;
                    case OP_BGEU :
                        if (((unsigned int)regs[inst.b.rs1]) >= ((unsigned int)regs[inst.b.rs2])) {
                            pc += offset;
                            if ((!branch_predict || offset > 0) && (pc&2) == 0)
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
            case OP_LOAD  : { // I-Type
                COUNTER counter;
                int memdata;
                int memaddr;
                int data;
                int address = regs[inst.i.rs1] + to_imm_i(inst.i.imm);
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
                memdata = data;
                switch(inst.i.func3) {
                    case OP_LB  : // fall through
                    case OP_LBU : data = (data >> ((address&3)*8))&0xff;
                                  if (inst.i.func3 == OP_LB && (data&0x80))
                                    data |= 0xffffff00;
                                  break;
                    case OP_LH  : // fall through
                    case OP_LHU : if (address&1) {
                                    TRACE_LOG "\n" TRACE_END;
                                    printf("Unalignment address 0x%08x to read at 0x%08x\n",
                                            DPA2VA(address), pc);
                                    TRAP(TRAP_LD_ALIGN, DPA2VA(address));
                                    continue;
                                  }
                                  data = (address&2) ? ((data>>16)&0xffff) : (data &0xffff);
                                  if (inst.i.func3 == OP_LH && (data&0x8000))
                                    data |= 0xffff0000;
                                  break;
                    case OP_LW  : if (address&3) {
                                    TRACE_LOG "\n" TRACE_END;
                                    printf("Unalignment address 0x%08x to read at 0x%08x\n",
                                            DPA2VA(address), pc);
                                    TRAP(TRAP_LD_ALIGN, DPA2VA(address));
                                    continue;
                                  }
                                  break;
                    default: TRACE_LOG " read 0x%08x => 0x%08x, x%02u (%s) <= 0x%08x\n",
                                       memaddr, memdata, inst.i.rd,
                                       regname[inst.i.rd], 0 TRACE_END;
                             printf("Illegal load instruction at PC 0x%08x\n", pc);
                             TRAP(TRAP_INST_ILL, inst.inst);
                             continue;
                }
                regs[inst.i.rd] = data;
                TRACE_LOG " read 0x%08x => 0x%08x, x%02u (%s) <= 0x%08x\n",
                          memaddr, memdata, inst.i.rd,
                          regname[inst.i.rd], regs[inst.i.rd] TRACE_END;
                break;
            }
            case OP_STORE : { // S-Type
                int address = regs[inst.s.rs1] +
                              to_imm_s(inst.s.imm2, inst.s.imm1);
                int data = regs[inst.s.rs2];
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
                                printf("Unknown address 0x%08x to write at 0x%08x\n",
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
                    case OP_SB : dmem[address/4] =
                                    (dmem[address/4]&~(0xff<<((address&3)*8))) |
                                    ((data & 0xff)<<((address&3)*8));
                                 break;
                    case OP_SH : if (address&1) {
                                    TRACE_LOG "\n" TRACE_END;
                                    printf("Unalignment address 0x%08x to write at 0x%08x\n",
                                            DPA2VA(address), pc);
                                    TRAP(TRAP_ST_ALIGN, DPA2VA(address));
                                    continue;
                                 }
                                 dmem[address/4] = (address&2) ?
                                    ((dmem[address/4]&0xffff)|(data << 16)) :
                                    ((dmem[address/4]&0xffff0000)|(data&0xffff));
                                 break;
                    case OP_SW : if (address&3) {
                                    TRACE_LOG "\n" TRACE_END;
                                    printf("Unalignment address 0x%08x to write at 0x%08x\n",
                                            DPA2VA(address), pc);
                                    TRAP(TRAP_ST_ALIGN, DPA2VA(address));
                                    continue;
                                 }
                                 dmem[address/4] = data;
                                 break;
                    default: printf("Illegal store instruction at PC 0x%08x\n", pc);
                             TRACE_LOG "\n" TRACE_END;
                             TRAP(TRAP_INST_ILL, inst.inst);
                             continue;
                }
                TRACE_LOG " write 0x%08x <= 0x%08x\n", DPA2VA(address), (data & mask) TRACE_END;
                break;
            }
            case OP_ARITHI: { // I-Type
                switch(inst.i.func3) {
                    case OP_ADD  : regs[inst.i.rd] = regs[inst.i.rs1] + to_imm_i(inst.i.imm);
                                   break;
                    case OP_SLT  : regs[inst.i.rd] = regs[inst.i.rs1] < to_imm_i(inst.i.imm) ? 1 : 0;
                                   break;
                    case OP_SLTU : //FIXME: to pass compliance test, the IMM should be singed
                                   //extension, and compare with unsigned.
                                   //regs[inst.i.rd] = ((unsigned int)regs[inst.i.rs1]) <
                                   //                ((unsigned int)to_imm_iu(inst.i.imm)) ? 1 : 0;
                                   regs[inst.i.rd] = ((unsigned int)regs[inst.i.rs1]) <
                                                     ((unsigned int)to_imm_i(inst.i.imm)) ? 1 : 0;
                                   break;
                    case OP_XOR  : regs[inst.i.rd] = regs[inst.i.rs1] ^ to_imm_i(inst.i.imm);
                                   break;
                    case OP_OR   : regs[inst.i.rd] = regs[inst.i.rs1] | to_imm_i(inst.i.imm);
                                   break;
                    case OP_AND  : regs[inst.i.rd] = regs[inst.i.rs1] & to_imm_i(inst.i.imm);
                                   break;
                    case OP_SLL  : regs[inst.i.rd] = regs[inst.i.rs1] << (inst.i.imm&0x1f);
                                   break;
                    case OP_SR   : if (inst.r.func7 == 0)
                                       regs[inst.i.rd] = ((unsigned int)regs[inst.i.rs1]) >>
                                                         (inst.i.imm&0x1f);
                                   else
                                       regs[inst.i.rd] = regs[inst.i.rs1] >> (inst.i.imm&0x1f);
                                   break;
                    default: printf("Unknown instruction at 0x%08x\n", pc);
                             TRAP(TRAP_INST_ILL, inst.inst);
                             continue;
                }
                TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n",
                          pc, inst.inst, inst.i.rd, regname[inst.i.rd],
                          regs[inst.i.rd] TRACE_END;
                break;
            }
            case OP_ARITHR: { // R-Type
                if (inst.r.func7 == 1) { // RV32M Multiply Extension
                    switch(inst.r.func3) {
                        case OP_MUL   : regs[inst.r.rd] = regs[inst.r.rs1] *
                                                          regs[inst.r.rs2];
                                        break;
                        case OP_MULH  : {
                                        union {
                                            long long l;
                                            struct { int l, h; } n;
                                        } a, b, r;
                                        a.l = (long long)regs[inst.r.rs1];
                                        b.l = (long long)regs[inst.r.rs2];
                                        r.l = a.l * b.l;
                                        regs[inst.r.rd] = r.n.h;
                                        }
                                        break;
                        case OP_MULSU : {
                                        union {
                                            long long l;
                                            struct { int l, h; } n;
                                        } a, b, r;
                                        a.l = (long long)regs[inst.r.rs1];
                                        b.n.l = regs[inst.r.rs2];
                                        b.n.h = 0;
                                        r.l = a.l * b.l;
                                        regs[inst.r.rd] = r.n.h;
                                        }
                                        break;
                        case OP_MULU  : {
                                        union {
                                            long long l;
                                            struct { int l, h; } n;
                                        } a, b, r;
                                        a.n.l = regs[inst.r.rs1]; a.n.h = 0;
                                        b.n.l = regs[inst.r.rs2]; b.n.h = 0;
                                        r.l = ((unsigned long long)a.l) *
                                              ((unsigned long long)b.l);
                                        regs[inst.r.rd] = r.n.h;
                                        }
                                        break;
                        case OP_DIV   : if (regs[inst.r.rs2])
                                            regs[inst.r.rd] = (int)(((long long)regs[inst.r.rs1]) /
                                                                   regs[inst.r.rs2]);
                                        else
                                            regs[inst.r.rd] = 0xffffffff;
                                        break;
                        case OP_DIVU  : if (regs[inst.r.rs2])
                                            regs[inst.r.rd] = (int)(((unsigned)regs[inst.r.rs1]) /
                                                                    ((unsigned)regs[inst.r.rs2]));
                                        else
                                            regs[inst.r.rd] = 0xffffffff;
                                        break;
                        case OP_REM   : if (regs[inst.r.rs2])
                                            regs[inst.r.rd] = (int)(((long long)regs[inst.r.rs1]) %
                                                                    regs[inst.r.rs2]);
                                        else
                                            regs[inst.r.rd] = regs[inst.r.rs1];
                                        break;
                        case OP_REMU  : if (regs[inst.r.rs2])
                                            regs[inst.r.rd] = (int)(((unsigned)regs[inst.r.rs1]) %
                                                                    ((unsigned)regs[inst.r.rs2]));
                                        else
                                            regs[inst.r.rd] = regs[inst.r.rs1];
                                        break;
                        default: printf("Unknown instruction at 0x%08x\n", pc);
                                 TRAP(TRAP_INST_ILL, inst.inst);
                                 continue;
                    }
                } else {
                    switch(inst.r.func3) {
                        case OP_ADD  : if (inst.r.func7 == 0)
                                           regs[inst.r.rd] = regs[inst.r.rs1] + regs[inst.r.rs2];
                                       else
                                           regs[inst.r.rd] = regs[inst.r.rs1] - regs[inst.r.rs2];
                                       break;
                        case OP_SLL  : regs[inst.r.rd] = regs[inst.r.rs1] << regs[inst.r.rs2];
                                       break;
                        case OP_SLT  : regs[inst.r.rd] = regs[inst.r.rs1] < regs[inst.r.rs2] ?
                                                         1 : 0;
                                       break;
                        case OP_SLTU : regs[inst.r.rd] = ((unsigned int)regs[inst.r.rs1]) <
                                            ((unsigned int)regs[inst.r.rs2]) ? 1 : 0;
                                       break;
                        case OP_XOR  : regs[inst.r.rd] = regs[inst.r.rs1] ^ regs[inst.r.rs2];
                                       break;
                        case OP_SR   : if (inst.r.func7 == 0)
                                           regs[inst.r.rd] = ((unsigned int)regs[inst.r.rs1]) >>
                                                             regs[inst.r.rs2];
                                       else
                                           regs[inst.r.rd] = regs[inst.r.rs1] >> regs[inst.r.rs2];
                                       break;
                        case OP_OR   : regs[inst.r.rd] = regs[inst.r.rs1] | regs[inst.r.rs2];
                                       break;
                        case OP_AND  : regs[inst.r.rd] = regs[inst.r.rs1] & regs[inst.r.rs2];
                                       break;
                        default: printf("Unknown instruction at 0x%08x\n", pc);
                                 TRAP(TRAP_INST_ILL, inst.inst);
                                 continue;
                    }
                }
                TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n",
                          pc, inst.inst, inst.r.rd, regname[inst.r.rd],
                          regs[inst.r.rd] TRACE_END;
                break;
            }
            case OP_FENCE : {
                TIME_LOG; TRACE_LOG "%08x %08x\n", pc, inst.inst TRACE_END;
                break;
            }
            case OP_SYSTEM: { // I-Type
                int val;
                int update;
                int csr_op = 0;
                int csr_type;
                // RDCYCLE, RDTIME and RDINSTRET are read only
                switch(inst.i.func3) {
                    case OP_ECALL  : TIME_LOG; TRACE_LOG "%08x %08x\n", pc, inst.inst TRACE_END;
                                     switch (inst.i.imm & 3) {
                                        case 0: // ecall
                                            switch(regs[A7]) { // function argument A7/X17
                                               case SYS_EXIT:
                                                   prog_exit(0);
                                                   break;
                                               case SYS_READ:
                                                   if (regs[A0] == STDIN) {
                                                       char *ptr = (char*)dmem;
                                                       int i = 0;
                                                       char c = 0;
                                                       do {
                                                           c = getch();
                                                           ptr[DVA2PA(regs[A1])+i] = c;
                                                       } while(++i<regs[A2] && c != '\n');
                                                   }
                                                   break;
                                               case SYS_WRITE:
                                                   if (regs[A0] == STDOUT) {
                                                       char *ptr = (char*)dmem;
                                                       int i;
                                                       for(i=0; i<regs[A2]; i++) {
                                                           char c = ptr[DVA2PA(regs[A1])+i];
                                                           putchar(c);
                                                       }
                                                       fflush(stdout);
                                                   }
                                                   break;
                                               case SYS_DUMP: {
                                                       FILE *fp;
                                                       int i;
                                                       if ((fp = fopen("dump.txt", "w")) == NULL) {
                                                           printf("Create dump.txt fail\n");
                                                           exit(1);
                                                       }
                                                       if ((regs[A0] & 3) != 0 || (regs[A1] & 3) != 0) {
                                                           printf("Alignment error on memory dumping.\n");
                                                           exit(1);
                                                       }
                                                       for(i = DVA2PA(regs[A0])/4;
                                                           i < DVA2PA(regs[A1])/4; i++) {
                                                           fprintf(fp, "%08x\n", dmem[i]);
                                                       }
                                                       fclose(fp);
                                                   }
                                                   break;
                                               default:
                                                   break;
                                            }
                                            TRAP(TRAP_ECALL, 0);
                                            continue;
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
                                            if ((pc&2) == 0)
                                                CYCLE_ADD(branch_penalty);
                                            continue;
                                        default:
                                            printf("Illegal system call at PC 0x%08x\n", pc);
                                            TRAP(TRAP_INST_ILL, 0);
                                            continue;
                                     }
                    case OP_CSRRWI : csr_op = 1;
                                     val = inst.i.rs1;
                                     update = 1;
                                     csr_type = OP_CSRRW;
                                     break;
                    // If the zimm[4:0] field is zero, then these instructions will not write
                    // to the CSR
                    case OP_CSRRW  : csr_op = 1;
                                     val = regs[inst.i.rs1];
                                     update = 1;
                                     csr_type = OP_CSRRW;
                                     break;
                    // For both CSRRS and CSRRC, if rs1=x0, then the instruction will not
                    // write to the CSR at all
                    case OP_CSRRSI : csr_op = 1;
                                     val = inst.i.rs1;
                                     update = (inst.i.rs1 == 0) ? 0 : 1;
                                     csr_type = OP_CSRRS;
                                     break;
                    case OP_CSRRS  : csr_op = 1;
                                     val = regs[inst.i.rs1];
                                     update = (inst.i.rs1 == 0) ? 0 : 1;
                                     csr_type = OP_CSRRS;
                                     break;
                    case OP_CSRRCI : csr_op = 1;
                                     val = inst.i.rs1;
                                     update = (inst.i.rs1 == 0) ? 0 : 1;
                                     csr_type = OP_CSRRC;
                                     break;
                    case OP_CSRRC  : csr_op = 1;
                                     val = regs[inst.i.rs1];
                                     update = (inst.i.rs1 == 0) ? 0 : 1;
                                     csr_type = OP_CSRRC;
                                     break;
                    default: printf("Unknown system instruction at 0x%08x\n", pc);
                             TIME_LOG; TRACE_LOG "%08x %08x\n", pc, inst.inst TRACE_END;
                             TRAP(TRAP_INST_ILL, inst.inst);
                             continue;
                }
                if (csr_op) {
                    int result = csr_rw(inst.i.imm, csr_type, val, update, &regs[inst.i.rd]);
                    TIME_LOG; TRACE_LOG "%08x %08x x%02u (%s) <= 0x%08x\n",
                              pc, inst.inst, inst.i.rd,
                              regname[inst.i.rd], regs[inst.i.rd] TRACE_END;
                    if (!result) {
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
        }
        pc = compressed ? pc + 2 : pc + 4;
    }

    aligned_free(imem);
    aligned_free(dmem);
    if (ft) fclose(ft);
}

