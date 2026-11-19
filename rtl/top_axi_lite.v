// SRV32 with a shared 32-bit AXI4-Lite master for instruction and data memory.
module top_axi_lite #(
    parameter RV32M = 1,
    parameter RV32E = 0,
    parameter RV32B = 0,
    parameter RV32C = 0
)(
    input                clk,
    input                resetb,
    input                stall,
    input                interrupt,
    output               exception,
    output               ex_irq,
    output reg           bus_error,

    output reg  [31:0]   m_axi_awaddr,
    output      [ 2:0]   m_axi_awprot,
    output reg           m_axi_awvalid,
    input                m_axi_awready,
    output reg  [31:0]   m_axi_wdata,
    output reg  [ 3:0]   m_axi_wstrb,
    output reg           m_axi_wvalid,
    input                m_axi_wready,
    input       [ 1:0]   m_axi_bresp,
    input                m_axi_bvalid,
    output               m_axi_bready,

    output reg  [31:0]   m_axi_araddr,
    output      [ 2:0]   m_axi_arprot,
    output               m_axi_arvalid,
    input                m_axi_arready,
    input       [31:0]   m_axi_rdata,
    input       [ 1:0]   m_axi_rresp,
    input                m_axi_rvalid,
    output               m_axi_rready
);

    localparam [2:0] IDLE = 3'd0, READ_ADDR = 3'd1, READ_DATA = 3'd2,
                     WRITE_REQUEST = 3'd3, WRITE_RESPONSE = 3'd4;

    reg [2:0] state;
    reg instruction_read;
    reg discard_fetch;
    wire advance;
    wire redirect;
    wire [31:0] imem_addr;
    reg imem_valid;
    reg imem_error;
    reg [31:0] imem_data;
    wire load_request;
    wire [31:0] load_addr;
    reg load_valid;
    reg load_error;
    reg store_error_pending;
    wire access_fault;
    reg [31:0] load_data;
    reg [31:0] wb_load_data;
    wire store_request;
    wire [31:0] store_addr;
    wire [31:0] store_data;
    wire [3:0] store_strb;
    wire store_done = m_axi_bready && m_axi_bvalid;

    wire store_error = store_done && m_axi_bresp != 2'b00;

    assign m_axi_awprot = 3'b001; // privileged data access
    assign m_axi_arprot = {instruction_read, 2'b01};
    assign m_axi_arvalid = state == READ_ADDR;
    assign m_axi_rready = state == READ_DATA;
    assign m_axi_bready = state == WRITE_RESPONSE;

    top #(
        .RV32M(RV32M), .RV32E(RV32E), .RV32B(RV32B), .RV32C(RV32C),
        .MEM_WAIT_STATES(1), .MEM_ACCESS_FAULTS(1)
    ) core (
        .clk(clk), .resetb(resetb), .stall(stall), .interrupt(interrupt),
        .exception(exception), .ex_irq(ex_irq),
        .imem_error(imem_error), .dmem_read_error(load_error),
        .dmem_write_error(store_error || store_error_pending), .access_fault(access_fault),
        .imem_ready(advance), .imem_redirect(redirect), .imem_valid(imem_valid), .imem_addr(imem_addr),
        .imem_rresp(imem_valid), .imem_rdata(imem_data),
        .dmem_wready(store_request), .dmem_wvalid(store_done),
        .dmem_waddr(store_addr), .dmem_wdata(store_data), .dmem_wstrb(store_strb),
        .dmem_rready(load_request), .dmem_rvalid(load_valid),
        .dmem_raddr(load_addr), .dmem_rresp(1'b1), .dmem_rdata(wb_load_data)
    );

    always @(posedge clk or negedge resetb) begin
        if (!resetb) begin
            state         <= IDLE;
            instruction_read <= 1'b0;
            discard_fetch <= 1'b0;
            imem_valid    <= 1'b0;
            imem_error    <= 1'b0;
            imem_data     <= 32'h00000013;
            load_valid    <= 1'b0;
            load_error    <= 1'b0;
            store_error_pending <= 1'b0;
            load_data     <= 32'b0;
            wb_load_data  <= 32'b0;
            bus_error     <= 1'b0;
            m_axi_araddr  <= 32'b0;
            m_axi_awaddr  <= 32'b0;
            m_axi_awvalid <= 1'b0;
            m_axi_wdata   <= 32'b0;
            m_axi_wstrb   <= 4'b0;
            m_axi_wvalid  <= 1'b0;
        end else begin
            if (access_fault)
                bus_error <= 1'b1;
            if (advance)
                store_error_pending <= 1'b0;
            else if (store_error)
                store_error_pending <= 1'b1;

            if (advance) begin
                imem_valid <= 1'b0;
                if (load_request) begin
                    load_valid   <= 1'b0;
                    wb_load_data <= load_data;
                end
            end

            // An issued AXI request must complete even if the core redirects.
            if (redirect && instruction_read && (state == READ_ADDR || state == READ_DATA))
                discard_fetch <= 1'b1;

            case (state)
                IDLE: begin
                    // Finish older stores before issuing a load or fetch.
                    if (store_request) begin
                        m_axi_awaddr  <= {store_addr[31:2], 2'b00};
                        m_axi_wdata   <= store_data;
                        m_axi_wstrb   <= store_strb;
                        m_axi_awvalid <= 1'b1;
                        m_axi_wvalid  <= 1'b1;
                        state         <= WRITE_REQUEST;
                    end else if (!store_error_pending && !redirect && ((load_request && !load_valid) || !imem_valid)) begin
                        discard_fetch    <= 1'b0;
                        instruction_read <= !(load_request && !load_valid);
                        m_axi_araddr     <= load_request && !load_valid ?
                                            {load_addr[31:2], 2'b00} : imem_addr;
                        state            <= READ_ADDR;
                    end
                end

                READ_ADDR:
                    if (m_axi_arready)
                        state <= READ_DATA;

                READ_DATA:
                    if (m_axi_rvalid) begin
                        if (instruction_read) begin
                            if (!discard_fetch && !redirect) begin
                                imem_data  <= m_axi_rresp == 2'b00 ? m_axi_rdata : 32'h00000013;
                                imem_error <= m_axi_rresp != 2'b00;
                                imem_valid <= 1'b1;
                            end
                        end else begin
                            load_error <= m_axi_rresp != 2'b00;
                            load_data  <= m_axi_rresp == 2'b00 ? m_axi_rdata : 32'b0;
                            load_valid <= 1'b1;
                        end
                        state <= IDLE;
                    end

                WRITE_REQUEST: begin
                    if (m_axi_awready)
                        m_axi_awvalid <= 1'b0;
                    if (m_axi_wready)
                        m_axi_wvalid <= 1'b0;
                    if ((!m_axi_awvalid || m_axi_awready) && (!m_axi_wvalid || m_axi_wready))
                        state <= WRITE_RESPONSE;
                end

                WRITE_RESPONSE:
                    if (m_axi_bvalid) begin
                        state <= IDLE;
                    end

                default: state <= IDLE;
            endcase
        end
    end
endmodule
