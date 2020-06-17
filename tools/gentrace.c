// Software simulator for RISC-V RV32I instruction sets
// Written by Kuoping Hsu, 2020, GPL license
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "opcode.h"

#define MAXLEN  1024
COUNTER time;
COUNTER cycle;
COUNTER instret;

char *regname[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0(fp)", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void usage(void) {
    printf(
"Usage: tracegen [-h] [-b n] [-l logfile]\n\n"
"       --help, -n              help\n"
"       --branch n, -b n        branch penalty (default 2)\n"
"       --log file, -l file     generate log file\n"
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

static inline int to_imm_iu(unsigned int n) {
    return (int)(n);
}

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

static void progstate(int exitcode) {
    printf("\nExcuting %lld instructions, %lld cycles\n", instret.c, cycle.c);
    printf("Program terminate\n");
    exit(exitcode);
}

int main(int argc, char **argv) {
    FILE *fi = NULL, *fd = NULL, *ft = NULL;
    static int regs[32];

    int i;
    int pc = 0;
    int *imem;
    int *dmem;
    int imem_addr = 0;
    int dmem_addr = 128*1024;
    int imem_size = 128*1024;
    int dmem_size = 128*1024;
    char *ifile = "../sw/imem.bin";
    char *dfile = "../sw/dmem.bin";
    char *tfile = NULL;
    int isize;
    int dsize;
    int branch_penalty = BRANCH_PENALTY;

    const char *optstring = "hb:l:";
    int c;
    struct option opts[] = {
        {"help", 0, NULL, 'h'},
        {"branch", 1, NULL, 'b'},
        {"log", 1, NULL, 'l'}
    };

    while((c = getopt_long(argc, argv, optstring, opts, NULL)) != -1) {
        switch(c) {
            case 'h':
                usage();
                return 1;
            case 'b':
                branch_penalty = atoi(optarg);
                break;
            case 'l':
                if ((tfile = malloc(MAXLEN)) == NULL) {
                    printf("malloc fail\n");
                    exit(1);
                }
                strncpy(tfile, optarg, MAXLEN);
                break;
            default:
                usage();
                return 1;
        }
    }

    if ((fi=fopen(ifile, "rb")) == NULL) {
        printf("can not open file %s\n", ifile);
        exit(1);
    }
    if ((fd=fopen(dfile, "rb")) == NULL) {
        printf("can not open file %s\n", dfile);
        exit(1);
    }
    if (tfile) {
        if ((ft=fopen(tfile, "w")) == NULL) {
            printf("can not open file %s\n", tfile);
            exit(1);
        }
    }
    if ((imem = (int*)aligned_malloc(sizeof(int), imem_size)) == NULL) {
        printf("malloc fail\n");
        exit(1);
    }
    if ((dmem = (int*)aligned_malloc(sizeof(int), dmem_size)) == NULL) {
        printf("malloc fail\n");
        exit(1);
    }

    isize = fread(imem, sizeof(int), imem_size/sizeof(int), fi);
    dsize = fread(dmem, sizeof(int), dmem_size/sizeof(int), fd);
    isize *= sizeof(int);
    dsize *= sizeof(int);

    if (isize <= 0 || dsize <= 0) {
        printf("file read error\n");
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
        inst.inst = imem[pc/4-imem_addr];
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
                cycle.c += branch_penalty;
                continue;
            }
            case OP_JALR  : { // I-Type
                regs[inst.i.rd] = pc + 4;
                if (ft) fprintf(ft, "%08x %08x x%02d (%s) <= 0x%08x\n", pc, inst.inst, inst.i.rd, regname[inst.i.rd], regs[inst.i.rd]);
                pc = regs[inst.i.rs1] + to_imm_i(inst.i.imm);
                cycle.c += branch_penalty;
                continue;
            }
            case OP_BRANCH: { // B-Type
                if (ft) fprintf(ft, "%08x %08x\n", pc, inst.inst);
                switch(inst.b.func3) {
                    case OP_BEQ  :
                        if (regs[inst.b.rs1] == regs[inst.b.rs2]) {
                            pc += to_imm_b(inst.b.imm2, inst.b.imm1);
                            cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BNE  :
                        if (regs[inst.b.rs1] != regs[inst.b.rs2]) {
                            pc += to_imm_b(inst.b.imm2, inst.b.imm1);
                            cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BLT  :
                        if (regs[inst.b.rs1] < regs[inst.b.rs2]) {
                            pc += to_imm_b(inst.b.imm2, inst.b.imm1);
                            cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BGE  :
                        if (regs[inst.b.rs1] >= regs[inst.b.rs2]) {
                            pc += to_imm_b(inst.b.imm2, inst.b.imm1);
                            cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BLTU :
                        if (((unsigned int)regs[inst.b.rs1]) < ((unsigned int)regs[inst.b.rs2])) {
                            pc += to_imm_b(inst.b.imm2, inst.b.imm1);
                            cycle.c += branch_penalty;
                            continue;
                        }
                        break;
                    case OP_BGEU :
                        if (((unsigned int)regs[inst.b.rs1]) >= ((unsigned int)regs[inst.b.rs2])) {
                            pc += to_imm_b(inst.b.imm2, inst.b.imm1);
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
                if (address < dmem_addr || address >= dmem_addr + dmem_size) {
                    printf("x%0d = 0x%08x, imm = %d\n", inst.i.rs1, regs[inst.i.rs1], to_imm_i(inst.i.imm));
                    printf("memory address 0x%08x out of range\n", address);
                    exit(-1);
                }
                address -= dmem_addr;
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
                if (ft) fprintf(ft, "%08x %08x read 0x%08x => 0x%08x, x%02d (%s) <= 0x%08x\n", pc, inst.inst, address+dmem_addr, dmem[address/4], inst.i.rd, regname[inst.i.rd], regs[inst.i.rd]);
                break;
            }
            case OP_STORE : { // S-Type
                int address = regs[inst.s.rs1] + to_imm_s(inst.s.imm2, inst.s.imm1);
                int data = regs[inst.s.rs2];
                if (ft) fprintf(ft, "%08x %08x write 0x%08x <= 0x%08x\n", pc, inst.inst, address, data);
                if (address < dmem_addr || address > dmem_addr+dmem_size) {
                    switch(address) {
                        case MMIO_PUTC:
                            putchar((char)data);
                            fflush(stdout);
                            pc += 4;
                            continue;
                        case MMIO_EXIT:
                            progstate(data);
                            break;
                        default:
                            printf("Unknown address 0x%08x to write at 0x%08x\n", address, pc);
                            exit(-1);
                    }
                }
                address -= dmem_addr;
                switch(inst.s.func3) {
                    case OP_SB : dmem[address/4] = (dmem[address/4]&~(0xff<<((address&3)*8))) | ((data & 0xff)<<((address&3)*8));
                                 break;
                    case OP_SH : dmem[address/4] = (address&2) ? ((dmem[address/4]&0xffff)|(data << 16)) : ((dmem[address/4]&0xffff0000)|(data&0xffff));
                                 if (address&1) {
                                    printf("Unalignment address 0x%08x to write at 0x%08x\n", address, pc);
                                    exit(-1);
                                 }
                                 break;
                    case OP_SW : dmem[address/4] = data;
                                 if (address&3) {
                                    printf("Unalignment address 0x%08x to write at 0x%08x\n", address, pc);
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
                    case OP_SLTU : regs[inst.i.rd] = regs[inst.i.rs1] < to_imm_iu(inst.i.imm) ? 1 : 0;
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
                if (ft) fprintf(ft, "%08x %08x x%02d (%s) <= 0x%08x\n", pc, inst.inst, inst.r.rd, regname[inst.r.rd], regs[inst.r.rd]);
                break;
            }
            case OP_FENCE : {
                if (ft) fprintf(ft, "%08x %08x %s\n", pc, inst.inst, "FENCE");
                break;
            }
            case OP_SYSTEM: { // I-Type
                // RDCYCLE, RDTIME and RDINSTRET are read only
                switch(inst.i.func3) {
                    case OP_ECALL  : break;
                    case OP_CSRRW  : // fall through
                    case OP_CSRRS  : // fall through
                    case OP_CSRRC  : // fall through
                    case OP_CSRRWI : // fall through
                    case OP_CSRRSI : // fall through
                    case OP_CSRRCI :
                            switch(inst.i.imm) {
                                case CSR_RDCYCLE   : regs[inst.i.rd] = cycle.d.lo;
                                                     break;
                                case CSR_RDCYCLEH  : regs[inst.i.rd] = cycle.d.hi;
                                                     break;
                                case CSR_RDTIME    : regs[inst.i.rd] = time.d.lo;
                                                     break;
                                case CSR_RDTIMEH   : regs[inst.i.rd] = time.d.hi;
                                                     break;
                                case CSR_RDINSTRET : regs[inst.i.rd] = instret.d.lo;
                                                     break;
                                case CSR_RDINSTRETH: regs[inst.i.rd] = instret.d.hi;
                                                     break;
                                default: printf("Unsupport CSR register %d\n", inst.i.imm);
                                         exit(-1);
                            }
                            break;
                    default: printf("Unknown system instruction at 0x%08x\n", pc);
                             exit(-1);
                }
                if (ft) fprintf(ft, "%08x %08x x%02d (%s) <= 0x%08x\n", pc, inst.inst, inst.i.rd, regname[inst.i.rd], regs[inst.i.rd]);
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
    fclose(fi);
    fclose(fd);
    if (ft) fclose(ft);
}

