// Copyright © 2020 Kuoping Hsu
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

`define PRINT_TIMELOG

`define TOP         top.core.riscv

/* verilator coverage_off */

`ifdef VERILATOR
import "DPI-C" function byte getch();
`endif

`ifdef SYNTHESIS
module fpga_top #(
`else
module testbench #(
`endif
    // 0: split RAM, 1: shared RAM, 2: AXI-Lite.
    parameter MEMORY_MODE = 0,
    parameter BRANCH_PREDICTOR = 1,
    parameter UART_CLOCKS_PER_BIT = 16,
`ifdef MEMSIZE
    parameter integer RAM_BYTES = `MEMSIZE * 1024,
`else
    parameter integer RAM_BYTES = MEMORY_MODE == 2 ? 512 : 128 * 1024,
`endif
    parameter integer AXI_RAM_BYTES = RAM_BYTES,
`ifdef RV32M_ENABLED
    parameter RV32M = 1,
`else
    parameter RV32M = 0,
`endif
`ifdef RV32E_ENABLED
    parameter RV32E = 1,
`else
    parameter RV32E = 0,
`endif
`ifdef RV32B_ENABLED
    parameter RV32B = 1,
`else
    parameter RV32B = 0,
`endif
`ifdef RV32C_ENABLED
    parameter RV32C = 1
`else
    parameter RV32C = 0
`endif
) (
`ifdef SYNTHESIS
    input clk, resetb, stall
`elsif VERILATOR
    input clk, resetb, stall
`endif
);

    generate
    if (MEMORY_MODE == 2) begin: axi_system
        localparam [31:0] UART_BASE = 32'h1000_0000;
        localparam CLOCKS_PER_BIT = UART_CLOCKS_PER_BIT;

        reg axi_clk = 0;
        reg axi_resetb = 0;
        reg axi_stall = 0;
        reg interrupt = 0;
        wire [31:0] awaddr, wdata, araddr, rdata;
        wire [2:0] awprot, arprot;
        wire [3:0] wstrb;
        wire awvalid, awready, wvalid, wready, bvalid, bready;
        wire arvalid, arready, rvalid, rready;
        wire [1:0] bresp, rresp;
        wire tx, exception, bus_error, unused_ex_irq;

        top_axi_lite_soc #(
            .RV32M(RV32M), .RV32E(RV32E),
            .RV32B(RV32B), .RV32C(RV32C),
            .UART_BASE(UART_BASE), .UART_CLOCKS_PER_BIT(CLOCKS_PER_BIT),
            .STATIC_BRANCH_PREDICTOR(BRANCH_PREDICTOR)
        ) dut (
            .clk(axi_clk), .resetb(axi_resetb), .stall(axi_stall), .external_interrupt(interrupt),
            .exception(exception), .ex_irq(unused_ex_irq), .bus_error(bus_error), .uart_tx(tx),
            .m_axi_awaddr(awaddr), .m_axi_awprot(awprot),
            .m_axi_awvalid(awvalid), .m_axi_awready(awready),
            .m_axi_wdata(wdata), .m_axi_wstrb(wstrb),
            .m_axi_wvalid(wvalid), .m_axi_wready(wready),
            .m_axi_bresp(bresp), .m_axi_bvalid(bvalid), .m_axi_bready(bready),
            .m_axi_araddr(araddr), .m_axi_arprot(arprot),
            .m_axi_arvalid(arvalid), .m_axi_arready(arready),
            .m_axi_rdata(rdata), .m_axi_rresp(rresp),
            .m_axi_rvalid(rvalid), .m_axi_rready(rready)
        );

`ifdef AXI_LITE_REGRESSION
        `include "axi_lite_scenarios.vh"
`else
        initial $fatal(1, "AXI-Lite mode requires stimulus and a bus response model. Use make -C sim sim_axi.");
`endif
    end else begin: ram_system

    localparam IRAMSIZE = RAM_BYTES;
    localparam DRAMSIZE = RAM_BYTES;
    localparam DRAMBASE = IRAMSIZE;

    `include "opcode.vh"
    localparam [4:0] REG_SYS = RV32E ? REG_T0 : REG_A7;

`ifndef SYNTHESIS
`ifndef VERILATOR
    reg             clk;
    reg             resetb;
    reg             stall;
`endif // !VERILATOR
`endif // !SYNTHESIS

    wire            exception;
    wire            ex_irq;
    wire            interrupt;

    reg     [31: 0] next_pc;
    reg     [ 7: 0] count;
    integer         i;
    integer         dump;
    integer         STDIN = 0;

task printStatistics;
begin
    $display("\nExcuting %0d instructions, %0d cycles, %0d.%03d CPI",
            `TOP.csr_instret, `TOP.csr_cycle,
            `TOP.csr_cycle/`TOP.csr_instret,
            (`TOP.csr_cycle * 1000 /`TOP.csr_instret) % 1000);
    $display("Program terminate");
end
endtask

`ifndef SYNTHESIS
initial begin
    if ($test$plusargs("help") != 0) begin
        $display("Usage: sim [+help] [+no-meminit] [+dump] [+trace] [prog.elf]");
        $display("");
        $display("    +help         usage help");
        $display("    +no-meminit   memory uninitialized");
        $display("    +dump         dump vcd file");
        $display("    +trace        generate trace log");
        $display("");
        $finish(0);
    end

    dump = $fopen("dump.txt", "w");

    if ($test$plusargs("dump") != 0) begin
        `ifdef VERILATOR
        $dumpfile("wave.fst");
        `else
        $dumpfile("wave.vcd");
        `endif
        $dumpvars(0, testbench);
    end

`ifndef VERILATOR
    clk             = 1'b1;
    resetb          = 1'b0;
    stall           = 1'b1;

    #100 resetb     = 1'b1;
    #100 stall      = 1'b0;
`endif // VERILATOR

end

`ifndef VERILATOR
always #10 clk      = ~clk;
`endif // VERILATOR

// check timeout if the PC do not change anymore
always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        next_pc     <= 32'h0;
        count       <= 8'h0;
    end else begin
        next_pc     <= `TOP.if_pc;

        if (next_pc == `TOP.if_pc)
            count   <= count + 1;
        else
            count   <= 8'h0;

        if (count > 100) begin
            $display("Executing timeout");
            $finish(2);
        end
    end
end

// stop at exception
`ifdef STOP_AT_EXCEPTION
always @(posedge clk) begin
    if (exception) begin
        $display("Exception occurs, simulation exist.");
        $finish(2);
    end
end
`endif
`endif // SYNTHESIS

    wire            mem_ready;
    wire            mem_valid;
    wire            mem_we;
    wire    [31: 0] mem_addr;
    wire            mem_rresp;
    wire            ram_rresp;
    wire    [31: 0] mem_rdata;
    wire    [31: 0] mem_wdata;
    wire    [ 3: 0] mem_wstrb;
    wire            ready;
    wire [31:0]     ram_rdata;
`ifndef SYNTHESIS
    reg [31:0]      result;
    reg             mmio_response;
    reg             mmio_fromhost;
    reg  [31:0]     mmio_data;
    reg             wb_trace_valid;
`endif

    assign mem_valid = 1'b1;
    assign interrupt = ex_irq;

`ifdef SYNTHESIS
    assign ready = mem_ready;
    assign mem_rresp = ram_rresp;
    assign mem_rdata = ram_rdata;
`else
    assign ready = mem_ready && mem_addr[31:28] != MMIO_BASE;
    assign mem_rresp = ram_rresp || mmio_response;
    assign mem_rdata = mmio_response ? (mmio_fromhost ? result : mmio_data) : ram_rdata;

    always @(posedge clk or negedge resetb) begin
        if (!resetb)
            wb_trace_valid <= 1'b0;
        else if (!`TOP.ex_stall)
            wb_trace_valid <= `TOP.ex_valid;
        else if (!`TOP.wb_stall)
            wb_trace_valid <= 1'b0;
    end
`endif

    localparam SINGLE_PORT = MEMORY_MODE == 1;
    wire instr_ready, instr_rresp, split_read_ready;
    wire [31:0] split_read_addr;
    wire read_ready = SINGLE_PORT ? (mem_ready && !mem_we) : split_read_ready;
    wire [31:0] read_addr = SINGLE_PORT ? mem_addr : split_read_addr;
    wire [31:0] instr_addr, instr_rdata;

    top_ram #(
        .RV32M (RV32M),
        .RV32E (RV32E),
        .RV32B (RV32B),
        .RV32C (RV32C),
        .STATIC_BRANCH_PREDICTOR (BRANCH_PREDICTOR),
        .SINGLE_PORT(SINGLE_PORT)
    ) top (
        .clk        (clk),
        .resetb     (resetb),

        .stall      (stall),
        .exception  (exception),

        .ex_irq     (ex_irq),
        .external_interrupt(interrupt),

        .mem_ready  (mem_ready),
        .mem_valid  (mem_valid),
        .mem_we     (mem_we),
        .mem_addr   (mem_addr),
        .mem_rresp  (mem_rresp),
        .mem_rdata  (mem_rdata),
        .mem_wdata  (mem_wdata),
        .mem_wstrb  (mem_wstrb),
        .instr_ready(instr_ready), .instr_valid(1'b1), .instr_addr(instr_addr),
        .instr_rresp(instr_rresp), .instr_rdata(instr_rdata),
        .split_read_ready(split_read_ready), .split_read_addr(split_read_addr)
    );

    if (MEMORY_MODE == 1) begin: memory
    assign instr_rresp = 1'b0;
    assign instr_rdata = 32'b0;
    mem1port # (
        .SIZE(IRAMSIZE+DRAMSIZE),
        .FILE("memory.bin")
    ) mem (
        .clk   (clk),
        .resetb(resetb),

        .ready (ready),
        .we    (mem_we),
        .addr  (mem_addr[31:2]),
        .rresp (ram_rresp),
        .rdata (ram_rdata),
        .wdata (mem_wdata),
        .wstrb (mem_wstrb)
    );

        function [31:0] getw;
            input [31:0] address;
            getw = mem.getw(address);
        endfunction
        function [7:0] getb;
            input [31:0] address;
            getb = mem.getb(address);
        endfunction
    end else begin: memory
    if (RV32C) begin: compressed_imem
    mem2r1w #(.SIZE(IRAMSIZE), .FILE("imem.bin")) imem (
        .clk(clk), .resetb(resetb), .rready(instr_ready), .wready(1'b0),
        .rresp(instr_rresp), .rdata(instr_rdata), .raddr(instr_addr[31:1]),
        .waddr(30'b0), .wdata(32'b0), .wstrb(4'b0)
    );
    end else begin: word_imem
    mem2ports #(.SIZE(IRAMSIZE), .FILE("imem.bin")) imem (
        .clk(clk), .resetb(resetb), .rready(instr_ready), .wready(1'b0),
        .rresp(instr_rresp), .rdata(instr_rdata), .raddr(instr_addr[31:2]),
        .waddr(30'b0), .wdata(32'b0), .wstrb(4'b0)
    );
    end
    mem2ports #(.SIZE(DRAMSIZE), .FILE("dmem.bin")) dmem (
        .clk(clk), .resetb(resetb), .rready(read_ready && read_addr[31:28] != MMIO_BASE), .wready(ready && mem_we),
        .rresp(ram_rresp), .rdata(ram_rdata),
        .raddr(read_addr[31:2] - DRAMBASE[31:2]),
        .waddr(mem_addr[31:2] - DRAMBASE[31:2]),
        .wdata(mem_wdata), .wstrb(mem_wstrb)
    );
        function [31:0] getw;
            input [31:0] address;
            getw = dmem.getw(address - IRAMSIZE);
        endfunction
        function [7:0] getb;
            input [31:0] address;
            getb = dmem.getb(address - IRAMSIZE);
        endfunction
    end

`ifndef SYNTHESIS
    // Route simulated console input separately from the RAM data output.
    always @(posedge clk or negedge resetb) begin
        if (!resetb) begin
            mmio_response <= 1'b0;
            mmio_fromhost <= 1'b0;
            mmio_data <= 32'b0;
        end else begin
            // Split RAM holds its read data until the next read. Hold the
            // MMIO selection too, since WB can remain active during a stall.
            if (SINGLE_PORT || read_ready) begin
                mmio_response <= read_ready &&
                                 (read_addr == MMIO_GETC || read_addr == MMIO_FROMHOST);
                mmio_fromhost <= read_ready && read_addr == MMIO_FROMHOST;
            end
            if (read_ready && read_addr == MMIO_GETC) begin
                `ifdef VERILATOR
                mmio_data <= {24'b0, getch()};
                `else
                mmio_data <= 32'h78;
                `endif
            end else if (read_ready && read_addr == MMIO_FROMHOST) begin
                mmio_data <= result;
            end
        end
    end

    // check memory range
    always @(posedge clk) begin
        if (!SINGLE_PORT && instr_ready && instr_addr >= IRAMSIZE) begin
            $display("IMEM address %x out of range", instr_addr);
            $finish(2);
        end
        if (!SINGLE_PORT && read_ready && read_addr != MMIO_GETC &&
            read_addr != MMIO_FROMHOST &&
            (read_addr < DRAMBASE || read_addr >= DRAMBASE + DRAMSIZE)) begin
            $display("DMEM address %x out of range", read_addr);
            $finish(2);
        end
        if (mem_ready && mem_we && mem_addr == MMIO_PUTC) begin
            $write("%c", mem_wdata[7:0]);
            $fflush;
        end
        else if (mem_ready && mem_we && mem_addr == MMIO_EXIT) begin
            printStatistics();
            $finish(1);
        end
        else if (mem_ready && mem_we && mem_addr == MMIO_TOHOST) begin
            host_request(mem_wdata);
        end
        else if (mem_ready &&
                 (mem_we || (mem_addr != MMIO_GETC && mem_addr != MMIO_FROMHOST)) &&
                 (mem_addr >= DRAMBASE + DRAMSIZE ||
                  (!SINGLE_PORT && mem_addr < DRAMBASE))) begin
            $display("DMEM address %x out of range", mem_addr);
            $finish(2);
        end
    end

    // syscall
    always @(posedge clk) begin
        if (wb_trace_valid && `TOP.wb_system && !`TOP.wb_stall) begin
            if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_SYS] == SYS_EXIT) begin
                printStatistics();
                $finish(2);
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_SYS] == SYS_WRITE &&
                `TOP.regs[REG_A0] == 32'h1) begin // stdout
                for (i = 0; i < `TOP.regs[REG_A2]; i = i + 1) begin
                    $write("%c", host_getb(`TOP.regs[REG_A1] + i));
                end
                /* verilator lint_off IGNOREDRETURN */
                `TOP.set_reg(REG_A0, `TOP.regs[REG_A2]);
                /* verilator lint_on IGNOREDRETURN */
                $fflush;
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_SYS] == SYS_READ &&
                `TOP.regs[REG_A0] == 32'h0) begin // stdin
                // TODO
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_SYS] == SYS_DUMP && dump != 0) begin
                for (i = `TOP.regs[REG_A0]; i < `TOP.regs[REG_A1]; i = i + 4) begin
                    $fdisplay(dump, "%02x%02x%02x%02x", host_getb(i + 3),
                                                        host_getb(i + 2),
                                                        host_getb(i + 1),
                                                        host_getb(i + 0));
                end
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_SYS] == SYS_DUMP_BIN && dump != 0) begin
                for (i = `TOP.regs[REG_A0]; i < `TOP.regs[REG_A1]; i = i + 1) begin
                    $fwrite(dump, "%c", host_getb(i));
                end
            end
        end
    end
`endif // SYNTHESIS

`ifndef SYNTHESIS

function [31:0] host_getw;
    input [31:0] address;
    begin
        host_getw = memory.getw(address);
    end
endfunction

function [7:0] host_getb;
    input [31:0] address;
    begin
        host_getb = memory.getb(address);
    end
endfunction

task host_request;
    input [31:0] request;
    integer offset;
    begin
        case (host_getw(request))
            //SYS_OPEN:  // TODO
            SYS_LSEEK:
            begin
                result[31: 0] <= 32'hffff_ffff;
            end
            //SYS_CLOSE: // TODO
            //SYS_READ:  // TODO
            SYS_WRITE:
            begin
                if (host_getw(request+'h4) == 32'h1) begin // STDOUT
                    for (offset = 0; offset < host_getw(request+'hc); offset = offset + 1) begin
                        $write("%c", host_getb(host_getw(request+'h8) + offset));
                    end
                end
                result[31: 0] <= host_getw(request+'hc);
            end
            //SYS_FSTAT: // TODO
            SYS_EXIT:
            begin
                printStatistics();
                result[31: 0] <= 'h0;
                $finish(2);
            end
            //SYS_SBRK: // TODO
            SYS_DUMP:
            begin
                if (dump != 0) begin
                    for (offset = host_getw(request+'h4);
                         offset < host_getw(request+'h8);
                         offset = offset + 4) begin
                        $fdisplay(dump, "%08x", host_getw(offset));
                    end
                end
                result[31: 0] <= 'h0;
            end
            SYS_DUMP_BIN:
            begin
                if (dump != 0) begin
                    for (offset = host_getw(request+'h4);
                         offset < host_getw(request+'h8);
                         offset = offset + 1) begin
                        $fdisplay(dump, "%c", host_getb(offset));
                    end
                end
                result[31: 0] <= 'h0;
            end
            default:
                $display("Unknown TOHOST command %x", host_getw(request));
        endcase
    end
endtask

`ifdef TRACE
////////////////////////////////////////////////////////////
// Generate trace.log
////////////////////////////////////////////////////////////
    integer         fp;

    reg [7*8:1] regname;

initial begin
    if ($test$plusargs("trace") != 0) begin
        fp = $fopen("trace.log", "w");
    end
end

always @* begin
    case(`TOP.wb_dst_sel)
        'd0: regname = "zero";
        'd1: regname = "ra";
        'd2: regname = "sp";
        'd3: regname = "gp";
        'd4: regname = "tp";
        'd5: regname = "t0";
        'd6: regname = "t1";
        'd7: regname = "t2";
        'd8: regname = "s0(fp)";
        'd9: regname = "s1";
        'd10: regname = "a0";
        'd11: regname = "a1";
        'd12: regname = "a2";
        'd13: regname = "a3";
        'd14: regname = "a4";
        'd15: regname = "a5";
        'd16: regname = "a6";
        'd17: regname = "a7";
        'd18: regname = "s2";
        'd19: regname = "s3";
        'd20: regname = "s4";
        'd21: regname = "s5";
        'd22: regname = "s6";
        'd23: regname = "s7";
        'd24: regname = "s8";
        'd25: regname = "s9";
        'd26: regname = "s10";
        'd27: regname = "s11";
        'd28: regname = "t3";
        'd29: regname = "t4";
        'd30: regname = "t5";
        'd31: regname = "t6";
        default: regname = "xx";
    endcase
end

always @(posedge clk) begin
    if ($test$plusargs("trace") != 0 && !`TOP.wb_stall && !`TOP.stall_r &&
        wb_trace_valid) begin
        `ifdef PRINT_TIMELOG
        $fwrite(fp, "%d ", `TOP.csr_cycle[31:0]);
        `endif
        $fwrite(fp, "%08x %08x", `TOP.wb_pc, `TOP.wb_insn);
        if (`TOP.wb_mem2reg && !`TOP.wb_ld_align_excp) begin
            $fwrite(fp, " read 0x%08x", `TOP.wb_raddress);
            if (`TOP.wb_alu2reg) begin
                $fwrite(fp, ", x%02d (%0s) <= 0x%08x\n", `TOP.wb_dst_sel,
                                                       regname,
                                                       (`TOP.wb_dst_sel != 0) ? `TOP.wb_rdata : 32'h0);
            end else begin
                $fwrite(fp, "\n");
            end
        end else if (`TOP.wb_alu2reg) begin
            if (!`TOP.wb_trap_nop) begin
                $fwrite(fp, " x%02d (%0s) <= 0x%08x\n", `TOP.wb_dst_sel, regname,
                                                        (`TOP.wb_dst_sel != 0) ? `TOP.wb_result : 32'h0);
            end else begin
                $fwrite(fp, "\n");
            end
        end else if (`TOP.dmem_wready) begin
            case(`TOP.wb_alu_op)
                3'h0: begin
                    case (`TOP.wb_wstrb)
                        4'b0001: $fwrite(fp, " write 0x%08x <= 0x%08x\n",
                        `TOP.dmem_waddr, {24'h0, `TOP.dmem_wdata[8*0+7:8*0]});
                        4'b0010: $fwrite(fp, " write 0x%08x <= 0x%08x\n",
                        `TOP.dmem_waddr, {24'h0, `TOP.dmem_wdata[8*1+7:8*1]});
                        4'b0100: $fwrite(fp, " write 0x%08x <= 0x%08x\n",
                        `TOP.dmem_waddr, {24'h0, `TOP.dmem_wdata[8*2+7:8*2]});
                        4'b1000: $fwrite(fp, " write 0x%08x <= 0x%08x\n",
                        `TOP.dmem_waddr, {24'h0, `TOP.dmem_wdata[8*3+7:8*3]});
                        default: ;
                    endcase
                end
                3'h1: begin
                    if (`TOP.wb_wstrb == 4'b0011)
                        $fwrite(fp, " write 0x%08x <= 0x%08x\n",
                        `TOP.dmem_waddr, {16'h0, `TOP.dmem_wdata[15:0]});
                    else if (`TOP.wb_wstrb == 4'b1100)
                        $fwrite(fp, " write 0x%08x <= 0x%08x\n",
                        `TOP.dmem_waddr, {16'h0, `TOP.dmem_wdata[31:16]});
                end
                3'h2: $fwrite(fp, " write 0x%08x <= 0x%08x\n",
                      `TOP.dmem_waddr, `TOP.dmem_wdata);
                default: $fwrite(fp, "\n");
            endcase
        end else begin
            $fwrite(fp, "\n");
        end
    end
end
`endif // TRACE
`endif // SYNTHESIS

    end
    endgenerate

endmodule

/* verilator coverage_on */
