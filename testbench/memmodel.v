// Memory model
// Wirtten by Kuoping Hsu, 2020, MIT license

module mem2ports # (
    parameter SIZE  = 4096,
    parameter FILE  = "memory.bin"
) (
    input               clk,
    input               resetb,

    input               rready,
    input               wready,
    output reg          rresp,
    output reg  [31: 0] rdata,
    input       [31: 2] raddr,
    input       [31: 2] waddr,
    input       [31: 0] wdata,
    input       [ 3: 0] wstrb
);

    localparam ADDRW = $clog2(SIZE/4);

    reg         [31: 0] mem [(SIZE/4)-1: 0];
    reg         [31: 0] data;
    wire   [ADDRW-1: 0] radr;
    wire   [ADDRW-1: 0] wadr;
    integer             i;
    integer             file;
    integer             r;

assign radr[ADDRW-1: 0] = raddr[ADDRW+1: 2];
assign wadr[ADDRW-1: 0] = waddr[ADDRW+1: 2];

function [7:0] getb;
    input [31:0] addr;
begin
    if (addr[31:ADDRW+2] != 0) begin
        $display("Address %08x out of range", addr);
    end

    case(addr[1:0])
        0: getb = mem[addr[ADDRW+1: 2]][8*0+7:8*0];
        1: getb = mem[addr[ADDRW+1: 2]][8*1+7:8*1];
        2: getb = mem[addr[ADDRW+1: 2]][8*2+7:8*2];
        3: getb = mem[addr[ADDRW+1: 2]][8*3+7:8*3];
    endcase
end
endfunction

initial begin
    file = $fopen(FILE, "rb");
    if (file) begin
        for (i=0; i<SIZE/4; i=i+1) begin
            r = $fread(data, file);
            if (r) begin
                mem[i][8*0+7:8*0] = data[8*3+7:8*3];
                mem[i][8*1+7:8*1] = data[8*2+7:8*2];
                mem[i][8*2+7:8*2] = data[8*1+7:8*1];
                mem[i][8*3+7:8*3] = data[8*0+7:8*0];
            end else if (!$test$plusargs("no-meminit")) begin
                mem[i] = 32'h0;
            end
        end
        $fclose(file);
    end else begin
        $display("Warning: an not open file %s", FILE);
    end
end

always @(posedge clk or negedge resetb) begin
    if (!resetb)
        rresp <= 1'b0;
    else
        rresp <= rready;
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

module mem1port # (
    parameter SIZE  = 4096,
    parameter FILE  = "memory.bin"
) (
    input               clk,
    input               resetb,

    input               ready,
    input               we,
    input       [31: 2] addr,
    output reg          rresp,
    output reg  [31: 0] rdata,
    input       [31: 0] wdata,
    input       [ 3: 0] wstrb
);

    localparam ADDRW = $clog2(SIZE/4);

    reg         [31: 0] mem [(SIZE/4)-1: 0];
    reg         [31: 0] data;
    wire   [ADDRW-1: 0] adr;
    integer             i;
    integer             file;
    integer             r;

assign adr[ADDRW-1: 0] = addr[ADDRW+1: 2];

function [7:0] getb;
    input [31:0] addr;
begin
    if (addr[31:ADDRW+2] != 0) begin
        $display("Address %08x out of range", addr);
    end

    case(addr[1:0])
        0: getb = mem[addr[ADDRW+1: 2]][8*0+7:8*0];
        1: getb = mem[addr[ADDRW+1: 2]][8*1+7:8*1];
        2: getb = mem[addr[ADDRW+1: 2]][8*2+7:8*2];
        3: getb = mem[addr[ADDRW+1: 2]][8*3+7:8*3];
    endcase
end
endfunction

initial begin
    file = $fopen(FILE, "rb");
    if (file) begin
        for (i=0; i<SIZE/4; i=i+1) begin
            r = $fread(data, file);
            if (r) begin
                mem[i][8*0+7:8*0] = data[8*3+7:8*3];
                mem[i][8*1+7:8*1] = data[8*2+7:8*2];
                mem[i][8*2+7:8*2] = data[8*1+7:8*1];
                mem[i][8*3+7:8*3] = data[8*0+7:8*0];
            end else if (!$test$plusargs("no-meminit")) begin
                mem[i] = 32'h0;
            end
        end
        $fclose(file);
    end else begin
        $display("Warning: an not open file %s", FILE);
    end
end

always @(posedge clk or negedge resetb) begin
    if (!resetb)
        rresp <= 1'b0;
    else if (ready && !we)
        rresp <= 1'b1;
    else
        rresp <= 1'b0;
end

always @(posedge clk) begin
    if (ready) begin
        if (we) begin
            if (wstrb[0]) mem[adr][8*0+7:8*0] <= wdata[8*0+7:8*0];
            if (wstrb[1]) mem[adr][8*1+7:8*1] <= wdata[8*1+7:8*1];
            if (wstrb[2]) mem[adr][8*2+7:8*2] <= wdata[8*2+7:8*2];
            if (wstrb[3]) mem[adr][8*3+7:8*3] <= wdata[8*3+7:8*3];
        end else begin
            rdata <= mem[adr];
        end
    end
end

endmodule

