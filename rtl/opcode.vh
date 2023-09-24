// Copyright © 2020 Kuoping Hsu
// Three pipeline stage RV32IM RISCV processor
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

`define OPCODE      6:0
`define FUNC3       14:12
`define FUNC7       31:25
`define SUBTYPE     30
`define RD          11:7
`define RS1         19:15
`define RS2         24:20
`define IMM12       31:20

localparam  [31: 0] RESETVEC   = 32'h0000_0000;

localparam  [31: 0] NOP        = 32'h0000_0013;     // addi x0, x0, 0

// OPCODE, INST[6:0]
localparam  [ 6: 0] OP_AUIPC   = 7'b0010111,        // U-type
                    OP_LUI     = 7'b0110111,        // U-type
                    OP_JAL     = 7'b1101111,        // J-type
                    OP_JALR    = 7'b1100111,        // I-type
                    OP_BRANCH  = 7'b1100011,        // B-type
                    OP_LOAD    = 7'b0000011,        // I-type
                    OP_STORE   = 7'b0100011,        // S-type
                    OP_ARITHI  = 7'b0010011,        // I-type
                    OP_ARITHR  = 7'b0110011,        // R-type
                    OP_FENCE   = 7'b0001111,
                    OP_SYSTEM  = 7'b1110011;


// FUNC3, INST[14:12], INST[6:0] = 7'b1100011
localparam  [ 2: 0] OP_BEQ     = 3'b000,
                    OP_BNE     = 3'b001,
                    OP_BLT     = 3'b100,
                    OP_BGE     = 3'b101,
                    OP_BLTU    = 3'b110,
                    OP_BGEU    = 3'b111;

// FUNC3, INST[14:12], INST[6:0] = 7'b0000011
localparam  [ 2: 0] OP_LB      = 3'b000,
                    OP_LH      = 3'b001,
                    OP_LW      = 3'b010,
                    OP_LBU     = 3'b100,
                    OP_LHU     = 3'b101;

// FUNC3, INST[14:12], INST[6:0] = 7'b0100011
localparam  [ 2: 0] OP_SB      = 3'b000,
                    OP_SH      = 3'b001,
                    OP_SW      = 3'b010;

// FUNC3, INST[14:12], INST[6:0] = 7'b0110011, 7'b0010011
localparam  [ 2: 0] OP_ADD     = 3'b000,    // inst[30] == 0: ADD, inst[31] == 1: SUB
                    OP_SLL     = 3'b001,
                    OP_SLT     = 3'b010,
                    OP_SLTU    = 3'b011,
                    OP_XOR     = 3'b100,
                    OP_SR      = 3'b101,    // inst[30] == 0: SRL, inst[31] == 1: SRA
                    OP_OR      = 3'b110,
                    OP_AND     = 3'b111;

// FUNC3, INST[14:12], INST[6:0] = 7'b0110011, FUNC7 INST[31:25] == 0x01
localparam  [ 2: 0] OP_MUL     = 3'b000,
                    OP_MULH    = 3'b001,
                    OP_MULSU   = 3'b010,
                    OP_MULU    = 3'b011,
                    OP_DIV     = 3'b100,
                    OP_DIVU    = 3'b101,
                    OP_REM     = 3'b110,
                    OP_REMU    = 3'b111;

// FUNC3, INST[14:12], INST[6:0] = 7'b1110011
localparam  [ 2: 0] OP_ECALL   = 3'b000,    // inst[20] == 0: ECALL, inst[20] == 1: EBREAK
                    OP_CSRRW   = 3'b001,
                    OP_CSRRS   = 3'b010,
                    OP_CSRRC   = 3'b011,
                    OP_CSRRWI  = 3'b101,
                    OP_CSRRSI  = 3'b110,
                    OP_CSRRCI  = 3'b111;

// CSR registers
localparam  [11: 0] CSR_MVENDORID   = 12'hF11,    // Vender ID
                    CSR_MARCHID     = 12'hF12,    // Architecture ID
                    CSR_MIMPID      = 12'hF13,    // Implementation ID
                    CSR_MHARTID     = 12'hF14,    // Hardware thread ID

                    CSR_MSTATUS     = 12'h300,    // Machine status register
                    CSR_MISA        = 12'h301,    // ISA and extensions
                    CSR_MEDELEG     = 12'h302,    // Machine exception delegation register
                    CSR_MIDELEG     = 12'h303,    // Machine interrupt delegation register
                    CSR_MIE         = 12'h304,    // Machine interrupt-enable register
                    CSR_MTVEC       = 12'h305,    // Machine trap-handler base address
                    CSR_MCOUNTEREN  = 12'h306,    // Machine counter enable

                    CSR_MSCRATCH    = 12'h340,    // Scratch register for machine trap handlers
                    CSR_MEPC        = 12'h341,    // Machine exception program counter
                    CSR_MCAUSE      = 12'h342,    // Machine trap cause
                    CSR_MTVAL       = 12'h343,    // Machine bad address or instructions
                    CSR_MIP         = 12'h344,    // Machine interrupt pending

                    CSR_SSTATUS     = 12'h100,    // Supervisor status register
                    CSR_SIE         = 12'h104,    // Supervisor interrupt-enable register
                    CSR_STVEC       = 12'h105,    // Supervisor trap handler base address
                    CSR_SSCRATCH    = 12'h140,    // Scratch register for supervisor trap handlers
                    CSR_SEPC        = 12'h141,    // Supervisor exception program counter
                    CSR_SCAUSE      = 12'h142,    // Supervisor trap cause
                    CSR_STVAL       = 12'h143,    // Supervisor bad address or instruction
                    CSR_SIP         = 12'h144,    // Supervisor interrupt pending
                    CSR_SATP        = 12'h180,    // Supervisor address translation and protection

                    CSR_RDCYCLE     = 12'hc00,    // cycle counter
                    CSR_RDCYCLEH    = 12'hc80,    // upper 32-bits of cycle counter
                    CSR_RDTIME      = 12'hc01,    // timer counter
                    CSR_RDTIMEH     = 12'hc81,    // upper 32-bits of timer counter
                    CSR_RDINSTRET   = 12'hc02,    // Instructions-retired counter
                    CSR_RDINSTRETH  = 12'hc82;    // upper 32-bits of instruction-retired counter

// system call defined in the file /usr/include/asm-generic/unistd.h
localparam  [31: 0] SYS_OPEN        = 32'hbeef0031,
                    SYS_LSEEK       = 32'hbeef0032,
                    SYS_CLOSE       = 32'hbeef0039,
                    SYS_READ        = 32'hbeef003f,
                    SYS_WRITE       = 32'hbeef0040,
                    SYS_FSTAT       = 32'hbeef0050,
                    SYS_EXIT        = 32'hbeef005d,
                    SYS_SBRK        = 32'hbeef00d6,
                    SYS_DUMP        = 32'hbeef0088,
                    SYS_DUMP_BIN    = 32'hbeef0099;

// Exception code
localparam  [31: 0] TRAP_INST_ALIGN = 32'h0,        // Instruction address misaligned
                    TRAP_INST_FAIL  = 32'h1,        // Instruction access fault
                    TRAP_INST_ILL   = 32'h2,        // Illegal instruction
                    TRAP_BREAK      = 32'h3,        // Breakpoint
                    TRAP_LD_ALIGN   = 32'h4,        // Load address misaligned
                    TRAP_LD_FAIL    = 32'h5,        // Load access fault
                    TRAP_ST_ALIGN   = 32'h6,        // Store/AMO address misaligned
                    TRAP_ST_FAIL    = 32'h7,        // Store/AMO access fault
                    TRAP_ECALL      = 32'hb,        // Environment call from M-mode
                    INT_USI         = 1<<31|32'h0,  // User software interrupt
                    INT_SSI         = 1<<31|32'h1,  // Supervisor software interrupt
                    INT_MSI         = 1<<31|32'h3,  // Machine software interrupt
                    INT_UTIME       = 1<<31|32'h4,  // User timer interrupt
                    INT_STIME       = 1<<31|32'h5,  // Supervisor timer interrupt
                    INT_MTIME       = 1<<31|32'h7,  // Machine timer interrupt
                    INT_UEI         = 1<<31|32'h8,  // User external interrupt
                    INT_SEI         = 1<<31|32'h9,  // Supervisor external interrupt
                    INT_MEI         = 1<<31|32'hb;  // Machine external interrupt

// mstatus register
localparam  [ 4: 0] UIE             = 5'd0,     // U-mode global interrupt enable
                    SIE             = 5'd1,     // S-mode global interrupt enable
                    MIE             = 5'd3,     // M-mode global interrupt enable
                    UPIE            = 5'd4,     // U-mode
                    SPIE            = 5'd5,     // S-mode
                    MPIE            = 5'd7,     // M-mode
                    SPP             = 5'd8,     // S-mode hold the previous privilege mode
                    MPP             = 5'd11,    // MPP[1:0] M-mode hold the previous privilege mode
                    FS              = 5'd13,    // FS[1:0]
                    XS              = 5'd15,    // XS[1:0]
                    MPRV            = 5'd17,    // memory privilege
                    SUM             = 5'd18,
                    MXR             = 5'd19,
                    TVM             = 5'd20,
                    TW              = 5'd21,
                    TSR             = 5'd22;

// mie register
localparam  [ 4: 0] USIE            = 5'd0,     // U-mode Software Interrupt Enable
                    SSIE            = 5'd1,     // S-mode Software Interrupt Enable
                    MSIE            = 5'd3,     // M-mode Software Interrupt Enable
                    UTIE            = 5'd4,     // U-mode Timer Interrupt Enable
                    STIE            = 5'd5,     // S-mode Timer Interrupt Enable
                    MTIE            = 5'd7,     // M-mode Timer Interrupt Enable
                    UEIE            = 5'd8,     // U-mode External Interrupt Enable
                    SEIE            = 5'd9,     // S-mode External Interrupt Enable
                    MEIE            = 5'd11;    // M-mode External Interrupt Enable

// mip register
localparam  [ 4: 0] USIP            = 5'd0,     // U-mode Software Interrupt Pending
                    SSIP            = 5'd1,     // S-mode Software Interrupt Pending
                    MSIP            = 5'd3,     // M-mode Software Interrupt Pending
                    UTIP            = 5'd4,     // U-mode Timer Interrupt Pending
                    STIP            = 5'd5,     // S-mode Timer Interrupt Pending
                    MTIP            = 5'd7,     // M-mode Timer Interrupt Pending
                    UEIP            = 5'd8,     // U-mode External Interrupt Pending
                    SEIP            = 5'd9,     // S-mode External Interrupt Pending
                    MEIP            = 5'd11;    // M-mode External Interrupt Pending

// Register/ABI mapping
localparam  [ 4: 0] REG_ZERO =  0, REG_RA =  1, REG_SP  =  2, REG_GP  =  3,
                    REG_TP   =  4, REG_T0 =  5, REG_T1  =  6, REG_T2  =  7,
                    REG_S0   =  8, REG_S1 =  9, REG_A0  = 10, REG_A1  = 11,
                    REG_A2   = 12, REG_A3 = 13, REG_A4  = 14, REG_A5  = 15,
                    REG_A6   = 16, REG_A7 = 17, REG_S2  = 18, REG_S3  = 19,
                    REG_S4   = 20, REG_S5 = 21, REG_S6  = 22, REG_S7  = 23,
                    REG_S8   = 24, REG_S9 = 25, REG_S10 = 26, REG_S11 = 27,
                    REG_T3   = 28, REG_T4 = 29, REG_T5  = 30, REG_T6  = 31;

localparam  [31: 0] MVENDORID = 32'h0,
                    MARCHID   = 32'h0,
                    MIMPID    = 32'h0,
                    MHARTID   = 32'h0;

localparam  [ 3: 0] MMIO_BASE     = 4'h9;
localparam  [31: 0] MTIME_BASE    = 32'h9000_0000,
                    MTIMECMP_BASE = 32'h9000_0008,
                    MSIP_BASE     = 32'h9000_0010,
                    MMIO_PUTC     = 32'h9000_001c,
                    MMIO_GETC     = 32'h9000_0020,
                    MMIO_EXIT     = 32'h9000_002c;

