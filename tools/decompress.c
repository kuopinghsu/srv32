// Copyright © 2020 Kuoping Hsu
// decompress.c: RV32C instruction decoder
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

#if defined(RV32C_ENABLED)
#include <string.h>
#include <stdint.h>
#include "opcode.h"

static inline
unsigned int GET_IMM (
    unsigned int v,
    unsigned int from,
    unsigned int to,
    unsigned int mask
) {
    unsigned int result;

    if (from > to)
        result = v >> (from - to);
    else
        result = v << (to - from);

    return (result & (mask << to));
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
                if (instc.ciw.imm != 0) {
                    r.i.op    = OP_ARITHI;
                    r.i.rd    = 8 + instc.ciw.rd_;
                    r.i.func3 = OP_ADD;
                    r.i.rs1   = 2;
                    r.i.imm   = GET_IMM(instc.ciw.imm, 0, 3, 0x1) + // imm[3]
                                GET_IMM(instc.ciw.imm, 1, 2, 0x1) + // imm[2]
                                GET_IMM(instc.ciw.imm, 2, 6, 0xf) + // imm[9:6]
                                GET_IMM(instc.ciw.imm, 6, 4, 0x3) ; // imm[5:4]
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
                r.i.imm   = GET_IMM(instc.cl.imm_h, 0, 3, 0x7) + // imm_h[5:3]
                            GET_IMM(instc.cl.imm_l, 0, 6, 0x1) + // imm_l[6]
                            GET_IMM(instc.cl.imm_l, 1, 2, 0x1) ; // imm_l[2]
                break;
            case OP_CSW       : // c.sw -> sw rs2′, offset[6:2](rs1′)
                r.s.op    = OP_STORE;
                r.s.imm1  = GET_IMM(instc.cs.imm_h, 0, 3, 0x3) +
                            GET_IMM(instc.cs.imm_l, 1, 2, 0x1) ;
                r.s.func3 = OP_SW;
                r.s.rs1   = 8 + instc.cs.rs1_;
                r.s.rs2   = 8 + instc.cs.rs2_;
                r.s.imm2  = GET_IMM(instc.cs.imm_l, 0, 1, 0x1) +
                            GET_IMM(instc.cs.imm_h, 2, 0, 0x1) ;
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
                    r.i.imm   = (instc.ci.imm_h ? 0xfe0 : 0) + instc.ci.imm_l;
                } else {
                    *illegal = 1;
                }
                break;
            case OP_CBEQZ     : // c.beqz -> beq rs1′, x0, offset[8:1]
                r.b.op    = OP_BRANCH;
                r.b.imm1  = GET_IMM(instc.cb.imm_h, 0, 3, 0x3) +
                            GET_IMM(instc.cb.imm_l, 1, 1, 0x3) +
                            ((instc.cb.imm_h & 4) ? 1 : 0);
                r.b.func3 = OP_BEQ;
                r.b.rs1   = instc.cb.rs1_ + 8;
                r.b.rs2   = 0;
                r.b.imm2  = ((instc.cb.imm_h & 4) ? 0x78 : 0) +
                            GET_IMM(instc.cb.imm_l, 3, 1, 0x3) +
                            GET_IMM(instc.cb.imm_l, 0, 0, 0x1) ;
                break;
            case OP_CBNEZ     : // c.bnez -> bne rs1′, x0, offset[8:1]
                r.b.op    = OP_BRANCH;
                r.b.imm1  = GET_IMM(instc.cb.imm_h, 0, 3, 0x3) +
                            GET_IMM(instc.cb.imm_l, 1, 1, 0x3) +
                            ((instc.cb.imm_h & 4) ? 1 : 0);
                r.b.func3 = OP_BNE;
                r.b.rs1   = instc.cb.rs1_ + 8;
                r.b.rs2   = 0;
                r.b.imm2  = ((instc.cb.imm_h & 4) ? 0x78 : 0) +
                            GET_IMM(instc.cb.imm_l, 3, 1, 0x3) +
                            GET_IMM(instc.cb.imm_l, 0, 0, 0x1) ;
                break;
            case OP_CJ        : // c.j -> jal x0, offset[11:1]
                r.j.op  = OP_JAL;
                r.j.rd  = 0;
                r.j.imm = ((instc.cj.offset & 0x400) ? 0x801ff : 0) +
                          GET_IMM(instc.cj.offset, 0, 13, 0x1) + // imm[5]
                          GET_IMM(instc.cj.offset, 1,  9, 0x7) + // imm[3:1]
                          GET_IMM(instc.cj.offset, 4, 15, 0x1) + // imm[7]
                          GET_IMM(instc.cj.offset, 5, 14, 0x1) + // imm[6]
                          GET_IMM(instc.cj.offset, 6, 18, 0x1) + // imm[10]
                          GET_IMM(instc.cj.offset, 7, 16, 0x3) + // imm[9:8]
                          GET_IMM(instc.cj.offset, 9, 12, 0x1) ; // imm[4]
                break;
            case OP_CJAL      : // c.jal -> jal x1, offset[11:1]
                r.j.op  = OP_JAL;
                r.j.rd  = 1;
                r.j.imm = ((instc.cj.offset & 0x400) ? 0x801ff : 0) +
                          GET_IMM(instc.cj.offset, 0, 13, 0x1) + // imm[5]
                          GET_IMM(instc.cj.offset, 1,  9, 0x7) + // imm[3:1]
                          GET_IMM(instc.cj.offset, 4, 15, 0x1) + // imm[7]
                          GET_IMM(instc.cj.offset, 5, 14, 0x1) + // imm[6]
                          GET_IMM(instc.cj.offset, 6, 18, 0x1) + // imm[10]
                          GET_IMM(instc.cj.offset, 7, 16, 0x3) + // imm[9:8]
                          GET_IMM(instc.cj.offset, 9, 12, 0x1) ; // imm[4]
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
                        r.i.imm   = (instc.ci.imm_h ? 0xe00 : 0) +
                                    GET_IMM(instc.ci.imm_l, 0, 5, 0x1) + // imm[5]
                                    GET_IMM(instc.ci.imm_l, 1, 7, 0x3) + // imm[8:7]
                                    GET_IMM(instc.ci.imm_l, 3, 6, 0x1) + // imm[6]
                                    GET_IMM(instc.ci.imm_l, 4, 4, 0x1) ; // imm[4]
                    } else { // reserved
                        *illegal = 1;
                        break;
                    }
                } else if (instc.ci.rd != 0) { // c.lui -> lui rd, nzimm[17:12]
                    if (instc.ci.imm_h != 0 || instc.ci.imm_l != 0) {
                        r.u.op  = OP_LUI;
                        r.u.rd  = instc.ci.rd;
                        r.u.imm = (instc.ci.imm_h ? 0xfffe0 : 0) +
                                  instc.ci.imm_l;
                    } else { // reserved
                        *illegal = 1;
                        break;
                    }
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
                        r.r.op    = OP_ARITHI;
                        r.r.rd    = instc.cb.rs1_ + 8;
                        r.r.func3 = OP_SR;
                        r.r.rs1   = instc.cb.rs1_ + 8;
                        r.r.rs2   = instc.cb.imm_l;
                        r.r.func7 = 0;
                        break;
                    case 1: // c.srai -> srai rd′, rd′, shamt[5:0]
                        r.r.op    = OP_ARITHI;
                        r.r.rd    = instc.cb.rs1_ + 8;
                        r.r.func3 = OP_SR;
                        r.r.rs1   = instc.cb.rs1_ + 8;
                        r.r.rs2   = instc.cb.imm_l;
                        r.r.func7 = 0x20;
                        break;
                    case 2: // c.andi -> andi rd′, rd′, imm[5:0]
                        r.i.op    = OP_ARITHI;
                        r.i.rd    = instc.cb.rs1_ + 8;
                        r.i.func3 = OP_AND;
                        r.i.rs1   = instc.cb.rs1_ + 8;
                        r.i.imm   = ((instc.cb.imm_h & 4) ? 0xfe0 : 0) +
                                    instc.cb.imm_l;
                        break;
                    case 3:
                        if (instc.ca.func6 == 0x27) { // Reserved
                            *illegal = 1;
                            break;
                        }
                        switch(instc.ca.func2) {
                            case 0: // c.sub -> sub rd′, rd′, rs2′
                                r.r.op    = OP_ARITHR;
                                r.r.rd    = instc.ca.rd_ + 8;
                                r.r.func3 = OP_ADD;
                                r.r.rs1   = instc.ca.rd_ + 8;
                                r.r.rs2   = instc.ca.rs2_ + 8;
                                r.r.func7 = 0x20;
                                break;
                            case 1: // c.xor -> xor rd′, rd′, rs2′
                                r.r.op    = OP_ARITHR;
                                r.r.rd    = instc.ca.rd_ + 8;
                                r.r.func3 = OP_XOR;
                                r.r.rs1   = instc.ca.rd_ + 8;
                                r.r.rs2   = instc.ca.rs2_ + 8;
                                r.r.func7 = 0x00;
                                break;
                            case 2: // c.or -> or rd′, rd′, rs2'
                                r.r.op    = OP_ARITHR;
                                r.r.rd    = instc.ca.rd_ + 8;
                                r.r.func3 = OP_OR;
                                r.r.rs1   = instc.ca.rd_ + 8;
                                r.r.rs2   = instc.ca.rs2_ + 8;
                                r.r.func7 = 0x00;
                                break;
                            case 3: // c.and -> and rd′, rd′, rs2′
                                r.r.op    = OP_ARITHR;
                                r.r.rd    = instc.ca.rd_ + 8;
                                r.r.func3 = OP_AND;
                                r.r.rs1   = instc.ca.rd_ + 8;
                                r.r.rs2   = instc.ca.rs2_ + 8;
                                r.r.func7 = 0x00;
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
                    r.i.op    = OP_JALR;
                    r.i.rd    = 0;
                    r.i.func3 = 0x0;
                    r.i.rs1   = instc.ci.rd;
                    r.i.imm   = 0;
                    break;
                }
                if (instc.ci.imm_h == 0 && instc.ci.rd != 0 && instc.ci.imm_l != 0) { // c.mv -> add rd, x0, rs2
                    r.r.op    = OP_ARITHR;
                    r.r.rd    = instc.cr.rd;
                    r.r.func3 = OP_ADD;
                    r.r.rs1   = 0;
                    r.r.rs2   = instc.cr.rs2;
                    r.r.func7 = 0;
                    break;
                }
                if (instc.ci.imm_h == 1 && instc.ci.rd == 0 && instc.ci.imm_l == 0) { // c.ebreak -> ebreak
                    r.inst    = 0x00100073;
                    break;
                }
                if (instc.ci.imm_h == 1 && instc.ci.rd != 0 && instc.ci.imm_l == 0) { // c.jalr -> jalr x1, 0(rs1)
                    r.i.op    = OP_JALR;
                    r.i.rd    = 1;
                    r.i.func3 = 0x0;
                    r.i.rs1   = instc.ci.rd;
                    r.i.imm   = 0;
                    break;
                }
                if (instc.ci.imm_h == 1 && instc.ci.rd != 0 && instc.ci.imm_l != 0) { // c.add -> add rd, rd, rs2
                    r.r.op    = OP_ARITHR;
                    r.r.rd    = instc.cr.rd;
                    r.r.func3 = OP_ADD;
                    r.r.rs1   = instc.cr.rd;
                    r.r.rs2   = instc.cr.rs2;
                    r.r.func7 = 0;
                    break;
                }
                *illegal = 1;
                break;
            case OP_CLWSP   : // c.lwsp -> lw rd, offset[7:2](x2)
                if (instc.ci.rd != 0) {
                    r.i.op    = OP_LOAD;
                    r.i.rd    = instc.ci.rd;
                    r.i.func3 = OP_LW;
                    r.i.rs1   = 2;
                    r.i.imm   = GET_IMM(instc.ci.imm_h, 0, 5, 0x1) +
                                GET_IMM(instc.ci.imm_l, 0, 6, 0x3) +
                                GET_IMM(instc.ci.imm_l, 2, 2, 0x7) ;
                } else {
                    *illegal = 1;
                }
                break;
            case OP_CSLLI   : // c.slli -> slli rd, rd, shamt[5:0]
                if (instc.ci.imm_h == 0) {
                    r.r.op    = OP_ARITHI;
                    r.r.rd    = instc.ci.rd;
                    r.r.func3 = OP_SLL;
                    r.r.rs1   = instc.ci.rd;
                    r.r.rs2   = instc.ci.imm_l;
                    r.r.func7 = 0;
                } else {
                    *illegal = 1;
                }
                break;
            case OP_CSWSP   : // c.swsp -> sw rs2, offset[7:2](x2)
                r.s.op    = OP_STORE;
                r.s.imm1  = GET_IMM(instc.css.imm, 2, 2, 0x7);
                r.s.func3 = OP_SW;
                r.s.rs1   = 2;
                r.s.rs2   = instc.css.rs2;
                r.s.imm2  = GET_IMM(instc.css.imm, 1, 2, 0x1) +
                            GET_IMM(instc.css.imm, 0, 1, 0x1) +
                            GET_IMM(instc.css.imm, 5, 0, 0x1) ;
                break;
            default: *illegal = 1;
        }
        inst->inst = r.inst;
        return 1;
    }

    return 0;
}
#endif // RV32C_ENABLED

