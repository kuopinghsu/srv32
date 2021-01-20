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

#include <string.h>
#include <stdint.h>
#include "opcode.h"

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
                if (r.i.imm != 0) {
                    r.i.op    = OP_ARITHI;
                    r.i.rd    = 8 + instc.ciw.rd_;
                    r.i.func3 = OP_ADD;
                    r.i.rs1   = 2;
                    r.i.imm   = (((instc.ciw.imm << 2) & 0xf0) +
                                 ((instc.ciw.imm >> 4) & 0xc0) +
                                 ((instc.ciw.imm << 1) & 0x02) +
                                 ((instc.ciw.imm >> 1) & 0x01) ) * 4;
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
                r.i.imm   = ((instc.cl.imm_h >> 1) +
                             ((instc.cl.imm_l >> 1) & 0x01) +
                             ((instc.cl.imm_l << 4) & 0x10) ) * 4;
                break;
            case OP_CSW       : // c.sw -> sw rs2′, offset[6:2](rs1′)
                r.i.op    = OP_STORE;
                r.i.rd    = 8 + instc.cl.rd_;
                r.i.func3 = OP_SW;
                r.i.rs1   = 8 + instc.cl.rs1_;
                r.i.imm   = ((instc.cl.imm_h >> 1) +
                             ((instc.cl.imm_l >> 1) & 0x01) +
                             ((instc.cl.imm_l << 4) & 0x10) ) * 4;
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
                    r.i.imm   = (instc.ci.imm_h << 5) + instc.ci.imm_l;
                } else {
                    *illegal = 1;
                }
                break;
            case OP_CBEQZ     : // c.beqz -> beq rs1′, x0, offset[8:1]
                r.b.op    = OP_BRANCH;
                r.b.imm1  = (instc.cb.imm_l & 6) + ((instc.cb.imm_h >> 2) & 1) +
                            ((instc.cb.imm_h << 3) & 0x18);
                r.b.func3 = OP_BEQ;
                r.b.rs1   = instc.cb.rs1_ + 8;
                r.b.rs2   = 0;
                r.b.imm2  = ((instc.cb.imm_h & 4) ? 0x78 : 0) +
                            (instc.cb.imm_l & 1) +
                            ((instc.cb.imm_l >> 2) & 6);
                break;
            case OP_CBNEZ     : // c.bnez -> bne rs1′, x0, offset[8:1]
                r.b.op    = OP_BRANCH;
                r.b.imm1  = (instc.cb.imm_l & 6) + ((instc.cb.imm_h >> 2) & 1) +
                            ((instc.cb.imm_h << 3) & 0x18);
                r.b.func3 = OP_BNE;
                r.b.rs1   = instc.cb.rs1_ + 8;
                r.b.rs2   = 0;
                r.b.imm2  = ((instc.cb.imm_h & 4) ? 0x78 : 0) +
                            (instc.cb.imm_l & 1) +
                            ((instc.cb.imm_l >> 2) & 6);
                break;
            case OP_CJ        : // c.j -> jal x0, offset[11:1]
                r.j.op  = OP_JAL;
                r.j.rd  = 0;
                r.j.imm = ((instc.cj.offset & 0x400) ? 0x801ff : 0) +
                          ((instc.cj.offset & 0x001) << 13) +
                          ((instc.cj.offset & 0x007) << 9) +
                          ((instc.cj.offset & 0x010) << 15) +
                          ((instc.cj.offset & 0x020) << 14) +
                          ((instc.cj.offset & 0x040) << 18) +
                          ((instc.cj.offset & 0x180) << 16) +
                          ((instc.cj.offset & 0x200) << 12);
                break;
            case OP_CJAL      : // c.jal -> jal x1, offset[11:1]
                r.j.op  = OP_JAL;
                r.j.rd  = 1;
                r.j.imm = ((instc.cj.offset & 0x400) ? 0x801ff : 0) +
                          ((instc.cj.offset & 0x001) << 13) +
                          ((instc.cj.offset & 0x007) << 9) +
                          ((instc.cj.offset & 0x010) << 15) +
                          ((instc.cj.offset & 0x020) << 14) +
                          ((instc.cj.offset & 0x040) << 18) +
                          ((instc.cj.offset & 0x180) << 16) +
                          ((instc.cj.offset & 0x200) << 12);
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
                        r.i.imm   = ((instc.ci.imm_h ? 0xe0 : 0) +
                                     ((instc.ci.imm_l >> 4) & 0x01) +
                                     ((instc.ci.imm_l >> 1) & 0x04) +
                                     ((instc.ci.imm_l << 2) & 0x18) +
                                     ((instc.ci.imm_l << 1) & 0x02) ) * 16;
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
                        // TODO
                        break;
                    case 3:
                        if (instc.ca.func6 == 0x27) { // Reserved
                            *illegal = 1;
                            break;
                        }
                        switch(instc.ca.func2) {
                            case 0: // c.sub -> sub rd′, rd′, rs2′
                                // TODO
                                break;
                            case 1: // c.xor -> xor rd′, rd′, rs2′
                                // TODO
                                break;
                            case 2: // c.or -> or rd′, rd′, rs2'
                                // TODO
                                break;
                            case 3: // c.and -> and rd′, rd′, rs2′
                                // TODO
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
                    // TODO
                    break;
                }
                if (instc.ci.imm_h == 0 && instc.ci.rd != 0 && instc.ci.imm_l != 0) { // c.mv -> add rd, x0, rs2
                    // TODO
                    break;
                }
                if (instc.ci.imm_h == 1 && instc.ci.rd == 0 && instc.ci.imm_l == 0) { // c.ebreak -> ebreak
                    // TODO
                    break;
                }
                if (instc.ci.imm_h == 1 && instc.ci.rd != 0 && instc.ci.imm_l == 0) { // c.jalr -> jalr x1, 0(rs1)
                    // TODO
                    break;
                }
                if (instc.ci.imm_h == 1 && instc.ci.rd != 0 && instc.ci.imm_l != 0) { // c.add -> add rd, rd, rs2
                    // TODO
                    break;
                }
                *illegal = 1;
                break;
            case OP_CLWSP   : // c.lwsp -> lw rd, offset[7:2](x2)
                // TODO
                break;
            case OP_CSLLI   : // c.slli -> slli rd, rd, shamt[5:0]
                // TODO
                break;
            case OP_CSWSP   : // c.swsp -> sw rs2, offset[7:2](x2)
                // TODO
                break;
            default: *illegal = 1;
        }
        inst->inst = r.inst;
        return 1;
    }

    return 0;
}

