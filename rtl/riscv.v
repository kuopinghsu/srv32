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

// Define it to enable RV32M multiply extension
`define RV32M_ENABLED       1

// ============================================================
// RISCV for imem and dmem separate port
// ============================================================
module riscv (
    input                   clk,
    input                   resetb,

    input                   stall,
    output reg              exception,
    output                  timer_en,

    // interrupt
    input                   timer_irq,
    input                   sw_irq,
    input                   interrupt,

    // interface of instruction RAM
    output                  imem_ready,
    input                   imem_valid,
    output          [31: 0] imem_addr,
    input                   imem_rresp,
    input           [31: 0] imem_rdata,

    // interface of data RAM
    output                  dmem_wready,
    input                   dmem_wvalid,
    output          [31: 0] dmem_waddr,
    output          [31: 0] dmem_wdata,
    output          [ 3: 0] dmem_wstrb,

    output                  dmem_rready,
    input                   dmem_rvalid,
    output          [31: 0] dmem_raddr,
    input                   dmem_rresp,
    input           [31: 0] dmem_rdata
);

`include "opcode.vh"

`define IF_NEXT_PC (4)
`define EX_NEXT_PC (4)

    reg                     stall_r;
    wire            [31: 0] inst;
    reg                     flush;
    reg             [ 1: 0] pipefill;

    wire                    if_stall;
    wire                    ex_stall;
    wire                    wb_stall;

    reg             [31: 0] fetch_pc;
    reg             [31: 0] if_pc;
    reg             [31: 0] ex_pc;
    reg             [31: 0] wb_pc;
    reg             [31: 0] if_insn;

    // register files
    reg             [31: 0] regs [31: 1];
    wire            [31: 0] reg_rdata1, reg_rdata2;
    wire            [31: 0] alu_op1;
    wire            [31: 0] alu_op2;

    reg             [31: 0] ex_insn;
    reg             [31: 0] ex_imm;
    reg                     ex_imm_sel;
    reg             [ 4: 0] ex_src1_sel;
    reg             [ 4: 0] ex_src2_sel;
    reg             [ 4: 0] ex_dst_sel;
    reg             [ 2: 0] ex_alu_op;
    reg                     ex_subtype;
    reg                     ex_memwr;
    reg                     ex_mem2reg;
    wire            [31: 0] ex_memaddr;
    wire            [31: 1] ex_ret_pc;
    reg                     ex_alu;
    reg                     ex_csr;
    reg                     ex_csr_wr;
    reg                     ex_lui;
    reg                     ex_auipc;
    reg                     ex_jal;
    reg                     ex_jalr;
    reg                     ex_branch;
    reg                     ex_system;
    reg                     ex_system_op;
    wire                    ex_systemcall;
    wire                    ex_flush;
    reg             [31: 0] ex_csr_read;
    wire                    ex_trap;
    wire            [31: 0] ex_trap_pc;
    wire            [31: 0] ex_csr_data;
    reg             [31: 0] ex_mcause;
    reg                     ex_illegal;
    reg                     ex_ill_branch;
    wire                    ex_ld_align_excp;
    wire                    ex_st_align_excp;
    wire                    ex_inst_ill_excp;
    wire                    ex_inst_align_excp;
    wire                    ex_timer_irq;
    wire                    ex_sw_irq;
    wire                    ex_interrupt;

`ifdef RV32M_ENABLED
    reg                     ex_mul;
`endif // RV32M_ENABLED

    reg                     wb_alu2reg;
    reg             [31: 0] wb_result;
    reg             [ 2: 0] wb_alu_op;
    reg                     wb_memwr;
    reg                     wb_mem2reg;
    reg             [ 4: 0] wb_dst_sel;
    reg                     wb_branch;
    reg                     wb_branch_nxt;
    reg                     wb_nop;
    reg                     wb_nop_more;
    reg             [31: 0] wb_waddr;
    reg             [ 1: 0] wb_raddr;
    reg             [ 3: 0] wb_wstrb;
    reg             [31: 0] wb_wdata;
    reg             [31: 0] wb_rdata;
    wire                    wb_flush;

    reg                     ex_ill_csr;

    reg             [63: 0] csr_cycle;
    reg             [63: 0] csr_instret;

    reg             [31: 0] csr_mscratch;
    reg             [31: 0] csr_mstatus;
    reg             [31: 0] csr_misa;
    reg             [31: 0] csr_mie;
    reg             [31: 0] csr_mip;
    reg             [31: 0] csr_mtvec;
    reg             [31: 0] csr_mepc;
    reg             [31: 0] csr_mcause;
    reg             [31: 0] csr_mtval;

    integer                 i;

assign if_insn              = imem_rdata;

assign inst                 = flush ? NOP : if_insn;
assign if_stall             = stall_r || !imem_valid;
assign dmem_waddr           = wb_waddr;
assign dmem_raddr           = ex_memaddr;
assign dmem_rready          = ex_mem2reg;
assign dmem_wready          = wb_memwr;
assign dmem_wdata           = wb_wdata;
assign dmem_wstrb           = wb_wstrb;

always @(posedge clk or negedge resetb) begin
    if (!resetb)
        exception           <= 1'b0;
    else if (ex_inst_ill_excp || ex_inst_align_excp ||
             ex_ld_align_excp || ex_st_align_excp)
        exception           <= 1'b1;
end

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        stall_r             <= 1'b1;
        flush               <= 1'b1;
    end else begin
        stall_r             <= stall;
        flush               <= stall_r;
    end
end

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        if_pc               <= RESETVEC;
    end else if (!wb_stall) begin
        if_pc               <= fetch_pc;
    end
end

////////////////////////////////////////////////////////////
//      F/D  E   W
//          F/D  E   W
//              F/D  E  W
//                  F/D E  w
////////////////////////////////////////////////////////////
// stage 1: fetch/decode
////////////////////////////////////////////////////////////
    reg             [31: 0] imm;

always @* begin
    case(inst[`OPCODE])
        OP_AUIPC : imm      = {inst[31:12], 12'd0}; // U-type
        OP_LUI   : imm      = {inst[31:12], 12'd0}; // U-type
        OP_JAL   : imm      = {{12{inst[31]}}, inst[19:12], inst[20], inst[30:21], 1'b0}; // J-type
        OP_JALR  : imm      = {{20{inst[31]}}, inst[31:20]}; // I-Type
        OP_BRANCH: imm      = {{20{inst[31]}}, inst[7], inst[30:25], inst[11:8], 1'b0}; // B-type
        OP_LOAD  : imm      = {{20{inst[31]}}, inst[31:20]}; // I-type
        OP_STORE : imm      = {{20{inst[31]}}, inst[31:25], inst[11:7]}; // S-type
        OP_ARITHI: imm      = (inst[`FUNC3] == OP_SLL || inst[`FUNC3] == OP_SR) ?
                              {27'h0, inst[24:20]} : {{20{inst[31]}}, inst[31:20]}; // I-type
        OP_ARITHR: imm      = 'd0; // R-type
        OP_FENCE : imm      = 'd0;
        OP_SYSTEM: imm      = {20'h0, inst[31:20]};
        default  : imm      = 'd0;
    endcase
end

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        ex_imm              <= 32'h0;
        ex_imm_sel          <= 1'b0;
        ex_src1_sel         <= 5'h0;
        ex_src2_sel         <= 5'h0;
        ex_dst_sel          <= 5'h0;
        ex_alu_op           <= 3'h0;
        ex_subtype          <= 1'b0;
        ex_memwr            <= 1'b0;
        ex_alu              <= 1'b0;
        ex_csr              <= 1'b0;
        ex_csr_wr           <= 1'b0;
        ex_lui              <= 1'b0;
        ex_auipc            <= 1'b0;
        ex_jal              <= 1'b0;
        ex_jalr             <= 1'b0;
        ex_branch           <= 1'b0;
        ex_system           <= 1'b0;
        ex_system_op        <= 1'b0;
        ex_pc               <= RESETVEC;
        ex_illegal          <= 1'b0;
        `ifdef RV32M_ENABLED
        ex_mul              <= 1'b0;
        `endif // RV32M_ENABLED
    end else if (!if_stall) begin
        ex_imm              <= imm;
        ex_imm_sel          <= (inst[`OPCODE] == OP_JALR  ) ||
                               (inst[`OPCODE] == OP_LOAD  ) ||
                               (inst[`OPCODE] == OP_ARITHI);
        ex_src1_sel         <= inst[`RS1];
        ex_src2_sel         <= inst[`RS2];
        ex_dst_sel          <= inst[`RD];
        ex_alu_op           <= inst[`FUNC3];
        ex_subtype          <= inst[`SUBTYPE] &&
                               !(inst[`OPCODE] == OP_ARITHI && inst[`FUNC3] == OP_ADD);
        ex_memwr            <= inst[`OPCODE] == OP_STORE;
        ex_alu              <= (inst[`OPCODE] == OP_ARITHI) ||
                               ((inst[`OPCODE] == OP_ARITHR) &&
                                (inst[`FUNC7] == 'h00 || inst[`FUNC7] == 'h20));
        ex_csr              <= (inst[`OPCODE] == OP_SYSTEM) &&
                               (inst[`FUNC3] != OP_ECALL);
                               // CSRRS and CSRRC, if rs1==0, then the instruction
                               // will not write to the CSR at all
        ex_csr_wr           <= (inst[`OPCODE] == OP_SYSTEM) &&
                               (inst[`FUNC3] != OP_ECALL) &&
                               !(inst[`FUNC3] != OP_CSRRW && inst[`FUNC3] != OP_CSRRWI &&
                                 inst[`RS1] == 5'h0);
        ex_lui              <= inst[`OPCODE] == OP_LUI;
        ex_auipc            <= inst[`OPCODE] == OP_AUIPC;
        ex_jal              <= inst[`OPCODE] == OP_JAL;
        ex_jalr             <= inst[`OPCODE] == OP_JALR;
        ex_branch           <= inst[`OPCODE] == OP_BRANCH;
        ex_system           <= (inst[`OPCODE] == OP_SYSTEM) &&
                               (inst[`FUNC3] == 3'b000);
        ex_system_op        <= inst[`OPCODE] == OP_SYSTEM;
        ex_pc               <= if_pc;
        ex_illegal          <= !((inst[`OPCODE] == OP_AUIPC )||
                                 (inst[`OPCODE] == OP_LUI   )||
                                 (inst[`OPCODE] == OP_JAL   )||
                                 (inst[`OPCODE] == OP_JALR  )||
                                 (inst[`OPCODE] == OP_BRANCH)||
                                 ((inst[`OPCODE] == OP_LOAD ) &&
                                  ((inst[`FUNC3] == OP_LB) ||
                                   (inst[`FUNC3] == OP_LH) ||
                                   (inst[`FUNC3] == OP_LW) ||
                                   (inst[`FUNC3] == OP_LBU) ||
                                   (inst[`FUNC3] == OP_LHU))) ||
                                 ((inst[`OPCODE] == OP_STORE) &&
                                  ((inst[`FUNC3] == OP_SB) ||
                                   (inst[`FUNC3] == OP_SH) ||
                                   (inst[`FUNC3] == OP_SW))) ||
                                 (inst[`OPCODE] == OP_ARITHI)||
                                 ((inst[`OPCODE] == OP_ARITHR) &&
                                  (inst[`FUNC7] == 'h00 || inst[`FUNC7] == 'h20)) ||
                                 `ifdef RV32M_ENABLED
                                 ((inst[`OPCODE] == OP_ARITHR) && (inst[`FUNC7] == 'h01)) ||
                                 `endif // RV32M_ENABLED
                                 (inst[`OPCODE] == OP_FENCE )||
                                 (inst[`OPCODE] == OP_SYSTEM));
        `ifdef RV32M_ENABLED
        ex_mul              <= (inst[`OPCODE] == OP_ARITHR) && (inst[`FUNC7] == 'h1);
        `endif // RV32M_ENABLED
    end
end

`ifndef SYNTHESIS
always @* begin
    if (ex_illegal && !ex_flush)
        $display("Illegal instruction at PC 0x%08x", ex_pc[31: 0]);
    if (ex_ill_branch && !ex_flush)
        $display("Illegal branch instruction at PC 0x%08x", ex_pc[31: 0]);
end
`endif

always @(posedge clk or negedge resetb) begin
    if (!resetb)
        ex_mem2reg          <= 1'b0;
    else if (inst[`OPCODE] == OP_LOAD)
        ex_mem2reg          <= 1'b1;
    else if (ex_mem2reg && dmem_rvalid)
        ex_mem2reg          <= 1'b0;
end

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        ex_insn             <= NOP;
    end else if (!if_stall) begin
        ex_insn             <= inst;
    end
end

////////////////////////////////////////////////////////////
// stage 2: execute
////////////////////////////////////////////////////////////
    wire            [32: 0] result_subs;
    wire            [32: 0] result_subu;
    reg             [31: 0] ex_result;
    reg             [31: 0] next_pc;
    reg                     branch_taken;

// Trap Exception
assign ex_ld_align_excp     = ex_mem2reg && !ex_flush && (
                                (ex_alu_op == OP_LH && ex_memaddr[0]) ||
                                (ex_alu_op == OP_LW && |ex_memaddr[1:0]) ||
                                (ex_alu_op == OP_LHU && ex_memaddr[0])
                              );
assign ex_st_align_excp     = ex_memwr && !ex_flush && (
                                (ex_alu_op == OP_SH && ex_memaddr[0]) ||
                                (ex_alu_op == OP_SW && |ex_memaddr[1:0])
                              );
assign ex_inst_ill_excp     = !ex_flush && (ex_ill_branch || ex_ill_csr || ex_illegal);
assign ex_inst_align_excp   = !ex_flush && next_pc[1];
assign ex_timer_irq         = timer_irq && csr_mstatus[MIE] && csr_mie[MTIE] && !ex_system_op && !ex_flush;
assign ex_sw_irq            = sw_irq && csr_mstatus[MIE] && csr_mie[MSIE] && !ex_system_op && !ex_flush;
assign ex_interrupt         = interrupt && csr_mstatus[MIE] && csr_mie[MEIE] && !ex_system_op && !ex_flush;

assign ex_stall             = stall_r || if_stall ||
                              (ex_mem2reg && !dmem_rvalid);
assign alu_op1[31: 0]       = reg_rdata1;
assign alu_op2[31: 0]       = (ex_imm_sel) ? ex_imm : reg_rdata2;

assign result_subs[32: 0]   = {alu_op1[31], alu_op1} - {alu_op2[31], alu_op2};
assign result_subu[32: 0]   = {1'b0, alu_op1} - {1'b0, alu_op2};
assign ex_memaddr           = alu_op1 + ex_imm;
assign ex_flush             = wb_branch || wb_branch_nxt;
assign ex_systemcall        = ex_system && !ex_flush;

always @* begin
    branch_taken  = !ex_flush;
    next_pc       = fetch_pc + `IF_NEXT_PC;
    ex_ill_branch = 1'b0;

    case(1'b1)
        ex_jal   : next_pc = ex_pc + ex_imm;
        ex_jalr  : next_pc = alu_op1 + ex_imm;
        ex_branch: begin
            case(ex_alu_op)
                OP_BEQ : begin
                            next_pc = (result_subs[32: 0] == 'd0) ?
                                      ex_pc + ex_imm : fetch_pc + `IF_NEXT_PC;
                            if (result_subs[32: 0] != 'd0) branch_taken = 1'b0;
                         end
                OP_BNE : begin
                            next_pc = (result_subs[32: 0] != 'd0) ?
                                      ex_pc + ex_imm : fetch_pc + `IF_NEXT_PC;
                            if (result_subs[32: 0] == 'd0) branch_taken = 1'b0;
                         end
                OP_BLT : begin
                            next_pc = result_subs[32] ?
                                      ex_pc + ex_imm : fetch_pc + `IF_NEXT_PC;
                            if (!result_subs[32]) branch_taken = 1'b0;
                         end
                OP_BGE : begin
                            next_pc = !result_subs[32] ?
                                      ex_pc + ex_imm : fetch_pc + `IF_NEXT_PC;
                            if (result_subs[32]) branch_taken = 1'b0;
                         end
                OP_BLTU: begin
                            next_pc = result_subu[32] ?
                                      ex_pc + ex_imm : fetch_pc + `IF_NEXT_PC;
                            if (!result_subu[32]) branch_taken = 1'b0;
                         end
                OP_BGEU: begin
                            next_pc = !result_subu[32] ?
                                      ex_pc + ex_imm : fetch_pc + `IF_NEXT_PC;
                            if (result_subu[32]) branch_taken = 1'b0;
                         end
                default: begin
                         next_pc    = fetch_pc;
                         ex_ill_branch = 1'b1;
                         end
            endcase
        end
        default  : begin
                   next_pc          = fetch_pc + `IF_NEXT_PC;
                   branch_taken     = 1'b0;
                   end
    endcase
end

`ifdef RV32M_ENABLED
    wire            [63: 0] result_mul;
    wire            [63: 0] result_mulsu;
    wire            [63: 0] result_mulu;
    wire            [31: 0] result_div;
    wire            [31: 0] result_divu;
    wire            [31: 0] result_rem;
    wire            [31: 0] result_remu;

assign result_mul[63: 0]    = $signed  ({{32{alu_op1[31]}}, alu_op1[31: 0]}) *
                              $signed  ({{32{alu_op2[31]}}, alu_op2[31: 0]});
assign result_mulu[63: 0]   = $unsigned({{32{1'b0}},        alu_op1[31: 0]}) *
                              $unsigned({{32{1'b0}},        alu_op2[31: 0]});
assign result_mulsu[63: 0]  = $signed  ({{32{alu_op1[31]}}, alu_op1[31: 0]}) *
                              $unsigned({{32{1'b0}},        alu_op2[31: 0]});

// The result of divided by zero and (-MAX / -1) cannot be represented in twos complement.
// Assign the value to pass RISC-V compliance test.
assign result_div[31: 0]    = (alu_op2 == 32'h00000000) ? 32'hffffffff :
                              ((alu_op1 == 32'h80000000) && (alu_op2 == 32'hffffffff)) ? 32'h80000000 :
                              $signed  ($signed  (alu_op1) / $signed  (alu_op2));
assign result_divu[31: 0]   = (alu_op2 == 32'h00000000) ? 32'hffffffff :
                              $unsigned($unsigned(alu_op1) / $unsigned(alu_op2));
assign result_rem[31: 0]    = (alu_op2 == 32'h00000000) ? alu_op1 :
                              ((alu_op1 == 32'h80000000) && (alu_op2 == 32'hffffffff)) ? 32'h00000000 :
                              $signed  ($signed  (alu_op1) % $signed  (alu_op2));
assign result_remu[31: 0]   = (alu_op2 == 32'h00000000) ? alu_op1 :
                              $unsigned($unsigned(alu_op1) % $unsigned(alu_op2));
`endif // RV32M_ENABLED

always @* begin
    case(1'b1)
        ex_memwr:   ex_result           = alu_op2;
        ex_jal:     ex_result           = ex_pc + `EX_NEXT_PC;
        ex_jalr:    ex_result           = ex_pc + `EX_NEXT_PC;
        ex_lui:     ex_result           = ex_imm;
        ex_auipc:   ex_result           = ex_pc + ex_imm;
        ex_csr:     ex_result           = ex_csr_read;
        `ifdef RV32M_ENABLED
        ex_mul:
            case(ex_alu_op)
                OP_MUL   : ex_result    = result_mul  [31: 0];
                OP_MULH  : ex_result    = result_mul  [63:32];
                OP_MULSU : ex_result    = result_mulsu[63:32];
                OP_MULU  : ex_result    = result_mulu [63:32];
                OP_DIV   : ex_result    = result_div  [31: 0];
                OP_DIVU  : ex_result    = result_divu [31: 0];
                OP_REM   : ex_result    = result_rem  [31: 0];
                // OP_REMU
                default  : ex_result    = result_remu [31: 0];
            endcase
        `endif // RV32M_ENABLED
        ex_alu:
            case(ex_alu_op)
                OP_ADD : if (ex_subtype == 1'b0)
                            ex_result   = alu_op1 + alu_op2;
                         else
                            ex_result   = alu_op1 - alu_op2;
                                          // In RISC-V ISA spec, only shift amount
                                          // held in lower 5 bits of register
                OP_SLL : ex_result      = alu_op1 << alu_op2[4:0];
                OP_SLT : ex_result      = result_subs[32] ? 'd1 : 'd0;
                OP_SLTU: ex_result      = result_subu[32] ? 'd1 : 'd0;
                OP_XOR : ex_result      = alu_op1 ^ alu_op2;
                OP_SR  : if (ex_subtype == 1'b0)
                            ex_result   = alu_op1 >>> alu_op2[4:0]; // shift more than 32 is undefined
                         else
                            ex_result   = $signed(alu_op1) >>> alu_op2[4:0]; // shift more than 32 is undefined
                OP_OR  : ex_result      = alu_op1 | alu_op2;
                // OP_AND
                default: ex_result      = alu_op1 & alu_op2;
            endcase
        default: begin
            ex_result                   = 32'h0;
        end
    endcase
end

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        fetch_pc            <= RESETVEC;
    end else if (!ex_stall) begin
        fetch_pc            <= (ex_flush) ? (fetch_pc + `EX_NEXT_PC) :
                               (ex_trap)  ? (ex_trap_pc)   :
                               {next_pc[31:1], 1'b0};
    end
end

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        wb_result           <= 32'h0;
        wb_alu2reg          <= 1'b0;
        wb_dst_sel          <= 5'h0;
        wb_branch           <= 1'b0;
        wb_branch_nxt       <= 1'b0;
        wb_mem2reg          <= 1'b0;
        wb_raddr            <= 2'h0;
        wb_alu_op           <= 3'h0;
    end else if (!ex_stall) begin
        wb_result           <= ex_result;
        wb_alu2reg          <= ex_alu || ex_lui || ex_auipc || ex_jal || ex_jalr ||
                               ex_csr ||
                               `ifdef RV32M_ENABLED
                               ex_mul ||
                               `endif
                               (ex_mem2reg && !ex_ld_align_excp);
        wb_dst_sel          <= ex_dst_sel;
        wb_branch           <= branch_taken || ex_trap;
        wb_branch_nxt       <= wb_branch;
        wb_mem2reg          <= ex_mem2reg;
        wb_raddr            <= dmem_raddr[1:0];
        wb_alu_op           <= ex_alu_op;
    end
end

always @(posedge clk or negedge resetb) begin
    if (!resetb)
        wb_memwr            <= 1'b0;
    else if (ex_memwr && !ex_flush && !ex_st_align_excp)
        wb_memwr            <= 1'b1;
    else if (wb_memwr && dmem_wvalid)
        wb_memwr            <= 1'b0;
end

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        wb_waddr            <= 32'h0;
        wb_wstrb            <= 4'h0;
        wb_wdata            <= 32'h0;
    end else if (!ex_stall && ex_memwr) begin
        wb_waddr            <= ex_memaddr;
        case(ex_alu_op)
            OP_SB: begin
                wb_wdata    <= {4{alu_op2[7:0]}};
                case(ex_memaddr[1:0])
                    2'b00:  wb_wstrb <= 4'b0001;
                    2'b01:  wb_wstrb <= 4'b0010;
                    2'b10:  wb_wstrb <= 4'b0100;
                    default:wb_wstrb <= 4'b1000;
                endcase
            end
            OP_SH: begin
                wb_wdata    <= {2{alu_op2[15:0]}};
                wb_wstrb    <= ex_memaddr[1] ? 4'b1100 : 4'b0011;
            end
            OP_SW: begin
                wb_wdata    <= alu_op2;
                wb_wstrb    <= 4'hf;
            end
            default: begin
                wb_wdata    <= 32'h0;
                wb_wstrb    <= 4'hf;
            end
        endcase
    end
end

////////////////////////////////////////////////////////////
// stage 3: write back
////////////////////////////////////////////////////////////
assign imem_addr            = fetch_pc;
assign imem_ready           = !stall_r && !wb_stall;
assign wb_stall             = stall_r ||
                              (wb_memwr && !dmem_wvalid) ||
                              (wb_mem2reg && !dmem_rresp);
assign wb_flush             = wb_nop || wb_nop_more;

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        wb_nop              <= 1'b0;
        wb_nop_more         <= 1'b0;
    end else if (!ex_stall && !(wb_memwr && !dmem_wvalid)) begin
        wb_nop              <= wb_branch;
        wb_nop_more         <= wb_nop;
    end
end

always @* begin
    case(wb_alu_op)
        OP_LB  : begin
                    case(wb_raddr[1:0])
                        2'b00: wb_rdata[31: 0] = {{24{dmem_rdata[7]}},
                                                  dmem_rdata[ 7: 0]};
                        2'b01: wb_rdata[31: 0] = {{24{dmem_rdata[15]}},
                                                  dmem_rdata[15: 8]};
                        2'b10: wb_rdata[31: 0] = {{24{dmem_rdata[23]}},
                                                  dmem_rdata[23:16]};
                        2'b11: wb_rdata[31: 0] = {{24{dmem_rdata[31]}},
                                                  dmem_rdata[31:24]};
                    endcase
                 end
        OP_LH  : begin
                     wb_rdata = (wb_raddr[1]) ?
                               {{16{dmem_rdata[31]}}, dmem_rdata[31:16]} :
                               {{16{dmem_rdata[15]}}, dmem_rdata[15: 0]};
                 end
        OP_LW  : begin
                    wb_rdata = dmem_rdata;
                 end
        OP_LBU : begin
                    case(wb_raddr[1:0])
                        2'b00: wb_rdata[31: 0] = {24'h0, dmem_rdata[7:0]};
                        2'b01: wb_rdata[31: 0] = {24'h0, dmem_rdata[15:8]};
                        2'b10: wb_rdata[31: 0] = {24'h0, dmem_rdata[23:16]};
                        2'b11: wb_rdata[31: 0] = {24'h0, dmem_rdata[31:24]};
                    endcase
                 end
        OP_LHU : begin
                    wb_rdata = (wb_raddr[1]) ?
                               {16'h0, dmem_rdata[31:16]} :
                               {16'h0, dmem_rdata[15: 0]};
                 end
        default: begin
                    wb_rdata = 32'h0;
                 end
    endcase
end

////////////////////////////////////////////////////////////
// Trap CSR
////////////////////////////////////////////////////////////
// Trap CSR @ execution stage
assign ex_trap      = (ex_inst_ill_excp || ex_inst_align_excp ||
                       ex_ld_align_excp || ex_st_align_excp ||
                       ex_timer_irq || ex_sw_irq || ex_interrupt ||
                       ex_systemcall) && !ex_flush;
assign ex_trap_pc   = (ex_systemcall && ex_imm[1:0] == 2'b10) ? // mret
                      csr_mepc :
                      csr_mtvec[0] ?
                      {csr_mtvec[31:2], 2'b00} + {26'h0, ex_mcause[3:0], 2'b00} :
                      {csr_mtvec[31:2], 2'b00};

assign ex_csr_data  = ex_alu_op[2] ? {27'h0, ex_src1_sel[4:0]} : reg_rdata1;

always @* begin
    ex_mcause     = 32'h0;
    case(1'b1)
        ex_inst_ill_excp   : ex_mcause = TRAP_INST_ILL;
        ex_inst_align_excp : ex_mcause = TRAP_INST_ALIGN;
        ex_ld_align_excp   : ex_mcause = TRAP_LD_ALIGN;
        ex_st_align_excp   : ex_mcause = TRAP_ST_ALIGN;
        ex_timer_irq       : ex_mcause = INT_MTIME;
        ex_sw_irq          : ex_mcause = INT_MSI;
        ex_interrupt       : ex_mcause = INT_MEI;
        ex_systemcall      : begin
            case (ex_imm[1:0])
                2'b00: ex_mcause   = TRAP_ECALL;
                2'b01: ex_mcause   = TRAP_BREAK;
                2'b10: ex_mcause   = csr_mcause; // uret, sret, mret
                default: begin
                    `ifndef SYNTHESIS
                    $display("Illegal system call at PC 0x%08x\n", ex_pc);
                    `endif
                    ex_mcause = TRAP_INST_ILL;
                end
            endcase
        end
    endcase
end

assign ex_ret_pc = (ex_jal || ex_jalr || (ex_branch && branch_taken)) ? next_pc[31: 1] : ex_pc[31: 1] + 31'd2;

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        csr_mcause                  <= 32'h0;
        csr_mepc                    <= 32'h0;
        csr_mtval                   <= 32'h0;
        csr_mstatus                 <= 32'h0;
        csr_mip                     <= 32'h0;
    end else if (!ex_stall && !ex_flush) begin
        case(1'b1)
            ex_inst_ill_excp : begin
                csr_mcause          <= TRAP_INST_ILL;
                csr_mepc            <= {ex_pc[31: 1], 1'b0};
                csr_mtval           <= ex_insn;
                csr_mstatus[MPIE]   <= csr_mstatus[MIE];
                csr_mstatus[MIE]    <= 1'b0;
                csr_mip             <= csr_mip;
            end
            ex_csr_wr : begin
                case (ex_imm[11: 0])
                    CSR_MEPC   : begin
                        csr_mepc    <= !ex_alu_op[1] ? ex_csr_data : // CSRRW
                                       !ex_alu_op[0] ? (csr_mepc | ex_csr_data) : // CSRRS
                                       (csr_mepc & ~ex_csr_data); // CSRRC
                    end
                    CSR_MCAUSE : begin
                        csr_mcause  <= !ex_alu_op[1] ? ex_csr_data : // CSRRW
                                       !ex_alu_op[0] ? (csr_mcause | ex_csr_data) : // CSRRS
                                       (csr_mcause & ~ex_csr_data); // CSRRC
                    end
                    CSR_MTVAL  : begin
                        csr_mtval   <= !ex_alu_op[1] ? ex_csr_data : // CSRRW
                                       !ex_alu_op[0] ? (csr_mtval | ex_csr_data) : // CSRRS
                                       (csr_mtval & ~ex_csr_data); // CSRRC
                    end
                    CSR_MSTATUS: begin
                        csr_mstatus <= !ex_alu_op[1] ? ex_csr_data : // CSRRW
                                       !ex_alu_op[0] ? (csr_mstatus | ex_csr_data) : // CSRRS
                                       (csr_mstatus & ~ex_csr_data); // CSRRC
                    end
                    CSR_MIP    : begin
                        csr_mip     <= !ex_alu_op[1] ? ex_csr_data : // CSRRW
                                       !ex_alu_op[0] ? (csr_mip | ex_csr_data) : // CSRRS
                                       (csr_mip & ~ex_csr_data); // CSRRC
                    end
                    default    : ;
                endcase
            end
            ex_inst_align_excp : begin
                csr_mcause          <= TRAP_INST_ALIGN;
                csr_mepc            <= {ex_pc[31: 1], 1'b0};
                csr_mtval           <= {next_pc[31: 1], 1'b0};
                csr_mstatus[MPIE]   <= csr_mstatus[MIE];
                csr_mstatus[MIE]    <= 1'b0;
                csr_mip             <= csr_mip;
            end
            ex_ld_align_excp : begin
                csr_mcause          <= TRAP_LD_ALIGN;
                csr_mepc            <= {ex_pc[31: 1], 1'b0};
                csr_mtval           <= ex_memaddr;
                csr_mstatus[MPIE]   <= csr_mstatus[MIE];
                csr_mstatus[MIE]    <= 1'b0;
                csr_mip             <= csr_mip;
            end
            ex_st_align_excp : begin
                csr_mcause          <= TRAP_ST_ALIGN;
                csr_mepc            <= {ex_pc[31: 1], 1'b0};
                csr_mtval           <= ex_memaddr;
                csr_mstatus[MPIE]   <= csr_mstatus[MIE];
                csr_mstatus[MIE]    <= 1'b0;
                csr_mip             <= csr_mip;
            end
            ex_timer_irq : begin
                csr_mcause          <= INT_MTIME;
                csr_mepc            <= {ex_ret_pc[31: 1], 1'b0};
                csr_mtval           <= 32'd0; // FIXME
                csr_mstatus[MPIE]   <= csr_mstatus[MIE];
                csr_mstatus[MIE]    <= 1'b0;
                csr_mip[MTIP]       <= 1'b1;
            end
            ex_sw_irq : begin
                csr_mcause          <= INT_MSI;
                csr_mepc            <= {ex_ret_pc[31: 1], 1'b0};
                csr_mtval           <= 32'd0; // FIXME
                csr_mstatus[MPIE]   <= csr_mstatus[MIE];
                csr_mstatus[MIE]    <= 1'b0;
                csr_mip[MSIP]       <= 1'b1;
            end
            ex_interrupt : begin
                csr_mcause          <= INT_MEI;
                csr_mepc            <= {ex_ret_pc[31: 1], 1'b0};
                csr_mtval           <= 32'd0; // FIXME
                csr_mstatus[MPIE]   <= csr_mstatus[MIE];
                csr_mstatus[MIE]    <= 1'b0;
                csr_mip[MEIP]       <= 1'b1;
            end
            ex_systemcall : begin
                case (ex_imm[1:0])
                    2'b00: begin // ECALL
                        csr_mcause          <= TRAP_ECALL;
                        csr_mepc            <= {ex_pc[31: 1], 1'b0};
                        csr_mtval           <= 32'd0; // FIXME
                        csr_mstatus[MPIE]   <= csr_mstatus[MIE];
                        csr_mstatus[MIE]    <= 1'b0;
                        csr_mip             <= csr_mip;
                    end
                    2'b01: begin // EBREAK
                        csr_mcause          <= TRAP_BREAK;
                        csr_mepc            <= {ex_pc[31: 1], 1'b0};
                        csr_mtval           <= 32'd0; // FIXME
                        csr_mstatus[MPIE]   <= csr_mstatus[MIE];
                        csr_mstatus[MIE]    <= 1'b0;
                        csr_mip             <= csr_mip;
                    end
                    2'b10: begin // URET, SRET, MRET
                        csr_mcause          <= csr_mcause;  // FIXME
                        csr_mepc            <= {ex_pc[31: 1], 1'b0}; // FIXME
                        csr_mtval           <= 32'd0; // FIXME
                        csr_mstatus[MIE]    <= csr_mstatus[MPIE];
                        csr_mip             <= csr_mip;
                    end
                    default: begin
                        csr_mcause          <= TRAP_INST_ILL;
                        csr_mepc            <= {ex_pc[31: 1], 1'b0};
                        csr_mtval           <= ex_insn;
                        csr_mstatus         <= csr_mstatus;
                        csr_mip             <= csr_mip;
                    end
                endcase
            end
        endcase
    end
end

////////////////////////////////////////////////////////////
// CSR file
////////////////////////////////////////////////////////////
// CSR read @ execution stage
always @* begin
    ex_ill_csr  = 1'b0;
    ex_csr_read = 32'h0;
    if (ex_csr && !ex_flush) begin
        case (ex_imm[11:0])
            CSR_MVENDORID  : ex_csr_read = MVENDORID;
            CSR_MARCHID    : ex_csr_read = MARCHID;
            CSR_MIMPID     : ex_csr_read = MIMPID;
            CSR_MHARTID    : ex_csr_read = MHARTID;
            CSR_MSCRATCH   : ex_csr_read = csr_mscratch;
            CSR_MSTATUS    : ex_csr_read = csr_mstatus;
            CSR_MISA       : ex_csr_read = csr_misa;
            CSR_MIE        : ex_csr_read = csr_mie;
            CSR_MIP        : ex_csr_read = csr_mip;
            CSR_MTVEC      : ex_csr_read = csr_mtvec;
            CSR_MEPC       : ex_csr_read = csr_mepc;
            CSR_MCAUSE     : ex_csr_read = csr_mcause[31:0];
            CSR_MTVAL      : ex_csr_read = csr_mtval;
            CSR_RDCYCLE    : ex_csr_read = csr_cycle[31:0];
            CSR_RDCYCLEH   : ex_csr_read = csr_cycle[63:32];
            CSR_RDINSTRET  : ex_csr_read = csr_instret[31:0];
            CSR_RDINSTRETH : ex_csr_read = csr_instret[63:32];
            default: begin
                ex_ill_csr = 1'b1;
                `ifndef SYNTHESIS
                $display("Unsupport CSR register 0x%0x at PC 0x%08x", ex_imm[11:0], ex_pc);
                `endif
            end
        endcase
    end
end

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        csr_misa            <= 32'h0;
        csr_mie             <= 32'h0;
        csr_mtvec           <= 32'h0;
    end else if (!ex_stall && ex_csr_wr && !ex_flush) begin
        case (ex_imm[11:0])
            CSR_MVENDORID  : ; // Read only
            CSR_MARCHID    : ; // Read only
            CSR_MIMPID     : ; // Read only
            CSR_MHARTID    : ; // Read only
            CSR_MSTATUS    : ; // update @ trap/interrupt handler
            CSR_MSCRATCH   : begin
                csr_mscratch<= !ex_alu_op[1] ? ex_csr_data : // CSRRW
                               !ex_alu_op[0] ? (csr_mscratch | ex_csr_data) : // CSRRS
                               (csr_mscratch & ~ex_csr_data); // CSRRC
            end
            CSR_MISA       : begin
                csr_misa    <= !ex_alu_op[1] ? ex_csr_data : // CSRRW
                               !ex_alu_op[0] ? (csr_misa | ex_csr_data) : // CSRRS
                               (csr_misa & ~ex_csr_data); // CSRRC
            end
            CSR_MIE        : begin
                csr_mie     <= !ex_alu_op[1] ? ex_csr_data : // CSRRW
                               !ex_alu_op[0] ? (csr_mie | ex_csr_data) : // CSRRS
                               (csr_mie & ~ex_csr_data); // CSRRC
            end
            CSR_MTVEC      : begin
                csr_mtvec   <= !ex_alu_op[1] ? ex_csr_data : // CSRRW
                               !ex_alu_op[0] ? (csr_mtvec | ex_csr_data) : // CSRRS
                               (csr_mtvec & ~ex_csr_data); // CSRRC
            end
            CSR_MEPC       : ; // update @ trap/interrupt handler
            CSR_MCAUSE     : ; // update @ trap/interrupt handler
            CSR_MTVAL      : ; // update @ trap/interrupt handler
            CSR_RDCYCLE    : ; // Read Only
            CSR_RDCYCLEH   : ; // Read Only
            CSR_RDINSTRET  : ; // Read Only
            CSR_RDINSTRETH : ; // Read Only
            default: ;
        endcase
    end
end

assign timer_en             = (pipefill == 2'b10);

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        csr_cycle           <= 64'h0;
        csr_instret         <= 64'h0;
        pipefill            <= 2'b00;
    end else if (!stall_r) begin
        if (pipefill != 2'b10)
            pipefill        <= pipefill + 1'b1;
        else begin
            csr_cycle       <= csr_cycle + 1'b1;
            if (!ex_stall && !ex_flush) begin
                csr_instret <= csr_instret + 1'b1;
            end
        end
    end
end

////////////////////////////////////////////////////////////
// Register file
////////////////////////////////////////////////////////////
// Read address  : ex_src1_sel, ex_src2_sel.
// Read data out : reg_rdata1, reg_rdata2
// Write enable  : wb_alu2reg
// Write address : wb_dst_sel
// Write data    : wb_result
////////////////////////////////////////////////////////////

// register reading @ execution stage and register forwarding
// When the execution result accesses the same register,
// the execution result is directly forwarded from the previous
// instruction (at write back stage)
assign reg_rdata1[31: 0]    = (ex_src1_sel == 5'h0) ? 32'h0 :
                              (!wb_flush && wb_alu2reg &&
                               (wb_dst_sel == ex_src1_sel)) ? // register forwarding
                                (wb_mem2reg ? wb_rdata : wb_result) :
                                regs[ex_src1_sel];
assign reg_rdata2[31: 0]    = (ex_src2_sel == 5'h0) ? 32'h0 :
                              (!wb_flush && wb_alu2reg &&
                               (wb_dst_sel == ex_src2_sel)) ? // register forwarding
                                (wb_mem2reg ? wb_rdata : wb_result) :
                                regs[ex_src2_sel];

// register writing @ write back stage
always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        for(i = 1; i < 32; i = i + 1) regs[i] <= 32'h0;
    end else if (wb_alu2reg && !stall_r && !(wb_stall || wb_flush)) begin
        regs[wb_dst_sel]    <= wb_mem2reg ? wb_rdata : wb_result;
    end
end

////////////////////////////////////////////////////////////
// for debugging, register name alias
////////////////////////////////////////////////////////////
`ifndef SYNTHESIS
/* verilator coverage_off */
/* Verilator lint_off UNUSED */
    wire        [31: 0] x0_zero     = 'd0;
    wire        [31: 0] x1_ra       = regs[ 1][31: 0];
    wire        [31: 0] x2_sp       = regs[ 2][31: 0];
    wire        [31: 0] x3_gp       = regs[ 3][31: 0];
    wire        [31: 0] x4_tp       = regs[ 4][31: 0];
    wire        [31: 0] x5_t0       = regs[ 5][31: 0];
    wire        [31: 0] x6_t1       = regs[ 6][31: 0];
    wire        [31: 0] x7_t2       = regs[ 7][31: 0];
    wire        [31: 0] x8_s0_fp    = regs[ 8][31: 0];
    wire        [31: 0] x9_s1       = regs[ 9][31: 0];
    wire        [31: 0] x10_a0      = regs[10][31: 0];
    wire        [31: 0] x11_a1      = regs[11][31: 0];
    wire        [31: 0] x12_a2      = regs[12][31: 0];
    wire        [31: 0] x13_a3      = regs[13][31: 0];
    wire        [31: 0] x14_a4      = regs[14][31: 0];
    wire        [31: 0] x15_a5      = regs[15][31: 0];
    wire        [31: 0] x16_a6      = regs[16][31: 0];
    wire        [31: 0] x17_a7      = regs[17][31: 0];
    wire        [31: 0] x18_s2      = regs[18][31: 0];
    wire        [31: 0] x19_s3      = regs[19][31: 0];
    wire        [31: 0] x20_s4      = regs[20][31: 0];
    wire        [31: 0] x21_s5      = regs[21][31: 0];
    wire        [31: 0] x22_s6      = regs[22][31: 0];
    wire        [31: 0] x23_s7      = regs[23][31: 0];
    wire        [31: 0] x24_s8      = regs[24][31: 0];
    wire        [31: 0] x25_s9      = regs[25][31: 0];
    wire        [31: 0] x26_s10     = regs[26][31: 0];
    wire        [31: 0] x27_s11     = regs[27][31: 0];
    wire        [31: 0] x28_t3      = regs[28][31: 0];
    wire        [31: 0] x29_t4      = regs[29][31: 0];
    wire        [31: 0] x30_t5      = regs[30][31: 0];
    wire        [31: 0] x31_t6      = regs[31][31: 0];
    reg         [31: 0] wb_insn;
    reg         [ 1: 0] wb_break;
    reg         [31: 0] wb_raddress;
    reg                 wb_system;
    reg                 wb_ld_align_excp;

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        wb_pc               <= RESETVEC;
        wb_insn             <= 32'h0;
        wb_break            <= 2'b00;
        wb_system           <= 1'b0;
        wb_ld_align_excp    <= 1'b0;
    end else if (!ex_stall) begin
        wb_pc               <= ex_pc;
        wb_insn             <= ex_insn;
        wb_break            <= ex_imm[1:0];
        wb_system           <= ex_systemcall;
        wb_ld_align_excp    <= ex_ld_align_excp;
    end
end

always @(posedge clk) begin
    if (!ex_stall) begin
        wb_raddress         <= dmem_raddr[31:0];
    end
end

function [31:0] set_reg;
    input [ 4:0] regn;
    input [31:0] data;
begin
    /* verilator lint_off BLKSEQ */
    regs[regn] = data;
    /* verilator lint_on BLKSEQ */
    set_reg = data;
end
endfunction

/* verilator coverage_on */
/* Verilator lint_on UNUSED */
`endif // SYNTHESIS

endmodule

