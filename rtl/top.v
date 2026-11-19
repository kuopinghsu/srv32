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
module top #(
    parameter RV32M = 1,
    parameter RV32E = 0,
    parameter RV32B = 0,
    parameter RV32C = 0,
    parameter MEM_WAIT_STATES = 0
)(
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

    output                  ex_irq,
    output                  imem_redirect
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

    assign dmem_wready      = dwready && (dwaddr[31:28] != CLINT_BASE);
    assign dwvalid          = (dwaddr[31:28] == CLINT_BASE) ? twvalid : dmem_wvalid;
    assign dmem_waddr       = dwaddr;
    assign dmem_wdata       = dwdata;
    assign dmem_wstrb       = dwstrb;

    assign dmem_rready      = drready && (draddr[31:28] != CLINT_BASE);
    assign drvalid          = (draddr[31:28] == CLINT_BASE) ? trvalid : dmem_rvalid;
    assign dmem_raddr       = draddr;
    assign drresp           = 1'b1; // FIXME dmem_rresp;
    assign drdata           = data_sel ? trdata : dmem_rdata; // FIXME

always @(posedge clk or negedge resetb)
begin
    if (!resetb)
        data_sel            <= 1'b0;
    else if (!MEM_WAIT_STATES || (drready && imem_ready))
        data_sel            <= (draddr[31:28] == CLINT_BASE) ? 1'b1 : 1'b0;
end

    riscv #(
        .RV32M (RV32M),
        .RV32E (RV32E),
        .RV32B (RV32B),
        .RV32C (RV32C),
        .MEM_WAIT_STATES (MEM_WAIT_STATES)
    ) riscv (
        .clk                (clk),
        .resetb             (resetb),

        .stall              (stall),
        .exception          (exception),
        .timer_en           (timer_en),

        .timer_irq          (timer_irq),
        .sw_irq             (sw_irq),
        .interrupt          (interrupt),

        .imem_ready         (imem_ready),
        .imem_redirect      (imem_redirect),
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

    assign twready          = dwready && (dwaddr[31:28] == CLINT_BASE);
    assign twaddr           = dwaddr;
    assign twdata           = dwdata;
    assign twstrb           = dwstrb;

    assign trready          = drready && (draddr[31:28] == CLINT_BASE) &&
                              (!MEM_WAIT_STATES || imem_ready);
    assign traddr           = draddr;

    clint #(
        .RV32M (RV32M),
        .RV32E (RV32E),
        .RV32B (RV32B),
        .RV32C (RV32C)
    ) clint (
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

// Single-port synchronous RAM adapter. A read request is accepted when
// mem_ready && mem_valid and completes with a later mem_rresp pulse.
/* verilator lint_off DECLFILENAME */
module top_single_ram #(
    parameter RV32M = 1,
    parameter RV32E = 0,
    parameter RV32B = 0,
    parameter RV32C = 0
)(
    input         clk,
    input         resetb,
    input         stall,
    input         external_interrupt,
    output        exception,
    output        ex_irq,

    output        mem_ready,
    input         mem_valid,
    output        mem_we,
    output [31:0] mem_addr,
    input         mem_rresp,
    input  [31:0] mem_rdata,
    output [31:0] mem_wdata,
    output [3:0]  mem_wstrb
);
    wire advance, redirect;
    wire [31:0] fetch_addr, load_addr, store_addr;
    wire load_request, store_request;

    reg fetch_valid, load_valid;
    reg [31:0] fetch_data, load_data, wb_load_data;
    reg read_pending, read_load, discard_fetch;

    wire response = read_pending && mem_rresp;
    wire fetch_response = response && !read_load && !discard_fetch;
    wire load_response = response && read_load;
    wire instruction_valid = (fetch_valid || fetch_response) && !store_request;
    wire port_available = resetb && mem_valid && (!read_pending || response);
    wire load_needed = load_request && !(load_valid || load_response);
    wire fetch_needed = !redirect && (advance || !(fetch_valid || fetch_response));
    wire store_done = port_available && store_request;

    // Pending stores are older than any read and must use the port first.
    assign mem_ready = port_available && (store_request || load_needed || fetch_needed);
    assign mem_we = store_request;
    assign mem_addr = store_request ? store_addr : load_needed ? load_addr :
                      advance ? fetch_addr + 32'd4 : fetch_addr;

    top #(
        .RV32M(RV32M), .RV32E(RV32E), .RV32B(RV32B), .RV32C(RV32C),
        .MEM_WAIT_STATES(1)
    ) core (
        .clk(clk), .resetb(resetb), .stall(stall), .interrupt(external_interrupt),
        .exception(exception), .ex_irq(ex_irq),
        .imem_ready(advance), .imem_redirect(redirect),
        .imem_valid(instruction_valid), .imem_addr(fetch_addr),
        .imem_rresp(instruction_valid), .imem_rdata(fetch_valid ? fetch_data : mem_rdata),
        .dmem_wready(store_request), .dmem_wvalid(store_done),
        .dmem_waddr(store_addr), .dmem_wdata(mem_wdata), .dmem_wstrb(mem_wstrb),
        .dmem_rready(load_request), .dmem_rvalid(load_valid || load_response), .dmem_raddr(load_addr),
        .dmem_rresp(1'b1), .dmem_rdata(wb_load_data)
    );

    always @(posedge clk or negedge resetb) begin
        if (!resetb) begin
            fetch_valid   <= 1'b0;
            load_valid    <= 1'b0;
            fetch_data    <= 32'b0;
            load_data     <= 32'b0;
            wb_load_data  <= 32'b0;
            read_pending  <= 1'b0;
            read_load     <= 1'b0;
            discard_fetch <= 1'b0;
        end else begin
            if (advance) begin
                fetch_valid <= 1'b0;
                if (load_request) begin
                    wb_load_data <= load_valid ? load_data : mem_rdata;
                    load_valid   <= 1'b0;
                end
            end

            if (read_pending && redirect && !read_load)
                discard_fetch <= 1'b1;

            // Consume synchronous responses directly when EX advances.
            // Hold responses only when the pipeline cannot use them yet.
            if (response) begin
                read_pending <= 1'b0;
                if (load_response && !advance) begin
                    load_data  <= mem_rdata;
                    load_valid <= 1'b1;
                end else if (fetch_response && !advance) begin
                    fetch_data  <= mem_rdata;
                    fetch_valid <= 1'b1;
                end
            end

            // A returning read can share a clock edge with the next request.
            if (mem_ready && !mem_we) begin
                read_pending  <= 1'b1;
                read_load     <= load_needed;
                discard_fetch <= 1'b0;
            end
        end
    end
endmodule
/* verilator lint_on DECLFILENAME */
