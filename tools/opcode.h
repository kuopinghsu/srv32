// Copyright © 2020 Kuoping Hsu
// opcode.h
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

#ifndef __OPCODE_H__
#define __OPCODE_H__

#define _CRT_SECURE_NO_WARNINGS 1

#if defined(__MINGW__) || defined(_MSC_VER)
#define __STDC_WANT_LIB_EXT1__ 1
#endif

typedef union _INSTC {
    short int inst;

    // byte order
    struct {
        char b0, b1;
    } byte;

    // CR-Type, compressed register type
    struct {
        unsigned int op    : 2;
        unsigned int rs2   : 5;
        unsigned int rd    : 5;
        unsigned int func4 : 4;
    } cr;

    // CI-Type, compressed intermedia type
    struct {
        unsigned int op    : 2;
        unsigned int imm_l : 5;
        unsigned int rd    : 5;
        unsigned int imm_h : 1;
        unsigned int func3 : 3;
    } ci;

    // CSS-Type, compressed stack-relative store type
    struct {
        unsigned int op    : 2;
        unsigned int rs2   : 5;
        unsigned int imm   : 6;
        unsigned int func3 : 3;
    } css;

    // CIW-Type, compressed wide intermedia type
    struct {
        unsigned int op    : 2;
        unsigned int rd_   : 3;
        unsigned int imm   : 8;
        unsigned int func3 : 3;
    } ciw;

    // CL-Type, compressed load type
    struct {
        unsigned int op    : 2;
        unsigned int rd_   : 3;
        unsigned int imm_l : 2;
        unsigned int rs1_  : 3;
        unsigned int imm_h : 3;
        unsigned int func3 : 3;
    } cl;

    // CS-Type, compressed store type
    struct {
        unsigned int op    : 2;
        unsigned int rs2_  : 3;
        unsigned int imm_l : 2;
        unsigned int rs1_  : 3;
        unsigned int imm_h : 3;
        unsigned int func3 : 3;
    } cs;

    // CA-Type, compressed arithmetic type
    struct {
        unsigned int op    : 2;
        unsigned int rs2_  : 3;
        unsigned int func2 : 2;
        unsigned int rd_   : 3;
        unsigned int func6 : 6;
    } ca;

    // CB-Type
    struct {
        unsigned int op    : 2;
        unsigned int imm_l : 5;
        unsigned int rs1_  : 3;
        unsigned int imm_h : 3;
        unsigned int func3 : 3;
    } cb;

    // CJ-Type
    struct {
        unsigned int op    : 2;
        unsigned int offset: 11;
        unsigned int func3 : 3;
    } cj;

} INSTC;

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
    OP_CLWSP        = 2,
    OP_CSWSP        = 6,
    OP_CLW          = 2,
    OP_CSW          = 6,
    OP_CJ           = 5,
    OP_CJAL         = 1,
    OP_CJR          = 4,
    OP_CJALR        = 4,
    OP_CBEQZ        = 6,
    OP_CBNEZ        = 7,
    OP_CLI          = 2,
    OP_CLUI         = 3,
    OP_CADDI        = 0,
    OP_CADDI16SP    = 3,
    OP_CADDI4SPN    = 0,
    OP_CSLLI        = 0,
    OP_CSRLI        = 4,
    OP_CSRAI        = 4,
    OP_CANDI        = 4,
    OP_CMV          = 4,
    OP_CADD         = 4,
    OP_CAND         = 4,
    OP_COR          = 4,
    OP_CXOR         = 4,
    OP_CSUB         = 4,
    OP_CNOP         = 0,
    OP_CEBREAK      = 4
};

enum {
    CSR_MVENDORID   = 0xF11,    // Vender ID
    CSR_MARCHID     = 0xF12,    // Architecture ID
    CSR_MIMPID      = 0xF13,    // Implementation ID
    CSR_MHARTID     = 0xF14,    // Hardware thread ID

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

    CSR_SSTATUS     = 0x100,    // Supervisor status register
    CSR_SIE         = 0x104,    // Supervisor interrupt-enable register
    CSR_STVEC       = 0x105,    // Supervisor trap handler base address
    CSR_SSCRATCH    = 0x140,    // Scratch register for supervisor trap handlers
    CSR_SEPC        = 0x141,    // Supervisor exception program counter
    CSR_SCAUSE      = 0x142,    // Supervisor trap cause
    CSR_STVAL       = 0x143,    // Supervisor bad address or instruction
    CSR_SIP         = 0x144,    // Supervisor interrupt pending
    CSR_SATP        = 0x180,    // Supervisor address translation and protection

    CSR_RDCYCLE     = 0xc00,    // cycle counter
    CSR_RDCYCLEH    = 0xc80,    // upper 32-bits of cycle counter
    CSR_RDTIME      = 0xc01,    // timer counter
    CSR_RDTIMEH     = 0xc81,    // upper 32-bits of timer counter
    CSR_RDINSTRET   = 0xc02,    // Intructions-retired counter
    CSR_RDINSTRETH  = 0xc82     // upper 32-bits of intruction-retired counter
};

// system call defined in the file /usr/include/asm-generic/unistd.h
enum {
    SYS_OPEN        = 0xbeef0031,
    SYS_LSEEK       = 0xbeef0032,
    SYS_CLOSE       = 0xbeef0039,
    SYS_READ        = 0xbeef003f,
    SYS_WRITE       = 0xbeef0040,
    SYS_FSTAT       = 0xbeef0050,
    SYS_EXIT        = 0xbeef005d,
    SYS_SBRK        = 0xbeef00d6,
    SYS_DUMP        = 0xbeef0088,
    SYS_DUMP_BIN    = 0xbeef0099
};

// Exception code
enum {
    TRAP_INST_ALIGN = 0,            // Instruction address misaligned
    TRAP_INST_FAIL  = 1,            // Instruction access fault
    TRAP_INST_ILL   = 2,            // Illegal instruction
    TRAP_BREAK      = 3,            // Breakpoint
    TRAP_LD_ALIGN   = 4,            // Load address misaligned
    TRAP_LD_FAIL    = 5,            // Load access fault
    TRAP_ST_ALIGN   = 6,            // Store/AMO address misaligned
    TRAP_ST_FAIL    = 7,            // Store/AMO access fault
    TRAP_ECALL      = 11,           // Environment call from M-mode

    INT_USI         = (1<<31)|0,    // User software interrupt
    INT_SSI         = (1<<31)|1,    // Supervisor software interrupt
    INT_MSI         = (1<<31)|3,    // Machine software interrupt
    INT_UTIME       = (1<<31)|4,    // User timer interrupt
    INT_STIME       = (1<<31)|5,    // Supervisor timer interrupt
    INT_MTIME       = (1<<31)|7,    // Machine timer interrupt
    INT_UEI         = (1<<31)|8,    // User external interrupt
    INT_SEI         = (1<<31)|9,    // Supervisor external interrupt
    INT_MEI         = (1<<31)|11    // Machine external interrupt
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

// mstatus register
enum {
    UIE     = 0,        // U-mode global interrupt enable
    SIE     = 1,        // S-mode global interrupt enable
//  WPRI    = 2,
    MIE     = 3,        // M-mode global interrupt enable
    UPIE    = 4,        // U-mode
    SPIE    = 5,        // S-mode
//  WPRI    = 6,
    MPIE    = 7,        // M-mode
    SPP     = 8,        // S-mode hold the previous privilege mode
//  WPRI    = 9, 10
    MPP     = 11,       // MPP[1:0] M-mode hold the previous privilege mode
    FS      = 13,       // FS[1:0]
    XS      = 15,       // XS[1:0]
    MPRV    = 17,       // memory privilege
    SUM     = 18,
    MXR     = 19,
    TVM     = 20,
    TW      = 21,
    TSR     = 22,
//  WPRI    = 31:23
};

// mie register
enum {
    USIE    = 0,        // U-mode Software Interrupt Enable
    SSIE    = 1,        // S-mode Software Interrupt Enable
//  WPRI    = 2,        // Reserved
    MSIE    = 3,        // M-mode Software Interrupt Enable
    UTIE    = 4,        // U-mode Timer Interrupt Enable
    STIE    = 5,        // S-mode Timer Interrupt Enable
//  WPRI    = 6,        // Reserved
    MTIE    = 7,        // M-mode Timer Interrupt Enable
    UEIE    = 8,        // U-mode External Interrupt Enable
    SEIE    = 9,        // S-mode External Interrupt Enable
//  WPRI    = 10,       // Reserved
    MEIE    = 11        // M-mode External Interrupt Enable
//  WPRI    = 12 ... MXLEN-1 // Reserved
};

// mip register
enum {
    USIP    = 0,        // U-mode Software Interrupt Pending
    SSIP    = 1,        // S-mode Software Interrupt Pending
//  WPRI    = 2,        // Reserved
    MSIP    = 3,        // M-mode Software Interrupt Pending
    UTIP    = 4,        // U-mode Timer Interrupt Pending
    STIP    = 5,        // S-mode Timer Interrupt Pending
//  WPRI    = 6,        // Reserved
    MTIP    = 7,        // M-mode Timer Interrupt Pending
    UEIP    = 8,        // U-mode External Interrupt Pending
    SEIP    = 9,        // S-mode External Interrupt Pending
//  WPRI    = 10,       // Reserved
    MEIP    = 11        // M-mode External Interrupt Pending
//  WPRI    = 12 ... MXLEN-1 // Reserved
};

// The privileged mode
enum {
    UMODE   = 0,        // User mode
    SMODE   = 1,        // Supervisor mode
    MMODE   = 2         // Machine mode
};

#define MVENDORID     0
#define MARCHID       0
#define MIMPID        0
#define MHARTID       0
#define MMIO_PUTC     0x9000001c
#define MMIO_GETC     0x90000020
#define MMIO_EXIT     0x9000002c
#define MMIO_MTIME    0x90000000
#define MMIO_MTIMECMP 0x90000008
#define MMIO_MSIP     0x90000010
#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define BRANCH_PENALTY 2

#if defined __APPLE__
#  define strncpy_s(a,b,c,d) strlcpy(a,c,d)
#  define strncat_s(a,b,c,d) strlcat(a,c,d)
#elif !defined(__STDC_WANT_LIB_EXT1__)
#  define strncpy_s __strncpy_s
#  define strncat_s __strncat_s
char *strncpy_s(char *dest, size_t n, const char *src, size_t count);
char *strncat_s(char *dest, size_t n, const char *src, size_t count);
#endif

int compressed_decoder (
    INSTC instc,
    INST  *inst,
    int   *illegal
);

#define IMEM_BASE   (0+mem_base)
#define DMEM_BASE   (mem_size+mem_base)
#define IMEM_SIZE   mem_size
#define DMEM_SIZE   mem_size

#define IVA2PA(addr) ((addr)-IMEM_BASE)
#define IPA2VA(addr) ((addr)+IMEM_BASE)
#define DVA2PA(addr) ((addr)-DMEM_BASE)
#define DPA2VA(addr) ((addr)+DMEM_BASE)

#endif // __OPCODE_H__

