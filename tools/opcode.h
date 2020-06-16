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
    OP_ECALL   = 0,
    OP_CSRRW   = 1,
    OP_CSRRS   = 2,
    OP_CSRRC   = 3,
    OP_CSRRWI  = 5,
    OP_CSRRSI  = 6,
    OP_CSRRCI  = 7
};

enum {
    CSR_RDCYCLE    = 0xc00,
    CSR_RDCYCLEH   = 0xc80,
    CSR_RDTIME     = 0xc01,
    CSR_RDTIMEH    = 0xc81,
    CSR_RDINSTRET  = 0xc02,
    CSR_RDINSTRETH = 0xc82
};

typedef union _counter {
    long long c;
    struct {
        int lo;
        int hi;
    } d;
} COUNTER;

#define MMIO_PUTC 0x8000001c
#define MMIO_EXIT 0x8000002c

#define BRANCH_PENALTY 2

