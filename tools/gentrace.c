// Software simulator for RISC-V RV32I instruction sets
// Copyright 2020, Kuoping Hsu, GPL license
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <bfd.h>
#include "opcode.h"

#define MAXLEN      1024
#define RAMSIZE     128*1024

#define IMEM_BASE   0
#define DMEM_BASE   RAMSIZE
#define IMEM_SIZE   RAMSIZE
#define DMEM_SIZE   RAMSIZE

COUNTER time;
COUNTER cycle;
COUNTER instret;
int mtvec = 0;
int misa  = 0x40000000;
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

void usage(void) {
    printf(
"Usage: tracegen [-h] [-b n] [-p] [-l logfile] file\n\n"
"       --help, -h              help\n"
"       --branch n, -b n        branch penalty (default 2)\n"
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
            unsigned int a4 : 20;
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
            unsigned int a4 : 12;
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
        printf("\nExcuting %lld instructions, %lld cycles, %1.3f CPI\n", instret.c, cycle.c, ((float)cycle.c)/instret.c);
        printf("Program terminate\n");
    }
    exit(exitcode);
}

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
		printf("Failed to open file %s!\n", file);
		return 0;
    }

	asection *text = bfd_get_section_by_name (abfd, ".text");
	asection *data = bfd_get_section_by_name (abfd, ".data");

	// copy the contents of the data and executable sections into the allocated memory
	bfd_get_section_contents(abfd, text, imem, 0, text->size);
	bfd_get_section_contents(abfd, data, dmem, 0, data->size);

    *isize = text->size;
    *dsize = data->size;

    if (!quiet) printf("Load .text section %ld, .data section %ld bytes\n", text->size, data->size);

    bfd_close(abfd);

    return 1;
}

int csr_rw(int regs, int val) {
    int result = 0;
    switch(regs) {
        case CSR_RDCYCLE   : result = cycle.d.lo-1;
                             break;
        case CSR_RDCYCLEH  : result = cycle.d.hi-1;
                             break;
        case CSR_RDTIME    : result = time.d.lo-1;
                             break;
        case CSR_RDTIMEH   : result = time.d.hi-1;
                             break;
        case CSR_RDINSTRET : result = instret.d.lo-1;
                             break;
        case CSR_RDINSTRETH: result = instret.d.hi-1;
                             break;
        case CSR_MTVEC     : result = mtvec;
                             mtvec = val;
                             break;
        case CSR_MISA      : result = misa;
                             misa = val;
                             break;
        default: printf("Unsupport CSR register 0x%03x\n", regs);
                 exit(-1);
    }
    return result;
}

int main(int argc, char **argv) {
    FILE *ft = NULL;
    static int regs[32];

    int i;
    int result;
    int pc = 0;
    int *imem;
    int *dmem;
    char *file = NULL;
    char *tfile = NULL;
    int isize;
    int dsize;
    int branch_penalty = BRANCH_PENALTY;
    int branch_predict = 0;

    const char *optstring = "hb:pl:q";
    int c;
    struct option opts[] = {
        {"help", 0, NULL, 'h'},
        {"branch", 1, NULL, 'b'},
        {"predict", 0, NULL, 'p'},
        {"log", 1, NULL, 'l'},
        {"quiet", 0, NULL, 'q'}
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
                strncpy(tfile, optarg, MAXLEN);
                break;
            case 'q':
                quiet = 1;
                break;
            default:
                usage();
                return 1;
        }
    }

    if (argc > 1) {
        if ((file = malloc(MAXLEN)) == NULL) {
            printf("malloc fail\n");
            exit(1);
        }
        strncpy(file, argv[optind], MAXLEN);
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
        exit(1);
    }

    // Registers initialize
    for(i=0; i<sizeof(regs)/sizeof(int); i++) {
        regs[i] = 0;
    }

    time.c    = 0;
    cycle.c   = 0;
    instret.c = 0;

    // Execution loop
    while(1) {
        INST inst;
        
        time.c++;
        cycle.c++;
        instret.c++;

        if (pc >= isize || pc < 0) {
            printf("PC 0x%08x out of range 0x%08x\n", pc, isize);
            break;
        }
        if ((pc&3) != 0) {
            printf("PC 0x%08x alignment error\n", pc);
            break;
        }
        regs[0] = 0;
        inst.inst = imem[IVA2PA(pc)/4];
        switch(inst.r.op) {
            case OP_AUIPC : { // U-Type
                regs[inst.u.rd] = pc + to_imm_u(inst.u.imm);
                if (ft) fprintf(ft, "%08x %08x x%02d (%s) <= 0x%08x\n", pc, inst.inst, inst.u.rd, regname[inst.u.rd], regs[inst.u.rd]);
                break;
            }
            case OP_LUI   : { // U-Type
                regs[inst.u.rd] = to_imm_u(inst.u.imm);
                if (ft) fprintf(ft, "%08x %08x x%02d (%s) <= 0x%08x\n", pc, inst.inst, inst.u.rd, regname[inst.u.rd], regs[inst.u.rd]);
                break;
            }
            case OP_JAL   : { // J-Type
                regs[inst.j.rd] = pc + 4;
                if (ft) fprintf(ft, "%08x %08x x%02d (%s) <= 0x%08x\n", pc, inst.inst, inst.j.rd, regname[inst.j.rd], regs[inst.j.rd]);
                pc += to_imm_j(inst.j.imm);
                if (to_imm_j(inst.j.imm) == 0) {
                    printf("Warnning: forever loop detected at PC 0x%08x\n", pc);
                    prog_exit(1);
                }
                cycle.c += branch_penalty;
                continue;
            }
            case OP_JALR  : { // I-Type
		int new_pc = pc + 4;
                if (ft) fprintf(ft, "%08x ", pc);
                pc = regs[inst.i.rs1] + to_imm_i(inst.i.imm);
                regs[inst.i.rd] = new_pc;
                if (ft) fprintf(ft, "%08x x%02d (%s) <= 0x%08x\n", inst.inst, inst.i.rd, regname[inst.i.rd], regs[inst.i.rd]);
                cycle.c += branch_penalty;
                continue;
            }
            case OP_BRANCH: { // B-Type
                if (ft) fprintf(ft, "%08x %08x\n", pc, inst.inst);
                int offset = to_imm_b(inst.b.imm2, inst.b.imm1);
                switch(inst.b.func3) {
                    case OP_BEQ  :
                        if (regs[inst.b.rs1] == regs[inst.b.rs2]) {
                            pc += offset;
                            if (!branch_predict || offset > 0)
                                cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BNE  :
                        if (regs[inst.b.rs1] != regs[inst.b.rs2]) {
                            pc += offset;
                            if (!branch_predict || offset > 0)
                                cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BLT  :
                        if (regs[inst.b.rs1] < regs[inst.b.rs2]) {
                            pc += offset;
                            if (!branch_predict || offset > 0)
                                cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BGE  :
                        if (regs[inst.b.rs1] >= regs[inst.b.rs2]) {
                            pc += offset;
                            if (!branch_predict || offset > 0)
                                cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BLTU :
                        if (((unsigned int)regs[inst.b.rs1]) < ((unsigned int)regs[inst.b.rs2])) {
                            pc += offset;
                            if (!branch_predict || offset > 0)
                                cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BGEU :
                        if (((unsigned int)regs[inst.b.rs1]) >= ((unsigned int)regs[inst.b.rs2])) {
                            pc += offset;
                            if (!branch_predict || offset > 0)
                                cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    default:
                        printf("Unknown branch instruction at 0x%08x\n", pc);
                        exit(-1);
                }
                break;
            }
            case OP_LOAD  : { // I-Type
                int data;
                int address = regs[inst.i.rs1] + to_imm_i(inst.i.imm);
                if (address < DMEM_BASE || address >= DMEM_BASE + DMEM_SIZE) {
                    printf("x%0d = 0x%08x, imm = %d\n", inst.i.rs1, regs[inst.i.rs1], to_imm_i(inst.i.imm));
                    printf("memory address 0x%08x out of range\n", address);
                    exit(-1);
                }
                address = DVA2PA(address);
                data = dmem[address/4];
                switch(inst.i.func3) {
                    case OP_LB  : data = (data >> ((address&3)*8))&0xff;
                                  if (data&0x80) data |= 0xffffff00;
                                  break;
                    case OP_LH  : data = (address&2) ? ((data>>16)&0xffff) : (data &0xffff);
                                  if (data&0x8000) data |= 0xffff0000;
                                  break;
                    case OP_LW  : break;
                    case OP_LBU : data = (data >> ((address&3)*8))&0xff;
                                  break;
                    case OP_LHU : data = (address&2) ? ((data>>16)&0xffff) : (data &0xffff);
                                  break;
                    default: printf("Unknown load instruction at 0x%08x\n", pc);
                             exit(-1);
                }
                regs[inst.i.rd] = data;
                if (ft) fprintf(ft, "%08x %08x read 0x%08x => 0x%08x, x%02d (%s) <= 0x%08x\n", pc, inst.inst, DPA2VA(address), dmem[address/4], inst.i.rd, regname[inst.i.rd], regs[inst.i.rd]);
                break;
            }
            case OP_STORE : { // S-Type
                int address = regs[inst.s.rs1] + to_imm_s(inst.s.imm2, inst.s.imm1);
                int data = regs[inst.s.rs2];
                int mask = (inst.s.func3 == OP_SB) ? 0xff :
                           (inst.s.func3 == OP_SH) ? 0xffff :
                           (inst.s.func3 == OP_SW) ? 0xffffffff :
                           0xffffffff;
                if (ft) fprintf(ft, "%08x %08x write 0x%08x <= 0x%08x\n", pc, inst.inst, address, (data & mask));
                if (address < DMEM_BASE || address > DMEM_BASE+DMEM_SIZE) {
                    switch(address) {
                        case MMIO_PUTC:
                            putchar((char)data);
                            fflush(stdout);
                            pc += 4;
                            continue;
                        case MMIO_EXIT:
                            prog_exit(data);
                            break;
                        default:
                            printf("Unknown address 0x%08x to write at 0x%08x\n", address, pc);
                            exit(-1);
                    }
                }
                address = DVA2PA(address);
                switch(inst.s.func3) {
                    case OP_SB : dmem[address/4] = (dmem[address/4]&~(0xff<<((address&3)*8))) | ((data & 0xff)<<((address&3)*8));
                                 break;
                    case OP_SH : dmem[address/4] = (address&2) ? ((dmem[address/4]&0xffff)|(data << 16)) : ((dmem[address/4]&0xffff0000)|(data&0xffff));
                                 if (address&1) {
                                    printf("Unalignment address 0x%08x to write at 0x%08x\n", DPA2VA(address), pc);
                                    exit(-1);
                                 }
                                 break;
                    case OP_SW : dmem[address/4] = data;
                                 if (address&3) {
                                    printf("Unalignment address 0x%08x to write at 0x%08x\n", DPA2VA(address), pc);
                                    exit(-1);
                                 }
                                 break;
                    default: printf("Unknown store instruction at 0x%08x\n", pc);
                             exit(-1);
                }
                break;
            }
            case OP_ARITHI: { // I-Type
                switch(inst.i.func3) {
                    case OP_ADD  : regs[inst.i.rd] = regs[inst.i.rs1] + to_imm_i(inst.i.imm);
                                   break;
                    case OP_SLT  : regs[inst.i.rd] = regs[inst.i.rs1] < to_imm_i(inst.i.imm) ? 1 : 0;
                                   break;
                    case OP_SLTU : //FIXME: to pass compliance test, the IMM should be singed extension, and compare with unsinged.
                                   //regs[inst.i.rd] = ((unsigned int)regs[inst.i.rs1]) < ((unsigned int)to_imm_iu(inst.i.imm)) ? 1 : 0;
                                   regs[inst.i.rd] = ((unsigned int)regs[inst.i.rs1]) < ((unsigned int)to_imm_i(inst.i.imm)) ? 1 : 0;
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
                                       regs[inst.i.rd] = ((unsigned int)regs[inst.i.rs1]) >> (inst.i.imm&0x1f);
                                   else
                                       regs[inst.i.rd] = regs[inst.i.rs1] >> (inst.i.imm&0x1f);
                                   break;
                    default: printf("Unknown instruction at 0x%08x\n", pc);
                             exit(-1);
                }
                if (ft) fprintf(ft, "%08x %08x x%02d (%s) <= 0x%08x\n", pc, inst.inst, inst.i.rd, regname[inst.i.rd], regs[inst.i.rd]);
                break;
            }
            case OP_ARITHR: { // R-Type
                if (inst.r.func7 == 1) { // RV32M Multiply Extension
                    switch(inst.r.func3) {
                        case OP_MUL   : regs[inst.r.rd] = regs[inst.r.rs1] * regs[inst.r.rs2];
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
                                        r.l = ((unsigned long long)a.l) * ((unsigned long long)b.l);
                                        regs[inst.r.rd] = r.n.h;
                                        }
                                        break;
                        case OP_DIV   : if (regs[inst.r.rs2])
                                            regs[inst.r.rd] = (int)(((long long)regs[inst.r.rs1]) / regs[inst.r.rs2]);
                                        else
                                            regs[inst.r.rd] = 0xffffffff;
                                        break;
                        case OP_DIVU  : if (regs[inst.r.rs2])
                                            regs[inst.r.rd] = (int)(((unsigned)regs[inst.r.rs1]) / ((unsigned)regs[inst.r.rs2]));
                                        else
                                            regs[inst.r.rd] = 0xffffffff;
                                        break;
                        case OP_REM   : if (regs[inst.r.rs2])
                                            regs[inst.r.rd] = (int)(((long long)regs[inst.r.rs1]) % regs[inst.r.rs2]);
                                        else
                                            regs[inst.r.rd] = regs[inst.r.rs1];
                                        break;
                        case OP_REMU  : if (regs[inst.r.rs2])
                                            regs[inst.r.rd] = (int)(((unsigned)regs[inst.r.rs1]) % ((unsigned)regs[inst.r.rs2]));
                                        else
                                            regs[inst.r.rd] = regs[inst.r.rs1];
                                        break;
                        default: printf("Unknown instruction at 0x%08x\n", pc);
                                 exit(-1);
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
                        case OP_SLT  : regs[inst.r.rd] = regs[inst.r.rs1] < regs[inst.r.rs2] ? 1 : 0;
                                       break;
                        case OP_SLTU : regs[inst.r.rd] = ((unsigned int)regs[inst.r.rs1]) < ((unsigned int)regs[inst.r.rs2]) ? 1 : 0;
                                       break;
                        case OP_XOR  : regs[inst.r.rd] = regs[inst.r.rs1] ^ regs[inst.r.rs2];
                                       break;
                        case OP_SR   : if (inst.r.func7 == 0)
                                           regs[inst.r.rd] = ((unsigned int)regs[inst.r.rs1]) >> regs[inst.r.rs2];
                                       else
                                           regs[inst.r.rd] = regs[inst.r.rs1] >> regs[inst.r.rs2];
                                       break;
                        case OP_OR   : regs[inst.r.rd] = regs[inst.r.rs1] | regs[inst.r.rs2];
                                       break;
                        case OP_AND  : regs[inst.r.rd] = regs[inst.r.rs1] & regs[inst.r.rs2];
                                       break;
                        default: printf("Unknown instruction at 0x%08x\n", pc);
                                 exit(-1);
                    }
                }
                if (ft) fprintf(ft, "%08x %08x x%02d (%s) <= 0x%08x\n", pc, inst.inst, inst.r.rd, regname[inst.r.rd], regs[inst.r.rd]);
                break;
            }
            case OP_FENCE : {
                if (ft) fprintf(ft, "%08x %08x %s\n", pc, inst.inst, "FENCE");
                break;
            }
            case OP_SYSTEM: { // I-Type
                int val;
                // RDCYCLE, RDTIME and RDINSTRET are read only
                switch(inst.i.func3) {
                    case OP_ECALL  : if (ft) fprintf(ft, "%08x %08x\n", pc, inst.inst);
                                     if (inst.i.imm == 1) { // ebreak
                                        break; // TODO
                                     }
                                     // ecall
                                     switch(regs[A7]) { // function argument A7/X17
                                        case SYS_EXIT:
                                            prog_exit(0);
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
                                                for(i = DVA2PA(regs[A0])/4; i < DVA2PA(regs[A1])/4; i++) {
                                                    fprintf(fp, "%08x\n", dmem[i]);
                                                }
                                                fclose(fp);
                                            }
                                            break;
                                        default:
                                            printf("Unsupport system call 0x%08x\n", regs[17]);
                                            prog_exit(1);
                                     }
                                     break;
                    case OP_CSRRW  : val = regs[inst.i.rs1];    // fall through
                    case OP_CSRRS  : val = regs[inst.i.rs1];    // fall through
                    case OP_CSRRC  : val = regs[inst.i.rs1];    // fall through
                    case OP_CSRRWI : val = inst.i.rs1;          // fall through
                    case OP_CSRRSI : val = inst.i.rs1;          // fall through
                    case OP_CSRRCI : val = inst.i.rs1;
                            regs[inst.i.rd] = csr_rw(inst.i.imm, val);
                            if (ft) fprintf(ft, "%08x %08x x%02d (%s) <= 0x%08x\n",
                                        pc, inst.inst, inst.i.rd, regname[inst.i.rd], regs[inst.i.rd]);
                            break;
                    default: printf("Unknown system instruction at 0x%08x\n", pc);
                             exit(-1);
                }
                break;
            }
            default: {
                printf("Illegal instruction at 0x%08x\n", pc);
                exit(-1);
            }
        }
        pc+=4;
    }

    aligned_free(imem);
    aligned_free(dmem);
    if (ft) fclose(ft);
}

