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

// ============================================================
// RISC-V top module
// ============================================================
module top (
    input                   clk,
    input                   resetb,

    input                   stall,
    output                  exception,

    // interrupt
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
    input           [31: 0] dmem_rdata,

    output                  ex_irq
);

    `include                "opcode.vh"

    wire                    timer_irq;
    wire                    timer_en;

    wire                    twready;
    wire                    twvalid;
    wire            [31: 0] twaddr;
    wire            [31: 0] twdata;
    wire            [ 3: 0] twstrb;

    wire                    trready;
    wire                    trvalid;
    wire            [31: 0] traddr;
    wire                    trresp;
    wire            [31: 0] trdata;

    wire                    dwready;
    wire                    dwvalid;
    wire            [31: 0] dwaddr;
    wire            [31: 0] dwdata;
    wire            [ 3: 0] dwstrb;

    wire                    drready;
    wire                    drvalid;
    wire            [31: 0] draddr;
    wire                    drresp;
    wire            [31: 0] drdata;
    reg                     data_sel;
    wire                    sw_irq;

    assign dmem_wready      = dwready && (dwaddr[31:28] != MMIO_BASE);
    assign dwvalid          = (dwaddr[31:28] == MMIO_BASE) ? twvalid : dmem_wvalid;
    assign dmem_waddr       = dwaddr;
    assign dmem_wdata       = dwdata;
    assign dmem_wstrb       = dwstrb;

    assign dmem_rready      = drready && (draddr[31:28] != MMIO_BASE);
    assign drvalid          = (draddr[31:28] == MMIO_BASE) ? trvalid : dmem_rvalid;
    assign dmem_raddr       = draddr;
    assign drresp           = 1'b1; // FIXME dmem_rresp;
    assign drdata           = data_sel ? trdata : dmem_rdata; // FIXME

always @(posedge clk or negedge resetb)
begin
    if (!resetb)
        data_sel            <= 1'b0;
    else
        data_sel            <= (draddr[31:28] == MMIO_BASE) ? 1'b1 : 1'b0;
end

    riscv riscv(
        .clk                (clk),
        .resetb             (resetb),

        .stall              (stall),
        .exception          (exception),
        .timer_en           (timer_en),

        .timer_irq          (timer_irq),
        .sw_irq             (sw_irq),
        .interrupt          (interrupt),

        .imem_ready         (imem_ready),
        .imem_valid         (imem_valid),
        .imem_addr          (imem_addr),
        .imem_rresp         (imem_rresp),
        .imem_rdata         (imem_rdata),

        .dmem_wready        (dwready),
        .dmem_wvalid        (dwvalid),
        .dmem_waddr         (dwaddr),
        .dmem_wdata         (dwdata),
        .dmem_wstrb         (dwstrb),

        .dmem_rready        (drready),
        .dmem_rvalid        (drvalid),
        .dmem_raddr         (draddr),
        .dmem_rresp         (drresp),
        .dmem_rdata         (drdata)
    );

    assign twready          = dwready && (dwaddr[31:28] == MMIO_BASE);
    assign twaddr           = dwaddr;
    assign twdata           = dwdata;
    assign twstrb           = dwstrb;

    assign trready          = drready && (draddr[31:28] == MMIO_BASE);
    assign traddr           = draddr;

    clint clint(
        .clk                (clk),
        .resetb             (resetb),
        .timer_en           (timer_en),

        .wready             (twready),
        .wvalid             (twvalid),
        .waddr              (twaddr),
        .wdata              (twdata),
        .wstrb              (twstrb),

        .rready             (trready),
        .rvalid             (trvalid),
        .raddr              (traddr),
        .rresp              (trresp),
        .rdata              (trdata),

        .timer_irq          (timer_irq),
        .sw_irq             (sw_irq),
        .ex_irq             (ex_irq)
    );

endmodule

