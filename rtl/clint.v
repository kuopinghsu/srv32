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

module clint(
    input                   clk,
    input                   resetb,
    input                   timer_en,

    input                   wready,
    output                  wvalid,
    input           [31: 0] waddr,
    input           [31: 0] wdata,
    input           [ 3: 0] wstrb,

    input                   rready,
    output                  rvalid,
    input           [31: 0] raddr,
    output reg              rresp,
    output reg      [31: 0] rdata,

    output reg              timer_irq,
    output reg              sw_irq,
    output reg              ex_irq
);

`include "opcode.vh"

    reg             [63: 0] mtime;
    reg             [63: 0] mtimecmp;
    wire            [63: 0] mtime_nxt;

assign rvalid = 1'b1;
assign wvalid = 1'b1;
assign mtime_nxt = mtime + 1;

// IRQ generation
always @(posedge clk or negedge resetb)
begin
    if (!resetb)
        timer_irq           <= 1'b0;
    else if (mtime_nxt >= mtimecmp)
        timer_irq           <= 1'b1;
    else
        timer_irq           <= 1'b0;
end

// timer, ignore strb
always @(posedge clk or negedge resetb)
begin
    if (!resetb)
        mtime[63: 0]        <= 64'd0;
    else if (wready && waddr == MTIME_BASE)
        mtime[31: 0]        <= wdata[31: 0];
    else if (wready && waddr == (MTIME_BASE+4))
        mtime[63:32]        <= wdata[31: 0];
    else if (timer_en)
        mtime[63: 0]        <= mtime_nxt[63: 0];
end

// timer compare, ignore strb.
always @(posedge clk or negedge resetb)
begin
    if (!resetb)
        mtimecmp[63: 0] <= 64'd0;
    else if (wready && waddr == MTIMECMP_BASE)
        mtimecmp[31: 0] <= wdata[31: 0];
    else if (wready && waddr == MTIMECMP_BASE+4)
        mtimecmp[63:32] <= wdata[31: 0];
end

// MSIP is used to trigger an interrupt. The external interrupt is at D[16],
// defined by srv32 core. This is used for software self-test, it connects
// ex_irq to the interrupt pin at top level of RTL code.
always @(posedge clk or negedge resetb)
begin
    if (!resetb) begin
        sw_irq          <= 1'b0;
        ex_irq          <= 1'b0;
    end else if (wready && waddr == MSIP_BASE) begin
        sw_irq          <= wdata[0];        // software interrupt
        ex_irq          <= wdata[16];       // external interrupt
    end
end

// register read
always @(posedge clk or negedge resetb)
begin
    if (!resetb)
        rresp               <= 1'b0;
    else if (rready)
        rresp               <= 1'b1;
    else
        rresp               <= 1'b0;
end

always @(posedge clk)
begin
    if (rready) begin
        case(raddr)
            MTIME_BASE      : rdata[31: 0] <= mtime[31: 0];
            MTIME_BASE+4    : rdata[31: 0] <= mtime[63:32];
            MTIMECMP_BASE   : rdata[31: 0] <= mtimecmp[31: 0];
            MTIMECMP_BASE+4 : rdata[31: 0] <= mtimecmp[63:32];
            MSIP_BASE       : rdata[31: 0] <= {15'h0, ex_irq, 15'h0, sw_irq};
            //default       : rdata[31: 0] <= rdata[31: 0];
        endcase
    end
end

endmodule

