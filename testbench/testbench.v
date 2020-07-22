// Testbench
// Written by Kuoping Hsu, 2020, MIT license

//`define SINGLE_RAM  1
`define PRINT_TIMELOG

`define TOP         top.riscv

`ifdef VERILATOR
module testbench(
    input           clk,
    input           resetb,
    input           stall
);
`else
module testbench();
`endif
    `include "opcode.vh"

    localparam      DRAMSIZE    = 128*1024;
    localparam      IRAMSIZE    = 128*1024;

`ifndef VERILATOR
    reg             clk;
    reg             resetb;
    reg             stall;
`endif

    wire            exception;
    wire            timer_irq;
    wire            interrupt;

    reg     [31: 0] next_pc;
    reg     [ 7: 0] count;
    reg     [ 1: 0] fillcount;
    integer         i;
    integer         dump;

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
`endif

end

`ifndef VERILATOR
always #10 clk      = ~clk;
`endif

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
    assign timer_irq = 1'b0;
    assign interrupt = 1'b0;

    assign ready =
        (mem_ready && mem_we &&
         (mem_addr == MMIO_PUTC || mem_addr == MMIO_EXIT)) ? 1'b0 : mem_ready;

    top top (
        .clk        (clk),
        .resetb     (resetb),

        .stall      (stall),
        .exception  (exception),

        .timer_irq  (timer_irq),
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

    // check memory range
    always @(posedge clk) begin
        if (mem_ready && mem_we && mem_addr == MMIO_PUTC) begin
            $write("%c", mem_wdata[7:0]); $fflush();
        end
        else if (mem_ready && mem_we && mem_addr == MMIO_EXIT) begin
            $display("\nExcuting %0d instructions, %0d cycles", `TOP.csr_instret,
                     `TOP.csr_cycle);
            $display("Program terminate");
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
            if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7][7:0] == SYS_EXIT) begin
                $display("\nExcuting %0d instructions, %0d cycles",
                        `TOP.csr_instret, `TOP.csr_cycle);
                $display("Program terminate");
                #10 $finish(2);
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7][7:0] == SYS_WRITE &&
                `TOP.regs[REG_A0] == 32'h1) begin // stdout
                for (i = 0; i < `TOP.regs[REG_A2]; i = i + 1) begin
                    $write("%c", mem.getb(`TOP.regs[REG_A1] + i));
                end
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7][7:0] == SYS_DUMP && dump != 0) begin
                for (i = `TOP.regs[REG_A0]; i < `TOP.regs[REG_A1]; i = i + 4) begin
                    $fdisplay(dump, "%02x%02x%02x%02x", mem.getb(i + 3),
                                                        mem.getb(i + 2),
                                                        mem.getb(i + 1),
                                                        mem.getb(i + 0));
                end
            end
        end
    end

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
    wire    [31: 0] dmem_rdata;

    wire            wready;
    wire            timer_irq;
    wire            interrupt;

    assign imem_valid   = 1'b1;
    assign dmem_rvalid  = 1'b1;
    assign dmem_wvalid  = 1'b1;

    assign interrupt    = 1'b0;

    assign wready =
        (dmem_wready &&
         (dmem_waddr == MMIO_PUTC || dmem_waddr == MMIO_EXIT)) ? 1'b0 : dmem_wready;

    top top(
        .clk        (clk),
        .resetb     (resetb),

        .stall      (stall),
        .exception  (exception),

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
        .raddr (imem_addr[31:2]),
        .waddr (30'h0),
        .wdata (32'h0),
        .wstrb (4'h0)
    );

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
        .raddr (dmem_raddr[31:2]),
        .waddr (dmem_waddr[31:2]),
        .wdata (dmem_wdata),
        .wstrb (dmem_wstrb)
    );

    // check memory range
    always @(posedge clk) begin
        if (imem_ready && imem_addr[31:$clog2(IRAMSIZE)] != 'd0) begin
            $display("IMEM address %x out of range", imem_addr);
            #10 $finish(2);
        end

        if (`TOP.dmem_wready && `TOP.dmem_waddr == MMIO_PUTC) begin
            $write("%c", dmem_wdata[7:0]);
        end
        else if (`TOP.dmem_wready && `TOP.dmem_waddr == MMIO_EXIT) begin
            $display("\nExcuting %0d instructions, %0d cycles", `TOP.csr_instret,
                     `TOP.csr_cycle);
            $display("Program terminate");
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
            if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7][7:0] == SYS_EXIT) begin
                $display("\nExcuting %0d instructions, %0d cycles",
                        `TOP.csr_instret, `TOP.csr_cycle);
                $display("Program terminate");
                #10 $finish(2);
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7][7:0] == SYS_WRITE &&
                `TOP.regs[REG_A0] == 32'h1) begin // stdout
                for (i = 0; i < `TOP.regs[REG_A2]; i = i + 1) begin
                    $write("%c", dmem.getb(`TOP.regs[REG_A1] - IRAMSIZE + i));
                end
            end else if (`TOP.wb_break == 2'b00 && `TOP.regs[REG_A7][7:0] == SYS_DUMP && dump != 0) begin
                for (i = `TOP.regs[REG_A0]; i < `TOP.regs[REG_A1]; i = i + 4) begin
                    $fdisplay(dump, "%02x%02x%02x%02x", dmem.getb(i - IRAMSIZE + 3),
                                                        dmem.getb(i - IRAMSIZE + 2),
                                                        dmem.getb(i - IRAMSIZE + 1),
                                                        dmem.getb(i - IRAMSIZE + 0));
                end
            end
        end
    end

`endif // SINGLE_RAM

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
                default: ;
            endcase
        end else begin
            $fwrite(fp, "\n");
        end
    end
end
`endif

endmodule

