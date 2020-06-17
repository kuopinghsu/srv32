// Testbench
// Written by Kuoping Hsu, 2020, MIT license

`define MEM_PUTC    32'h8000001c
`define MEM_EXIT    32'h8000002c

module testbench();
    localparam      DRAMSIZE = 128*1024;
    localparam      IRAMSIZE = 128*1024;

    reg             clk;
    reg             resetb;
    reg             stall;
    wire            exception;

    wire            imem_ready;
    wire    [31: 0] imem_rdata;
    wire            imem_valid;
    wire    [31: 0] imem_addr;

    wire            dmem_rready;
    wire            dmem_wready;
    wire    [31: 0] dmem_rdata;
    wire            dmem_rvalid;
    wire            dmem_wvalid;
    wire    [31: 0] dmem_raddr;
    wire    [31: 0] dmem_waddr;
    wire    [31: 0] dmem_wdata;
    wire    [ 3: 0] dmem_wstrb;

    reg     [31: 0] next_pc;
    reg     [ 7: 0] count;
    reg     [ 1: 0] fillcount;

assign dmem_rvalid  = 1'b1;
assign dmem_wvalid  = 1'b1;
assign imem_valid   = 1'b1;

initial begin

    if ($test$plusargs("dumpvcd")) begin
        $dumpfile("testbench.vcd");
        $dumpvars(0, testbench);
    end

    clk             <= 1'b1;
    resetb          <= 1'b0;
    stall           <= 1'b1;

    repeat (10) @(posedge clk);
    resetb          <= 1'b1;

    repeat (10) @(posedge clk);
    stall           <= 1'b0;

end

always #10 clk      <= ~clk;

// check timeout if the PC do not change anymore
always @(posedge clk or negedge resetb) begin
    if (!resetb) begin
        next_pc     <= 32'h0;
        count       <= 8'h0;
    end else begin
        next_pc     <= riscv.if_pc;

        if (next_pc == riscv.if_pc)
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
always @(posedge clk) begin
    if (exception) begin
        $display("Exception occurs, simulation exist.");
        #10 $finish(2);
    end
end

    riscv riscv(
        .clk        (clk),
        .resetb     (resetb),
        .stall      (stall),
        .exception  (exception),

        .imem_ready (imem_ready),
        .imem_rdata (imem_rdata),
        .imem_valid (imem_valid),
        .imem_addr  (imem_addr),

        .dmem_rready(dmem_rready),
        .dmem_wready(dmem_wready),
        .dmem_rdata (dmem_rdata),
        .dmem_rvalid(dmem_rvalid),
        .dmem_wvalid(dmem_wvalid),
        .dmem_raddr (dmem_raddr),
        .dmem_waddr (dmem_waddr),
        .dmem_wdata (dmem_wdata),
        .dmem_wstrb (dmem_wstrb)
    );

    memmodel # (
        .SIZE(IRAMSIZE),
        .FILE("../sw/imem.hex")
    ) imem (
        .clk   (clk),

        .rready(imem_ready),
        .wready(1'b0),
        .rdata (imem_rdata),
        .raddr (imem_addr[31:2]),
        .waddr (30'h0),
        .wdata (32'h0),
        .wstrb (4'h0)
    );

    memmodel # (
        .SIZE(DRAMSIZE),
        .FILE("../sw/dmem.hex")
    ) dmem (
        .clk   (clk),

        .rready(dmem_rready),
        .wready(dmem_wready),
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

    if (dmem_wready && dmem_waddr == `MEM_PUTC) begin
        $write("%c", dmem_wdata[7:0]);
    end
    else if (dmem_wready && dmem_waddr == `MEM_EXIT) begin
        $display("\nExcuting %0d instructions, %0d cycles", riscv.rdinstret, riscv.rdcycle);
        $display("Program terminate");
        #10 $finish(1);
    end
    else if (dmem_wready && dmem_waddr[31:$clog2(DRAMSIZE+IRAMSIZE)] != 'd0) begin
        $display("DMEM address %x out of range", dmem_waddr);
        #10 $finish(2);
    end
end

`ifdef TRACE
    integer         fp;

    reg [7*8:1] regname;

initial begin
    if ($test$plusargs("trace")) begin
        fp = $fopen("trace.log", "w");
    end
end

always @* begin
    case(riscv.wb_dst_sel)
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
    if (!resetb) begin
        fillcount       <= 'd0;
    end else if (!riscv.wb_stall && !riscv.stall_r && !riscv.wb_flush &&
                 fillcount != 2'b11) begin
        fillcount       <= fillcount + 1;
    end
end

always @(posedge clk) begin
    if ($test$plusargs("trace") && !riscv.wb_stall && !riscv.stall_r &&
        !riscv.wb_flush && fillcount == 2'b11) begin
        $fwrite(fp, "%08x %08x", riscv.wb_pc, riscv.wb_insn);
        if (riscv.wb_mem2reg) begin
            $fwrite(fp, " read 0x%08x => 0x%08x,", riscv.wb_raddress, riscv.dmem_rdata);
            $fwrite(fp, " x%02d (%0s) <= 0x%08x\n", riscv.wb_dst_sel, regname, riscv.wb_rdata);
        end else if (riscv.wb_alu2reg) begin
            $fwrite(fp, " x%02d (%0s) <= 0x%08x\n", riscv.wb_dst_sel, regname, riscv.wb_result);
        end else if (riscv.dmem_wready) begin
            case(riscv.wb_alu_op)
                3'h0: begin
                    case (riscv.wb_wstrb)
                        4'b0001: $fwrite(fp, " write 0x%08x <= 0x%08x\n", riscv.dmem_waddr, {24'h0, riscv.dmem_wdata[8*0+7:8*0]});
                        4'b0010: $fwrite(fp, " write 0x%08x <= 0x%08x\n", riscv.dmem_waddr, {24'h0, riscv.dmem_wdata[8*1+7:8*1]});
                        4'b0100: $fwrite(fp, " write 0x%08x <= 0x%08x\n", riscv.dmem_waddr, {24'h0, riscv.dmem_wdata[8*2+7:8*2]});
                        4'b1000: $fwrite(fp, " write 0x%08x <= 0x%08x\n", riscv.dmem_waddr, {24'h0, riscv.dmem_wdata[8*3+7:8*3]});
                    endcase
                end
                3'h1: begin
                    if (riscv.wb_wstrb == 4'b0011)
                        $fwrite(fp, " write 0x%08x <= 0x%08x\n", riscv.dmem_waddr, {16'h0, riscv.dmem_wdata[15:0]});
                    else if (riscv.wb_wstrb == 4'b1100)
                        $fwrite(fp, " write 0x%08x <= 0x%08x\n", riscv.dmem_waddr, {16'h0, riscv.dmem_wdata[31:16]});
                end
                3'h2: $fwrite(fp, " write 0x%08x <= 0x%08x\n", riscv.dmem_waddr, riscv.dmem_wdata);
            endcase
        end else begin
            $fwrite(fp, "\n");
        end
    end
end
`endif

endmodule

