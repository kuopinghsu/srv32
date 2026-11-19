`timescale 1ns/1ps

module axi_lite_tb;
    localparam [31:0] UART_BASE = 32'h1000_0000;
    localparam CLOCKS_PER_BIT = 16;

    reg clk = 0;
    reg resetb = 0;
    reg stall = 0;
    reg interrupt = 0;
    reg redirect_test;
    reg [4095:0] image_path;
    initial redirect_test = $test$plusargs("redirect");
    always #5 clk = !clk;

    wire [31:0] awaddr, wdata, araddr, rdata;
    wire [2:0] awprot, arprot;
    wire [3:0] wstrb;
    wire awvalid, awready, wvalid, wready, bvalid, bready;
    wire arvalid, arready, rvalid, rready;
    wire [1:0] bresp, rresp;
    wire tx, exception, bus_error;

    top_axi_lite_soc #(
        .UART_BASE(UART_BASE), .UART_CLOCKS_PER_BIT(CLOCKS_PER_BIT)
    ) dut (
        .clk(clk), .resetb(resetb), .stall(stall), .external_interrupt(interrupt),
        .exception(exception), .ex_irq(), .bus_error(bus_error), .uart_tx(tx),
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

    integer cycle = 0;
    always @(posedge clk)
        cycle <= cycle + 1;

    reg [31:0] ram [0:127];
    reg ram_aw_pending = 0, ram_w_pending = 0;
    reg ram_bvalid = 0, ram_rvalid = 0;
    reg [31:0] ram_awaddr, ram_wdata, ram_araddr, ram_rdata;
    reg [3:0] ram_wstrb;
    reg [1:0] ram_bresp, ram_rresp;
    integer write_delay = 0, read_delay = 0;
    integer write_count = 0;
    reg saw_aw_first = 0, saw_w_first = 0;
    integer lane;
    reg first_target_read = 1;

    // Force opposite handshake orders on the first two RAM writes.
    wire allow_aw = cycle % 7 == 1 && (write_count != 1 || ram_w_pending);
    wire allow_w = cycle % 5 == 2 && (write_count != 0 || ram_aw_pending);

    assign awready = allow_aw && !ram_aw_pending;
    assign wready = allow_w && !ram_w_pending;
    assign bvalid = ram_bvalid;
    assign bresp = ram_bresp;
    assign arready = cycle % 3 == 0;
    assign rvalid = ram_rvalid;
    assign rresp = ram_rresp;
    assign rdata = ram_rdata;

    always @(posedge clk) begin
        if (awvalid && awready) begin
            ram_awaddr <= awaddr;
            ram_aw_pending <= 1;
        end
        if (wvalid && wready) begin
            ram_wdata <= wdata;
            ram_wstrb <= wstrb;
            ram_w_pending <= 1;
        end
        if (ram_aw_pending && !ram_w_pending)
            saw_aw_first <= 1;
        if (ram_w_pending && !ram_aw_pending)
            saw_w_first <= 1;
        if (ram_aw_pending && ram_w_pending) begin
            ram_aw_pending <= 0;
            ram_w_pending <= 0;
            write_count <= write_count + 1;
            write_delay <= 4;
            ram_bresp <= ram_awaddr < 512 ? 2'b00 : 2'b11;
            if (ram_awaddr < 512)
                for (lane = 0; lane < 4; lane = lane + 1)
                    if (ram_wstrb[lane])
                        ram[ram_awaddr[8:2]][8*lane +: 8] <= ram_wdata[8*lane +: 8];
        end
        if (write_delay != 0) begin
            write_delay <= write_delay - 1;
            if (write_delay == 1)
                ram_bvalid <= 1;
        end
        if (ram_bvalid && bready)
            ram_bvalid <= 0;

        if (arvalid && arready) begin
            ram_araddr <= araddr;
            read_delay <= 5;
        end
        if (read_delay != 0) begin
            read_delay <= read_delay - 1;
            if (read_delay == 1) begin
                ram_rvalid <= 1;
                if (redirect_test && ram_araddr == 36 && first_target_read) begin
                    // The IRQ invalidates this outstanding fetch. Its error must be discarded.
                    ram_rresp <= 2'b10;
                    first_target_read <= 0;
                end else begin
                    ram_rresp <= ram_araddr < 512 &&
                             !(!redirect_test && !$test$plusargs("image=") && ram_araddr == 84) ?
                             2'b00 : 2'b10;
                end
                ram_rdata <= ram[ram_araddr[8:2]];
            end
        end
        if (ram_rvalid && rready)
            ram_rvalid <= 0;
    end

    task check_frame;
        input [7:0] expected;
        reg [9:0] expected_frame;
        integer bit_index, tick;
        begin
            expected_frame = {1'b1, expected, 1'b0};
            @(negedge tx);
            for (bit_index = 0; bit_index < 10; bit_index = bit_index + 1)
                for (tick = 0; tick < CLOCKS_PER_BIT; tick = tick + 1) begin
                    @(negedge clk);
                    if (tx !== expected_frame[bit_index])
                        $fatal(1, "UART byte %h, bit %0d, clock %0d", expected, bit_index, tick);
                end
        end
    endtask

    reg uart_checked = 0;
    initial if (!$test$plusargs("image=") && !$test$plusargs("store_irq")) begin
        check_frame(8'h55);
        check_frame(8'ha6);
        uart_checked = 1;
    end

    initial if (!$test$plusargs("redirect") && !$test$plusargs("image=") &&
                !$test$plusargs("store_irq")) begin
        ram[0] = 32'h10000093; // addi x1, x0, 256
        ram[1] = 32'h05500113; // addi x2, x0, 0x55
        ram[2] = 32'h002080a3; // sb x2, 1(x1)
        ram[3] = 32'h00209123; // sh x2, 2(x1)
        ram[4] = 32'h0010c183; // lbu x3, 1(x1)
        ram[5] = 32'h0020d203; // lhu x4, 2(x1)
        ram[6] = 32'h00418463; // beq x3, x4, +8
        ram[7] = 32'h0000a023; // sw x0, 0(x1) (squashed)
        ram[8] = 32'h100002b7; // lui x5, 0x10000
        ram[9] = 32'h00328023; // sb x3, 0(x5)
        ram[10] = 32'h0042a303; // lw x6, 4(x5) (UART busy)
        ram[11] = 32'h0a600393; // addi x7, x0, 0xa6
        ram[12] = 32'h0072a023; // sw x7, 0(x5)
        ram[13] = 32'h90000437; // lui x8, 0x90000
        ram[14] = 32'h00342823; // sw x3, 16(x8) (CLINT MSIP)
        ram[15] = 32'h01042483; // lw x9, 16(x8)
        ram[16] = 32'h08000613; // addi x12, x0, 128
        ram[17] = 32'h30561073; // csrw mtvec, x12
        ram[18] = 32'h20000513; // addi x10, x0, 512
        ram[19] = 32'h00052583; // lw x11, 0(x10) (load access fault)
        ram[20] = 32'h00952023; // sw x9, 0(x10) (store access fault)
        ram[21] = 32'h06300593; // instruction fetch fault prevents this addi
        ram[22] = 32'h0090a223; // sw x9, 4(x1)
        ram[23] = 32'h0000006f; // jal x0, 0
        ram[32] = 32'h341026f3; // csrr x13, mepc
        ram[33] = 32'h00468693; // addi x13, x13, 4
        ram[34] = 32'h34169073; // csrw mepc, x13
        ram[35] = 32'h30200073; // mret
        ram[64] = 32'hdeadbeef;
        ram[65] = 32'h0;

        repeat (3) @(negedge clk);
        resetb = 1;
        wait (wvalid);
        @(negedge clk);
        stall = 1;
        repeat (20) @(negedge clk);
        stall = 0;

        // Let the failed B response arrive while the pipeline is stalled.
        wait (wvalid && awaddr == 512);
        @(negedge clk);
        stall = 1;
        repeat (25) @(negedge clk);
        if (fault_count != 1)
            $fatal(1, "Store access fault advanced while stalled");
        stall = 0;

        wait (uart_checked && ram[65] == 1);
        if (ram[64] !== 32'h005555ef || write_count != 4 || !saw_aw_first || !saw_w_first ||
            exception !== 1'b1 || bus_error !== 1'b1 || fault_count != 3 ||
            dut.cpu.core.riscv.regs[6] !== 0 || dut.cpu.core.riscv.regs[11] !== 0)
            $fatal(1, "Unexpected RAM, UART status, or bus error result");
        $display("AXI4-Lite RAM and UART TX test passed");
        $finish;
    end

    always @(posedge clk) begin
        if (resetb && dut.cpu.store_error_pending && (arvalid || awvalid || wvalid))
            $fatal(1, "AXI request issued while a store access fault was pending");
    end

    integer fault_count = 0;
    reg fault_taken = 0;
    always @(posedge clk)
        fault_taken <= resetb && dut.cpu.access_fault;

    always @(negedge clk) begin
        if (fault_taken) begin
            case (fault_count)
                0: check_fault(5, 76, 512);
                1: check_fault(7, 80, 512);
                2: check_fault(1, 84, 84);
                default: $fatal(1, "Unexpected additional access fault");
            endcase
            fault_count = fault_count + 1;
        end
    end

    task check_fault;
        input [31:0] cause, pc, address;
        begin
            if (dut.cpu.core.riscv.csr_mcause !== cause ||
                dut.cpu.core.riscv.csr_mepc !== pc ||
                dut.cpu.core.riscv.csr_mtval !== address)
                $fatal(1, "Incorrect access fault CSRs for cause %0d", cause);
        end
    endtask

    localparam [71:0] SOFTWARE_UART_OUTPUT = "AXI-Lite\n";
    integer uart_byte;
    initial if ($value$plusargs("image=%s", image_path)) begin
        $readmemh(image_path, ram);
        ram[97] = 0; // Software writes its completion signature at 0x184.
        repeat (3) @(negedge clk);
        resetb = 1;
        for (uart_byte = 8; uart_byte >= 0; uart_byte = uart_byte - 1)
            check_frame(SOFTWARE_UART_OUTPUT[uart_byte * 8 +: 8]);

        wait (ram[97] != 0);
        if (ram[97] !== 1 || ram[96] !== 32'h80fea578 || exception || bus_error ||
            !saw_aw_first || !saw_w_first)
            $fatal(1, "AXI-Lite software RAM or bus check failed");
        $display("AXI-Lite software test passed (UART: AXI-Lite)");
        $finish;
    end

    always @(negedge clk)
        if (resetb && $test$plusargs("image=") && ram[97] == 32'hffff_ffff)
            $fatal(1, "AXI-Lite software RAM readback failed");

    reg [63:0] before_trap;
    initial if ($test$plusargs("redirect")) begin
        ram[0] = 32'h10000093; // addi x1, x0, 256
        ram[1] = 32'h30509073; // csrw mtvec, x1
        ram[2] = 32'h00001137; // lui x2, 1
        ram[3] = 32'h00115113; // srli x2, x2, 1
        ram[4] = 32'h30411073; // csrw mie, x2 (enable external interrupt)
        ram[5] = 32'h30046073; // csrsi mstatus, 8 (enable interrupts)
        ram[6] = 32'h1e40006f; // jal x0, 508 (last RAM word)
        ram[127] = 32'he29ff06f; // jal x0, 36
        ram[9] = 32'h00700193; // addi x3, x0, 7
        ram[10] = 32'h00000073; // ecall
        ram[64] = 32'h30200073; // mret

        repeat (3) @(negedge clk);
        resetb = 1;

        // Interrupt after AR is asserted and before the target enters EX.
        wait (arvalid && araddr == 36);
        @(negedge clk);
        before_trap = dut.cpu.core.riscv.csr_instret;
        interrupt = 1;
        @(posedge clk);
        while (!(dut.cpu.redirect && dut.cpu.core.riscv.ex_interrupt))
            @(posedge clk);
        @(negedge clk);
        interrupt = 0;
        if (dut.cpu.core.riscv.csr_mepc !== 32'd36)
            $fatal(1, "IRQ during redirect bubble saved the wrong PC");
        if (before_trap !== 64'd8 || dut.cpu.core.riscv.csr_instret !== before_trap)
            $fatal(1, "A reset or redirect bubble incremented instret");

        @(posedge clk);
        while (!(dut.cpu.redirect && dut.cpu.core.riscv.ex_insn == 32'h00000073))
            @(posedge clk);
        @(negedge clk);
        if (dut.cpu.core.riscv.regs[3] !== 32'd7)
            $fatal(1, "MRET failed to resume at the jump target");
        if (bus_error !== 1'b0 || first_target_read)
            $fatal(1, "Failed to discard the stale fetch error");
        if (dut.cpu.core.riscv.csr_instret !== 64'd10)
            $fatal(1, "Unexpected instret after MRET, target instruction, and ECALL");
        $display("AXI4-Lite redirect, interrupt, and retirement test passed");
        $finish;
    end

    initial if ($test$plusargs("store_irq")) begin
        ram[0] = 32'h10000093; // addi x1, x0, 256 (trap vector)
        ram[1] = 32'h30509073; // csrw mtvec, x1
        ram[2] = 32'h00001137; // lui x2, 1
        ram[3] = 32'h00115113; // srli x2, x2, 1
        ram[4] = 32'h30411073; // csrw mie, x2 (external interrupt enable)
        ram[5] = 32'h30046073; // csrsi mstatus, 8
        ram[6] = 32'h18000193; // addi x3, x0, 384
        ram[7] = 32'h0021a023; // sw x2, 0(x3)
        ram[8] = 32'h0021a223; // sw x2, 4(x3)
        ram[9] = 32'h0000006f; // jal x0, 0
        ram[64] = 32'h30200073; // mret

        repeat (3) @(negedge clk);
        resetb = 1;
        wait (awvalid && awaddr == 384);
        @(negedge clk);
        interrupt = 1;
        @(posedge clk);
        while (!(dut.cpu.redirect && dut.cpu.core.riscv.ex_interrupt))
            @(posedge clk);
        @(negedge clk);
        interrupt = 0;
        if (write_count != 1 || dut.cpu.core.riscv.csr_mepc !== 32'd32 ||
            dut.cpu.core.riscv.csr_mcause !== 32'h8000000b)
            $fatal(1, "External interrupt did not take the boundary between stores");

        wait (write_count == 2);
        @(negedge clk);
        if (ram[96] !== 32'h800 || ram[97] !== 32'h800 || exception || bus_error)
            $fatal(1, "Store did not complete after MRET");
        $display("AXI4-Lite interrupt between stores test passed");
        $finish;
    end

    initial begin
        repeat (5000) @(posedge clk);
        $fatal(1, "AXI4-Lite test timed out");
    end
endmodule
