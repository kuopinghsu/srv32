// Three pipeline stage RV32I RISCV processor
// Written by Kuoping Hsu, 2020, MIT license

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
localparam  [11: 0] CSR_RDCYCLE    = 12'hc00,
                    CSR_RDCYCLEH   = 12'hc80,
                    CSR_RDTIME     = 12'hc01,
                    CSR_RDTIMEH    = 12'hc81,
                    CSR_RDINSTRET  = 12'hc02,
                    CSR_RDINSTRETH = 12'hc82;

// system call defined in the file /usr/include/asm-generic/unistd.h
localparam  [ 7: 0] SYS_CLOSE   = 8'h39,
                    SYS_WRITE   = 8'h40,
                    SYS_FSTAT   = 8'h50,
                    SYS_EXIT    = 8'h5d,
                    SYS_SBRK    = 8'hd6;

// Register/ABI mapping
localparam  [ 4: 0] REG_ZERO =  0, REG_RA =  1, REG_SP  =  2, REG_GP  =  3,
                    REG_TP   =  4, REG_T0 =  5, REG_T1  =  6, REG_T2  =  7,
                    REG_S0   =  8, REG_S1 =  9, REG_A0  = 10, REG_A1  = 11,
                    REG_A2   = 12, REG_A3 = 13, REG_A4  = 14, REG_A5  = 15,
                    REG_A6   = 16, REG_A7 = 17, REG_S2  = 18, REG_S3  = 19,
                    REG_S4   = 20, REG_S5 = 21, REG_S6  = 22, REG_S7  = 23,
                    REG_S8   = 24, REG_S9 = 25, REG_S10 = 26, REG_S11 = 27,
                    REG_T3   = 28, REG_T4 = 29, REG_T5  = 30, REG_T6  = 31;
