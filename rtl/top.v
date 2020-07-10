// Three pipeline stage RV32I RISCV processor
// Written by Kuoping Hsu, 2020, MIT license

// ============================================================
// Top module to merge the memory port
// ============================================================
module top (
    input                   clk,
    input                   resetb,

    input                   stall,
    output                  exception,

    output reg              mem_ready,
    input                   mem_valid,
    output reg              mem_we,
    output reg      [31: 0] mem_addr,
    input                   mem_rresp,
    input           [31: 0] mem_rdata,
    output          [31: 0] mem_wdata,
    output          [ 3: 0] mem_wstrb
);

    localparam [ 1: 0]      IMEM_READ  = 2'b01,
                            DMEM_WRITE = 2'b10,
                            DMEM_READ  = 2'b11;
    localparam              RESPLEN    = 4;

    reg             [ 1: 0] respattr [$clog2(RESPLEN)-1: 0];
    reg             [ 1: 0] state;
    reg             [ 1: 0] next_state;

    wire                    imem_ready;
    reg                     imem_valid;
    wire            [31: 0] imem_addr;
    reg                     imem_rresp;
    reg             [31: 0] imem_rdata;

    wire                    dmem_wready;
    reg                     dmem_wvalid;
    wire            [31: 0] dmem_waddr;
    wire            [31: 0] dmem_wdata;
    wire            [ 3: 0] dmem_wstrb;

    wire                    dmem_rready;
    reg                     dmem_rvalid;
    wire            [31: 0] dmem_raddr;
    reg                     dmem_rresp;
    reg             [31: 0] dmem_rdata;

    reg             [31: 0] rdaddr;

    riscv riscv(
        .clk                (clk),
        .resetb             (resetb),

        .stall              (stall),
        .exception          (exception),

        .imem_ready         (imem_ready),
        .imem_valid         (imem_valid),
        .imem_addr          (imem_addr),
        .imem_rresp         (imem_rresp),
        .imem_rdata         (imem_rdata),

        .dmem_wready        (dmem_wready),
        .dmem_wvalid        (dmem_wvalid),
        .dmem_waddr         (dmem_waddr),
        .dmem_wdata         (dmem_wdata),
        .dmem_wstrb         (dmem_wstrb),

        .dmem_rready        (dmem_rready),
        .dmem_rvalid        (dmem_rvalid),
        .dmem_raddr         (dmem_raddr),
        .dmem_rresp         (dmem_rresp),
        .dmem_rdata         (dmem_rdata)
    );

assign mem_wdata            = dmem_wdata;
assign mem_wstrb            = dmem_wstrb;

always @(posedge clk or negedge resetb)
begin
    if (!resetb)
        state               <= IMEM_READ;
    else if (!stall)
        state               <= next_state;
end

always @(posedge clk)
begin
    rdaddr                  <= dmem_raddr;
end

always @* begin
    imem_valid              = 1'b0;
    imem_rdata              = 32'h0;
    imem_rresp              = 1'b0;
    dmem_rvalid             = 1'b0;
    dmem_wvalid             = 1'b0;
    dmem_rdata              = 32'h0;
    dmem_rresp              = 1'b0;

    case (state)
        IMEM_READ: begin
            if (dmem_wready && mem_valid)
                next_state  = DMEM_WRITE;
            else if (dmem_rready && mem_valid)
                next_state  = DMEM_READ;
            else
                next_state  = IMEM_READ;

            mem_addr        = imem_addr;
            mem_ready       = imem_ready;
            mem_we          = 1'b0;
            imem_valid      = mem_valid;
            imem_rdata      = mem_rdata;
        end
        DMEM_WRITE: begin
            if (dmem_rready && mem_valid)
                next_state  = DMEM_READ;
            else if (imem_ready && mem_valid)
                next_state  = IMEM_READ;
            else
                next_state  = DMEM_WRITE;

            mem_addr        = dmem_waddr;
            mem_ready       = dmem_wready;
            mem_we          = 1'b1;
            dmem_wvalid     = mem_valid;
        end
        default: begin // DMEM_READ
            if (mem_valid)
                next_state  = IMEM_READ;
            else
                next_state  = DMEM_READ;

            mem_addr        = rdaddr;
            mem_ready       = dmem_rready;
            mem_we          = 1'b0;
            dmem_rvalid     = mem_valid;
            dmem_rdata      = mem_rdata;
        end
    endcase
end

endmodule

