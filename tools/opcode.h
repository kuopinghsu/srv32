// Written by Kuoping Hsu, 2020, GPL license
typedef union _INST {
    int inst;

    // Byte order
    struct {
        char b0, b1, b2, b3;
    } byte;

    // R-Type
    struct {
        unsigned int op    : 7;
        unsigned int rd    : 5;
        unsigned int func3 : 3;
        unsigned int rs1   : 5;
        unsigned int rs2   : 5;
        unsigned int func7 : 7;
    } r;

    // I-Type
    struct {
        unsigned int op    : 7;
        unsigned int rd    : 5;
        unsigned int func3 : 3;
        unsigned int rs1   : 5;
        unsigned int imm   : 12;
    } i;

    // S-Type
    struct {
        unsigned int op    : 7;
        unsigned int imm1  : 5;
        unsigned int func3 : 3;
        unsigned int rs1   : 5;
        unsigned int rs2   : 5;
        unsigned int imm2  : 7;
    } s;

    // B-Type
    struct {
        unsigned int op    : 7;
        unsigned int imm1  : 5;
        unsigned int func3 : 3;
        unsigned int rs1   : 5;
        unsigned int rs2   : 5;
        unsigned int imm2  : 7;
    } b;

    // U-Type
    struct {
        unsigned int op    : 7;
        unsigned int rd    : 5;
        unsigned int imm   : 20;
    } u;

    // J-Type
    struct {
        unsigned int op    : 7;
        unsigned int rd    : 5;
        unsigned int imm   : 20;
    } j;

} INST;

enum {
    OP_AUIPC   = 0x17,         // U-type
    OP_LUI     = 0x37,         // U-type
    OP_JAL     = 0x6f,         // J-type
    OP_JALR    = 0x67,         // I-type
    OP_BRANCH  = 0x63,         // B-type
    OP_LOAD    = 0x03,         // I-type
    OP_STORE   = 0x23,         // S-type
    OP_ARITHI  = 0x13,         // I-type
    OP_ARITHR  = 0x33,         // R-type
    OP_FENCE   = 0x0f, 
    OP_SYSTEM  = 0x73  
};

enum {
    OP_BEQ     = 0,
    OP_BNE     = 1,
    OP_BLT     = 4,
    OP_BGE     = 5,
    OP_BLTU    = 6,
    OP_BGEU    = 7
};

enum {
    OP_LB      = 0,
    OP_LH      = 1,
    OP_LW      = 2,
    OP_LBU     = 4,
    OP_LHU     = 5
};

enum {
    OP_SB      = 0,
    OP_SH      = 1,
    OP_SW      = 2
};

enum {
    OP_ADD     = 0,
    OP_SLL     = 1,
    OP_SLT     = 2,
    OP_SLTU    = 3,
    OP_XOR     = 4,
    OP_SR      = 5,
    OP_OR      = 6,
    OP_AND     = 7
};

enum {
    OP_MUL     = 0,
    OP_MULH    = 1,
    OP_MULSU   = 2,
    OP_MULU    = 3,
    OP_DIV     = 4,
    OP_DIVU    = 5,
    OP_REM     = 6,
    OP_REMU    = 7
};

enum {
    OP_ECALL   = 0,
    OP_CSRRW   = 1,
    OP_CSRRS   = 2,
    OP_CSRRC   = 3,
    OP_CSRRWI  = 5,
    OP_CSRRSI  = 6,
    OP_CSRRCI  = 7
};

enum {
    CSR_VENDERID    = 0xF11,    // Vender ID
    CSR_MARCHID     = 0xF13,    // Architecture ID
    CSR_IMPLID      = 0xF14,    // Implementation ID
    CSR_MHARDID     = 0xF15,    // Hardware thread ID

    CSR_MSTATUS     = 0x300,    // Machine status register
    CSR_MISA        = 0x301,    // ISA and extensions
    CSR_MEDELEG     = 0x302,    // Machine exception delegation register
    CSR_MIDELEG     = 0x303,    // Machine interrupt delegation register
    CSR_MIE         = 0x304,    // Machine interrupt-enable register
    CSR_MTVEC       = 0x305,    // Machine trap-handler base address
    CSR_MCOUNTEREN  = 0x306,    // Machine counter enable

    CSR_MSCRATCH    = 0x340,    // Scratch register for machine trap handlers
    CSR_MEPC        = 0x341,    // Machine exception program counter
    CSR_MCAUSE      = 0x342,    // Machine trap cause
    CSR_MTVAL       = 0x343,    // Machine bad address or instructions
    CSR_MIP         = 0x344,    // Machine interrupt pending

    CSR_RDCYCLE     = 0xc00,    // cycle counter
    CSR_RDCYCLEH    = 0xc80,    // upper 32-bits of cycle counter
    CSR_RDTIME      = 0xc01,    // timer counter
    CSR_RDTIMEH     = 0xc81,    // upper 32-bits of timer counter
    CSR_RDINSTRET   = 0xc02,    // Intructions-retired counter
    CSR_RDINSTRETH  = 0xc82     // upper 32-bits of intruction-retired counter
};

// system call defined in the file /usr/include/asm-generic/unistd.h
enum {
    SYS_CLOSE   = 0x39,
    SYS_WRITE   = 0x40,
    SYS_FSTAT   = 0x50,
    SYS_EXIT    = 0x5d,
    SYS_SBRK    = 0xd6,
    SYS_DUMP    = 0x88
};

typedef union _counter {
    long long c;
    struct {
        int lo;
        int hi;
    } d;
} COUNTER;

enum {
    ZERO = 0, RA = 1, SP = 2, GP = 3, TP = 4, T0 = 5, T1 = 6, T2 = 7,
    S0 = 8, S1 = 9, A0 = 10, A1 = 11, A2 = 12, A3 = 13, A4 = 14, A5 = 15,
    A6 = 16, A7 = 17, S2 = 18, S3 = 19, S4 = 20, S5 = 21, S6 = 22,
    S7 = 23, S8 = 24, S9 = 25, S10 = 26, S11 = 27, T3 = 28, T4 = 29,
    T5 = 30, T6 = 31
};

#define MMIO_PUTC 0x8000001c
#define MMIO_EXIT 0x8000002c
#define STDOUT 1

#define BRANCH_PENALTY 2

