// Three pipeline stage RV32I RISCV processor
// Written by Kuoping Hsu, 2020, MIT license

`define OPCODE      6:0
`define FUNC3       14:12
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

// FUNC3, INST[14:12], INST[6:0] = 7'b1110011
localparam  [ 2: 0] OP_ECALL   = 3'b000,    // inst[20] == 0: ECALL, inst[20] == 1: EBREAK
                    OP_CSRRW   = 3'b001,
                    OP_CSRRS   = 3'b010,
                    OP_CSRRC   = 3'b011,
                    OP_CSRRWI  = 3'b101,
                    OP_CSRRSI  = 3'b110,
                    OP_CSRRCI  = 3'b111;

// CSR registers
localparam  [12: 0] CSR_RDCYCLE    = 12'hc00,
                    CSR_RDCYCLEH   = 12'hc80,
                    CSR_RDTIME     = 12'hc01,
                    CSR_RDTIMEH    = 12'hc81,
                    CSR_RDINSTRET  = 12'hc02,
                    CSR_RDINSTRETH = 12'hc82;

