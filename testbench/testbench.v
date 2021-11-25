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

//`define SINGLE_RAM  1
`define PRINT_TIMELOG

`define TOP         top.riscv

/* verilator coverage_off */

`ifdef VERILATOR
import "DPI-C" function byte getch();
`endif

`ifdef SYNTHESIS
module fpga_top(
    input           clk,
    input           resetb,
    input           stall
);
`else
`ifdef VERILATOR
module testbench(
    input           clk,
    input           resetb,
    input           stall
);
`else
module testbench();
`endif // VERILATOR
`endif // SYNTHESIS

    `include "opcode.vh"

`ifdef MEMSIZE
    localparam      IRAMSIZE    = `MEMSIZE*1024;
    localparam      IRAMBASE    = 0;
    localparam      DRAMSIZE    = `MEMSIZE*1024;
    localparam      DRAMBASE    = `MEMSIZE*1024;
`else
    localparam      IRAMSIZE    = 128*1024;
    localparam      IRAMBASE    = 0;
    localparam      DRAMSIZE    = 128*1024;
    localparam      DRAMBASE    = 128*1024;
`endif

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
    reg     [ 1: 0] fillcount;
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
        $display("");
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
            #10 $finish(2);
        end
    end
end

// stop at exception
`ifdef STOP_AT_EXCEPTION
always @(posedge clk) begin
    if (exception) begin
        $display("Exception occurs, simulation exist.");
        #10 $finish(2);
    end
end
`endif
`endif // SYNTHESIS

`ifdef SINGLE_RAM

    wire            mem_ready;
    wire            mem_valid;
    wire            mem_we;
    wire    [31: 0] mem_addr;
    wire            mem_rresp;
    wire    [31: 0] mem_rdata;
    wire    [31: 0] mem_wdata;
    wire    [ 3: 0] mem_wstrb;
    wire            ready;

    assign mem_valid = 1'b1;
    assign interrupt = ex_irq;

    assign ready =
        (mem_ready && mem_we &&
         (mem_addr == MMIO_PUTC ||
          mem_addr == MMIO_GETC ||
          mem_addr == MMIO_EXIT)) ? 1'b0 : mem_ready;

    top top (
        .clk        (clk),
        .resetb     (resetb),

        .stall      (stall),
        .exception  (exception),

        .ex_irq     (ex_irq),
        .interrupt  (interrupt),

        .mem_ready  (mem_ready),
        .mem_valid  (mem_valid),
        .mem_we     (mem_we),
        .mem_addr   (mem_addr),
        .mem_rresp  (mem_rresp),
        .mem_rdata  (mem_rdata),
        .mem_wdata  (mem_wdata),
        .mem_wstrb  (mem_wstrb)
    );

    mem1port # (
        .SIZE(IRAMSIZE+DRAMSIZE),
        .FILE("memory.bin")
    ) mem (
        .clk   (clk),
        .resetb(resetb),

        .ready (ready),
        .we    (mem_we),
        .addr  (mem_addr[31:2]),
        .rresp (mem_rresp),
        .rdata (mem_rdata),
        .wdata (mem_wdata),
        .wstrb (mem_wstrb)
    );

`ifndef SYNTHESIS
    // check memory range
    always @(posedge clk) begin
        if (mem_ready && mem_we && mem_addr == MMIO_PUTC) begin
            $write("%c", mem_wdata[7:0]);
            $fflush;
        end
        else if (mem_ready && !mem_we && mem_addr == MMIO_GETC) begin
            `ifdef VERILATOR
            mem_rdata[ 7: 0] <= getch();
            `else
            // TODO
            mem_rdata[ 7: 0] <= 8'h78; // $fgetc(STDIN);
            `endif
            mem_rdata[31: 8] <= 'd0;
        end
        else if (mem_ready && mem_we && mem_addr == MMIO_EXIT) begin
            printStatistics();
            #10 $finish(1);
        end
        else if (mem_ready &&
                 mem_addr[31:$clog2(DRAMSIZE+IRAMSIZE)] != 'd0) begin
            $display("DMEM address %x out of range", mem_addr);
            #10 $finish(2);
        end
    end

    // syscall
    always @(posedge clk) begin
        if (`TOP.wb_system && !`TOP.wb_stall) begin
            if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_EXIT) begin
                printStatistics();
                #10 $finish(2);
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_WRITE &&
                `TOP.regs[REG_A0] == 32'h1) begin // stdout
                for (i = 0; i < `TOP.regs[REG_A2]; i = i + 1) begin
                    $write("%c", mem.getb(`TOP.regs[REG_A1] + i));
                end
                /* verilator lint_off IGNOREDRETURN */
                `TOP.set_reg(REG_A0, `TOP.regs[REG_A2]);
                /* verilator lint_on IGNOREDRETURN */
                $fflush;
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_READ &&
                `TOP.regs[REG_A0] == 32'h0) begin // stdin
                // TODO
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_DUMP && dump != 0) begin
                for (i = `TOP.regs[REG_A0]; i < `TOP.regs[REG_A1]; i = i + 4) begin
                    $fdisplay(dump, "%02x%02x%02x%02x", mem.getb(i + 3),
                                                        mem.getb(i + 2),
                                                        mem.getb(i + 1),
                                                        mem.getb(i + 0));
                end
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_DUMP_BIN && dump != 0) begin
                for (i = `TOP.regs[REG_A0]; i < `TOP.regs[REG_A1]; i = i + 1) begin
                    $fwrite(dump, "%c", mem.getb(i));
                end
            end
        end
    end
`endif // SYNTHESIS

`else // SINGLE_RAM

    wire            imem_ready;
    wire            imem_valid;
    wire    [31: 0] imem_addr;
    wire            imem_rresp;
    wire    [31: 0] imem_rdata;

    wire            dmem_wready;
    wire            dmem_wvalid;
    wire    [31: 0] dmem_waddr;
    wire    [31: 0] dmem_wdata;
    wire    [ 3: 0] dmem_wstrb;

    wire            dmem_rready;
    wire            dmem_rvalid;
    wire    [31: 0] dmem_raddr;
    wire            dmem_rresp;
    reg     [31: 0] dmem_rdata;

    wire            wready;

    assign imem_valid   = 1'b1;
    assign dmem_rvalid  = 1'b1;
    assign dmem_wvalid  = 1'b1;

    assign interrupt    = ex_irq;

    assign wready =
        (dmem_wready &&
         (dmem_waddr == MMIO_PUTC ||
          dmem_waddr == MMIO_GETC ||
          dmem_waddr == MMIO_EXIT)) ? 1'b0 : dmem_wready;

    top top(
        .clk        (clk),
        .resetb     (resetb),

        .stall      (stall),
        .exception  (exception),

        .ex_irq     (ex_irq),
        .interrupt  (interrupt),

        .imem_ready (imem_ready),
        .imem_valid (imem_valid),
        .imem_addr  (imem_addr),
        .imem_rresp (imem_rresp),
        .imem_rdata (imem_rdata),

        .dmem_wready(dmem_wready),
        .dmem_wvalid(dmem_wvalid),
        .dmem_waddr (dmem_waddr),
        .dmem_wdata (dmem_wdata),
        .dmem_wstrb (dmem_wstrb),

        .dmem_rready(dmem_rready),
        .dmem_rvalid(dmem_rvalid),
        .dmem_raddr (dmem_raddr),
        .dmem_rresp (dmem_rresp),
        .dmem_rdata (dmem_rdata)
    );

`ifdef RV32C_ENABLED
    mem2r1w # (
        .SIZE(IRAMSIZE),
        .FILE("imem.bin")
    ) imem (
        .clk   (clk),
        .resetb(resetb),

        .rready(imem_ready & imem_valid),
        .wready(1'b0),
        .rresp (imem_rresp),
        .rdata (imem_rdata),
        .raddr (imem_addr[31:1]-(IRAMBASE/2)),
        .waddr (30'h0),
        .wdata (32'h0),
        .wstrb (4'h0)
    );
`else
    mem2ports # (
        .SIZE(IRAMSIZE),
        .FILE("imem.bin")
    ) imem (
        .clk   (clk),
        .resetb(resetb),

        .rready(imem_ready & imem_valid),
        .wready(1'b0),
        .rresp (imem_rresp),
        .rdata (imem_rdata),
        .raddr (imem_addr[31:2]-(IRAMBASE/4)),
        .waddr (30'h0),
        .wdata (32'h0),
        .wstrb (4'h0)
    );
`endif // RV32C_ENABLED

    mem2ports # (
        .SIZE(DRAMSIZE),
        .FILE("dmem.bin")
    ) dmem (
        .clk   (clk),
        .resetb(resetb),

        .rready(dmem_rready & dmem_rvalid),
        .wready(wready & dmem_wvalid),
        .rresp (dmem_rresp),
        .rdata (dmem_rdata),
        .raddr (dmem_raddr[31:2]-(DRAMBASE/4)),
        .waddr (dmem_waddr[31:2]-(DRAMBASE/4)),
        .wdata (dmem_wdata),
        .wstrb (dmem_wstrb)
    );

`ifndef SYNTHESIS
    // check memory range
    always @(posedge clk) begin
        if (imem_ready && imem_addr[31:$clog2(IRAMSIZE)] != 'd0) begin
            $display("IMEM address %x out of range", imem_addr);
            #10 $finish(2);
        end

        if (`TOP.dmem_wready && `TOP.dmem_waddr == MMIO_PUTC) begin
            $write("%c", dmem_wdata[7:0]);
            $fflush;
        end
        else if (`TOP.dmem_rready && `TOP.dmem_raddr == MMIO_GETC) begin
            `ifdef VERILATOR
            dmem_rdata[ 7: 0] <= getch();
            `else
            // TODO
            dmem_rdata[ 7: 0] <= 8'h78; // $fgetc(STDIN);
            `endif
            dmem_rdata[31: 8] <= 'd0;
        end
        else if (`TOP.dmem_wready && `TOP.dmem_waddr == MMIO_EXIT) begin
            printStatistics();
            #10 $finish(1);
        end
        else if (dmem_wready &&
                 dmem_waddr[31:$clog2(DRAMSIZE+IRAMSIZE)] != 'd0) begin
            $display("DMEM address %x out of range", dmem_waddr);
            #10 $finish(2);
        end
    end

    // syscall
    always @(posedge clk) begin
        if (`TOP.wb_system && !`TOP.wb_stall) begin
            if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_EXIT) begin
                printStatistics();
                #10 $finish(2);
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_WRITE &&
                `TOP.regs[REG_A0] == 32'h1) begin // stdout
                for (i = 0; i < `TOP.regs[REG_A2]; i = i + 1) begin
                    $write("%c", dmem.getb(`TOP.regs[REG_A1] - IRAMSIZE + i));
                end
                /* verilator lint_off IGNOREDRETURN */
                `TOP.set_reg(REG_A0, `TOP.regs[REG_A2]);
                /* verilator lint_on IGNOREDRETURN */
                $fflush;
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_READ &&
                `TOP.regs[REG_A0] == 32'h0) begin // stdin
                // TODO
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_DUMP && dump != 0) begin
                for (i = `TOP.regs[REG_A0]; i < `TOP.regs[REG_A1]; i = i + 4) begin
                    $fdisplay(dump, "%02x%02x%02x%02x", dmem.getb(i - IRAMSIZE + 3),
                                                        dmem.getb(i - IRAMSIZE + 2),
                                                        dmem.getb(i - IRAMSIZE + 1),
                                                        dmem.getb(i - IRAMSIZE + 0));
                end
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7] == SYS_DUMP_BIN && dump != 0) begin
                for (i = `TOP.regs[REG_A0]; i < `TOP.regs[REG_A1]; i = i + 1) begin
                    $fwrite(dump, "%c", dmem.getb(i - IRAMSIZE));
                end
            end
        end
    end
`endif // SYNTHESIS

`endif // SINGLE_RAM

`ifndef SYNTHESIS
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

always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        fillcount       <= 'd0;
    end else if (!`TOP.wb_stall && !`TOP.stall_r && !`TOP.wb_flush &&
                 fillcount != 2'b11) begin
        fillcount       <= fillcount + 1;
    end
end

always @(posedge clk) begin
    if ($test$plusargs("trace") != 0 && !`TOP.wb_stall && !`TOP.stall_r &&
        !`TOP.wb_flush && fillcount == 2'b11) begin
        `ifdef PRINT_TIMELOG
        $fwrite(fp, "%d ", top.riscv.csr_cycle[31:0]);
        `endif
        $fwrite(fp, "%08x %08x", `TOP.wb_pc, `TOP.wb_insn);
        if (`TOP.wb_mem2reg && !`TOP.wb_ld_align_excp) begin
            $fwrite(fp, " read 0x%08x => 0x%08x", `TOP.wb_raddress,
                                                  `TOP.dmem_rdata);
            if (`TOP.wb_alu2reg) begin
                $fwrite(fp, ", x%02d (%0s) <= 0x%08x\n", `TOP.wb_dst_sel,
                                                       regname, `TOP.wb_rdata);
            end else begin
                $fwrite(fp, "\n");
            end
        end else if (`TOP.wb_alu2reg) begin
            $fwrite(fp, " x%02d (%0s) <= 0x%08x\n", `TOP.wb_dst_sel, regname,
                                                    `TOP.wb_result);
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

endmodule

/* verilator coverage_on */

