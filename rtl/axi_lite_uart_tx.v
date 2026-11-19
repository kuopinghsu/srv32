// AXI4-Lite UART transmitter, 8 data bits, no parity, one stop bit.
module axi_lite_uart_tx #(
    parameter [31:0] BASE_ADDR = 32'h1000_0000,
    parameter integer CLOCKS_PER_BIT = 434
)(
    input                clk,
    input                resetb,
    output               tx,

    input       [31:0]   s_axi_awaddr,
    input       [ 2:0]   s_axi_awprot,
    input                s_axi_awvalid,
    output               s_axi_awready,
    input       [31:0]   s_axi_wdata,
    input       [ 3:0]   s_axi_wstrb,
    input                s_axi_wvalid,
    output               s_axi_wready,
    output reg  [ 1:0]   s_axi_bresp,
    output reg           s_axi_bvalid,
    input                s_axi_bready,

    input       [31:0]   s_axi_araddr,
    input       [ 2:0]   s_axi_arprot,
    input                s_axi_arvalid,
    output               s_axi_arready,
    output reg  [31:0]   s_axi_rdata,
    output reg  [ 1:0]   s_axi_rresp,
    output reg           s_axi_rvalid,
    input                s_axi_rready
);

    localparam [31:0] TXDATA = BASE_ADDR, TXSTATUS = BASE_ADDR + 32'd4;
    localparam integer COUNTER_WIDTH = CLOCKS_PER_BIT > 1 ? $clog2(CLOCKS_PER_BIT) : 1;
    localparam integer BIT_RELOAD_VALUE = CLOCKS_PER_BIT - 1;
    localparam [COUNTER_WIDTH-1:0] BIT_RELOAD = BIT_RELOAD_VALUE[COUNTER_WIDTH-1:0];

    reg aw_pending;
    reg w_pending;
    reg [31:0] write_addr;
    reg [7:0] write_byte;
    reg write_lane;

    reg [9:0] frame;
    reg [3:0] bits_left;
    reg [COUNTER_WIDTH-1:0] bit_timer;
    wire busy = bits_left != 0;
    wire send_byte = write_addr == TXDATA && write_lane;

    assign tx = frame[0];
    assign s_axi_awready = resetb && !aw_pending && !s_axi_bvalid;
    assign s_axi_wready  = resetb && !w_pending && !s_axi_bvalid;
    assign s_axi_arready = resetb && !s_axi_rvalid;

    always @(posedge clk or negedge resetb) begin
        if (!resetb) begin
            aw_pending   <= 1'b0;
            w_pending    <= 1'b0;
            write_addr   <= 32'b0;
            write_byte   <= 8'b0;
            write_lane   <= 1'b0;
            s_axi_bvalid <= 1'b0;
            s_axi_bresp  <= 2'b00;
            s_axi_rvalid <= 1'b0;
            s_axi_rresp  <= 2'b00;
            s_axi_rdata  <= 32'b0;
            frame        <= 10'h3ff;
            bits_left    <= 4'b0;
            bit_timer    <= 0;
        end else begin
            if (busy) begin
                if (bit_timer == 0) begin
                    frame     <= {1'b1, frame[9:1]};
                    bits_left <= bits_left - 1'b1;
                    bit_timer <= BIT_RELOAD;
                end else begin
                    bit_timer <= bit_timer - 1'b1;
                end
            end

            if (s_axi_awready && s_axi_awvalid) begin
                write_addr <= s_axi_awaddr;
                aw_pending <= 1'b1;
            end
            if (s_axi_wready && s_axi_wvalid) begin
                write_byte <= s_axi_wdata[7:0];
                write_lane <= s_axi_wstrb[0];
                w_pending  <= 1'b1;
            end
            if (s_axi_bvalid && s_axi_bready)
                s_axi_bvalid <= 1'b0;

            // A queued TXDATA write completes when the serializer starts it.
            if (aw_pending && w_pending && (!send_byte || !busy)) begin
                aw_pending   <= 1'b0;
                w_pending    <= 1'b0;
                s_axi_bvalid <= 1'b1;
                s_axi_bresp  <= write_addr == TXDATA ? 2'b00 : 2'b10;
                if (send_byte) begin
                    frame     <= {1'b1, write_byte, 1'b0};
                    bits_left <= 4'd10;
                    bit_timer <= BIT_RELOAD;
                end
            end

            if (s_axi_rvalid && s_axi_rready)
                s_axi_rvalid <= 1'b0;
            if (s_axi_arready && s_axi_arvalid) begin
                s_axi_rvalid <= 1'b1;
                s_axi_rresp  <= s_axi_araddr == TXSTATUS ? 2'b00 : 2'b10;
                s_axi_rdata  <= {31'b0, s_axi_araddr == TXSTATUS && !busy};
            end
        end
    end
endmodule
