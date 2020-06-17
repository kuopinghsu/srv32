// Memory model
// Wirtten by Kuoping Hsu, 2020, MIT license

module memmodel # (
    parameter SIZE  = 4096,
    parameter FILE  = "memory.bin"
) (
    input               clk,

    input               rready,
    input               wready,
    output reg  [31: 0] rdata,
    input       [31: 2] raddr,
    input       [31: 2] waddr,
    input       [31: 0] wdata,
    input       [ 3: 0] wstrb
);

    localparam ADDRW = $clog2(SIZE/4);

    reg         [31: 0] mem [(SIZE/4)-1: 0];
    wire   [ADDRW-1: 0] radr;
    wire   [ADDRW-1: 0] wadr;
    integer             i;

assign radr[ADDRW-1: 0] = raddr[ADDRW+1: 2];
assign wadr[ADDRW-1: 0] = waddr[ADDRW+1: 2];

initial begin
    if ($test$plusargs("meminit")) begin
        for (i=0; i<SIZE/4; i=i+1) mem[i] = 32'h0;
    end

    $readmemh(FILE, mem, 0, SIZE/4-1);
end

always @(posedge clk) begin
    if (rready) begin
        if (wready && radr == wadr) begin
            rdata[8*0+7:8*0] <= (wstrb[0]) ? wdata[8*0+7:8*0] : mem[radr][8*0+7:8*0];
            rdata[8*1+7:8*1] <= (wstrb[1]) ? wdata[8*1+7:8*1] : mem[radr][8*1+7:8*1];
            rdata[8*2+7:8*2] <= (wstrb[2]) ? wdata[8*2+7:8*2] : mem[radr][8*2+7:8*2];
            rdata[8*3+7:8*3] <= (wstrb[3]) ? wdata[8*3+7:8*3] : mem[radr][8*3+7:8*3];
        end else begin
            rdata <= mem[radr];
        end
    end

    if (wready) begin
        if (wstrb[0]) mem[wadr][8*0+7:8*0] <= wdata[8*0+7:8*0];
        if (wstrb[1]) mem[wadr][8*1+7:8*1] <= wdata[8*1+7:8*1];
        if (wstrb[2]) mem[wadr][8*2+7:8*2] <= wdata[8*2+7:8*2];
        if (wstrb[3]) mem[wadr][8*3+7:8*3] <= wdata[8*3+7:8*3];
    end
end

endmodule

