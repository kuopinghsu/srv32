// Three pipeline stage RV32IM RISCV processor
// Written by Kuoping Hsu, 2020, MIT license

module timer(
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
// defined by simple-rsicv. This is used for software selftest, it connects
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

