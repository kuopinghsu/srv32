// SRV32 with an on-chip UART and a 32-bit AXI4-Lite port for external RAM.
module top_axi_lite_soc #(
    parameter RV32M = 1,
    parameter RV32E = 0,
    parameter RV32B = 0,
    parameter RV32C = 0,
    parameter [31:0] UART_BASE = 32'h1000_0000,
    parameter integer UART_CLOCKS_PER_BIT = 434
)(
    input                clk,
    input                resetb,
    input                stall,
    input                external_interrupt,
    output               exception,
    output               ex_irq,
    output               uart_tx,
    output               bus_error,

    output      [31:0]   m_axi_awaddr,
    output      [ 2:0]   m_axi_awprot,
    output               m_axi_awvalid,
    input                m_axi_awready,
    output      [31:0]   m_axi_wdata,
    output      [ 3:0]   m_axi_wstrb,
    output               m_axi_wvalid,
    input                m_axi_wready,
    input       [ 1:0]   m_axi_bresp,
    input                m_axi_bvalid,
    output               m_axi_bready,

    output      [31:0]   m_axi_araddr,
    output      [ 2:0]   m_axi_arprot,
    output               m_axi_arvalid,
    input                m_axi_arready,
    input       [31:0]   m_axi_rdata,
    input       [ 1:0]   m_axi_rresp,
    input                m_axi_rvalid,
    output               m_axi_rready
);

    wire awvalid, wvalid, bready, arvalid, rready;
    wire uart_awready, uart_wready, uart_bvalid, uart_arready, uart_rvalid;
    wire [1:0] uart_bresp, uart_rresp;
    wire [31:0] uart_rdata;

    // The core master serializes transactions and holds each address through
    // its response. Decode AWADDR for both write channels, even if W wins.
    wire uart_write = m_axi_awaddr == UART_BASE || m_axi_awaddr == UART_BASE + 32'd4;
    wire uart_read  = m_axi_araddr == UART_BASE || m_axi_araddr == UART_BASE + 32'd4;

    assign m_axi_awvalid = awvalid && !uart_write;
    assign m_axi_wvalid  = wvalid && !uart_write;
    assign m_axi_bready  = bready && !uart_write;
    assign m_axi_arvalid = arvalid && !uart_read;
    assign m_axi_rready  = rready && !uart_read;

    top_axi_lite #(
        .RV32M(RV32M), .RV32E(RV32E), .RV32B(RV32B), .RV32C(RV32C)
    ) cpu (
        .clk(clk), .resetb(resetb), .stall(stall), .interrupt(external_interrupt),
        .exception(exception), .ex_irq(ex_irq), .bus_error(bus_error),
        .m_axi_awaddr(m_axi_awaddr), .m_axi_awprot(m_axi_awprot),
        .m_axi_awvalid(awvalid), .m_axi_awready(uart_write ? uart_awready : m_axi_awready),
        .m_axi_wdata(m_axi_wdata), .m_axi_wstrb(m_axi_wstrb),
        .m_axi_wvalid(wvalid), .m_axi_wready(uart_write ? uart_wready : m_axi_wready),
        .m_axi_bresp(uart_write ? uart_bresp : m_axi_bresp),
        .m_axi_bvalid(uart_write ? uart_bvalid : m_axi_bvalid), .m_axi_bready(bready),
        .m_axi_araddr(m_axi_araddr), .m_axi_arprot(m_axi_arprot),
        .m_axi_arvalid(arvalid), .m_axi_arready(uart_read ? uart_arready : m_axi_arready),
        .m_axi_rdata(uart_read ? uart_rdata : m_axi_rdata),
        .m_axi_rresp(uart_read ? uart_rresp : m_axi_rresp),
        .m_axi_rvalid(uart_read ? uart_rvalid : m_axi_rvalid), .m_axi_rready(rready)
    );

    axi_lite_uart_tx #(
        .BASE_ADDR(UART_BASE), .CLOCKS_PER_BIT(UART_CLOCKS_PER_BIT)
    ) uart (
        .clk(clk), .resetb(resetb), .tx(uart_tx),
        .s_axi_awaddr(m_axi_awaddr), .s_axi_awprot(m_axi_awprot),
        .s_axi_awvalid(awvalid && uart_write), .s_axi_awready(uart_awready),
        .s_axi_wdata(m_axi_wdata), .s_axi_wstrb(m_axi_wstrb),
        .s_axi_wvalid(wvalid && uart_write), .s_axi_wready(uart_wready),
        .s_axi_bresp(uart_bresp), .s_axi_bvalid(uart_bvalid), .s_axi_bready(bready && uart_write),
        .s_axi_araddr(m_axi_araddr), .s_axi_arprot(m_axi_arprot),
        .s_axi_arvalid(arvalid && uart_read), .s_axi_arready(uart_arready),
        .s_axi_rdata(uart_rdata), .s_axi_rresp(uart_rresp),
        .s_axi_rvalid(uart_rvalid), .s_axi_rready(rready && uart_read)
    );
endmodule
