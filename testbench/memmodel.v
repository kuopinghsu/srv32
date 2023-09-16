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

`ifndef SINGLE_RAM
`define HAVE_MEM2PORTS 1
`else
`define HAVE_MEM1PORT 1
`endif

/* verilator coverage_off */
/* verilator lint_off DECLFILENAME */
`ifdef HAVE_MEM2PORTS
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

    reg         [31: 0] ram [(SIZE/4)-1: 0];
    reg         [31: 0] data;
    wire   [ADDRW-1: 0] radr;
    wire   [ADDRW-1: 0] wadr;
    integer             i;
    integer             file;
    integer             r;

assign radr[ADDRW-1: 0] = raddr[ADDRW+1: 2];
assign wadr[ADDRW-1: 0] = waddr[ADDRW+1: 2];

function [7:0] getb;
    input [31:0] address;
begin
    if (address[31:ADDRW+2] != 0) begin
        $display("Address %08x out of range", address);
    end

    case(address[1:0])
        0: getb = ram[address[ADDRW+1: 2]][8*0+7:8*0];
        1: getb = ram[address[ADDRW+1: 2]][8*1+7:8*1];
        2: getb = ram[address[ADDRW+1: 2]][8*2+7:8*2];
        3: getb = ram[address[ADDRW+1: 2]][8*3+7:8*3];
    endcase
end
endfunction

function [7:0] setb;
    input [31:0] address;
    input [7:0] din;
begin
    if (address[31:ADDRW+2] != 0) begin
        $display("Address %08x out of range", address);
    end

    case(address[1:0])
        0: ram[address[ADDRW+1: 2]][8*0+7:8*0] = din[7:0];
        1: ram[address[ADDRW+1: 2]][8*1+7:8*1] = din[7:0];
        2: ram[address[ADDRW+1: 2]][8*2+7:8*2] = din[7:0];
        3: ram[address[ADDRW+1: 2]][8*3+7:8*3] = din[7:0];
    endcase

    setb = din;
end
endfunction

`ifndef SYNTHESIS
initial begin
    file = $fopen(FILE, "rb");
    if (file != 0) begin
        for (i=0; i<SIZE/4; i=i+1) begin
            r = $fread(data, file);
            if (r != 0) begin
                ram[i][8*0+7:8*0] = data[8*3+7:8*3];
                ram[i][8*1+7:8*1] = data[8*2+7:8*2];
                ram[i][8*2+7:8*2] = data[8*1+7:8*1];
                ram[i][8*3+7:8*3] = data[8*0+7:8*0];
            end else if ($test$plusargs("no-meminit") == 0) begin
                ram[i] = 32'h0;
            end
        end
        $fclose(file);
    end else begin
        $display("Warning: can not open file %s", FILE);
        $finish(0);
    end
end
`endif

always @(posedge clk or negedge resetb) begin
    if (!resetb)
        rresp <= 1'b0;
    else
        rresp <= rready;
end

always @(posedge clk) begin
    if (rready) begin
        if (wready && radr == wadr) begin
            rdata[8*0+7:8*0] <= (wstrb[0]) ? wdata[8*0+7:8*0] : ram[radr][8*0+7:8*0];
            rdata[8*1+7:8*1] <= (wstrb[1]) ? wdata[8*1+7:8*1] : ram[radr][8*1+7:8*1];
            rdata[8*2+7:8*2] <= (wstrb[2]) ? wdata[8*2+7:8*2] : ram[radr][8*2+7:8*2];
            rdata[8*3+7:8*3] <= (wstrb[3]) ? wdata[8*3+7:8*3] : ram[radr][8*3+7:8*3];
        end else begin
            rdata <= ram[radr];
        end
    end

    if (wready) begin
        if (wstrb[0]) ram[wadr][8*0+7:8*0] <= wdata[8*0+7:8*0];
        if (wstrb[1]) ram[wadr][8*1+7:8*1] <= wdata[8*1+7:8*1];
        if (wstrb[2]) ram[wadr][8*2+7:8*2] <= wdata[8*2+7:8*2];
        if (wstrb[3]) ram[wadr][8*3+7:8*3] <= wdata[8*3+7:8*3];
    end
end

endmodule

`ifdef RV32C_ENABLED
module mem2r1w # (
    parameter SIZE  = 4096,
    parameter FILE  = "memory.bin"
) (
    input               clk,
    input               resetb,

    input               rready,
    input               wready,
    output reg          rresp,
    output      [31: 0] rdata,
    input       [31: 1] raddr,
    input       [31: 2] waddr,
    input       [31: 0] wdata,
    input       [ 3: 0] wstrb
);

    localparam ADDRW = $clog2(SIZE/4);

    reg         [31: 0] ram [(SIZE/4)-1: 0];
    reg         [31: 0] data;
    reg         [31: 0] rdata1;
    reg         [31: 0] rdata2;
    reg                 aligned;
    wire   [ADDRW-1: 0] radr1;
    wire   [ADDRW-1: 0] radr2;
    wire   [ADDRW-1: 0] wadr;
    integer             i;
    integer             file;
    integer             r;

assign radr1[ADDRW-1: 0] = raddr[ADDRW+1: 2];
assign radr2[ADDRW-1: 0] = raddr[ADDRW+1: 2]+1;
assign wadr[ADDRW-1: 0]  = waddr[ADDRW+1: 2];

function [7:0] getb;
    input [31:0] address;
begin
    if (address[31:ADDRW+2] != 0) begin
        $display("Address %08x out of range", address);
    end

    case(address[1:0])
        0: getb = ram[address[ADDRW+1: 2]][8*0+7:8*0];
        1: getb = ram[address[ADDRW+1: 2]][8*1+7:8*1];
        2: getb = ram[address[ADDRW+1: 2]][8*2+7:8*2];
        3: getb = ram[address[ADDRW+1: 2]][8*3+7:8*3];
    endcase
end
endfunction

function [7:0] setb;
    input [31:0] address;
    input [7:0] din;
begin
    if (address[31:ADDRW+2] != 0) begin
        $display("Address %08x out of range", address);
    end

    case(address[1:0])
        0: ram[address[ADDRW+1: 2]][8*0+7:8*0] = din[7:0];
        1: ram[address[ADDRW+1: 2]][8*1+7:8*1] = din[7:0];
        2: ram[address[ADDRW+1: 2]][8*2+7:8*2] = din[7:0];
        3: ram[address[ADDRW+1: 2]][8*3+7:8*3] = din[7:0];
    endcase

    setb = din;
end
endfunction

`ifndef SYNTHESIS
initial begin
    file = $fopen(FILE, "rb");
    if (file != 0) begin
        for (i=0; i<SIZE/4; i=i+1) begin
            r = $fread(data, file);
            if (r != 0) begin
                ram[i][8*0+7:8*0] = data[8*3+7:8*3];
                ram[i][8*1+7:8*1] = data[8*2+7:8*2];
                ram[i][8*2+7:8*2] = data[8*1+7:8*1];
                ram[i][8*3+7:8*3] = data[8*0+7:8*0];
            end else if ($test$plusargs("no-meminit") == 0) begin
                ram[i] = 32'h0;
            end
        end
        $fclose(file);
    end else begin
        $display("Warning: can not open file %s", FILE);
        $finish(0);
    end
end
`endif

assign rdata[31: 0] = aligned ? rdata1[31: 0] : {rdata2[15: 0], rdata1[31:16]};

always @(posedge clk or negedge resetb) begin
    if (!resetb)
        rresp <= 1'b0;
    else
        rresp <= rready;
end

always @(posedge clk or negedge resetb) begin
    if (!resetb)
        aligned <= 1'b0;
    else if (rready)
        aligned <= !raddr[1];
end

always @(posedge clk) begin
    if (rready) begin
        if (wready && radr1 == wadr) begin
            rdata1[8*0+7:8*0] <= (wstrb[0]) ? wdata[8*0+7:8*0] : ram[radr1][8*0+7:8*0];
            rdata1[8*1+7:8*1] <= (wstrb[1]) ? wdata[8*1+7:8*1] : ram[radr1][8*1+7:8*1];
            rdata1[8*2+7:8*2] <= (wstrb[2]) ? wdata[8*2+7:8*2] : ram[radr1][8*2+7:8*2];
            rdata1[8*3+7:8*3] <= (wstrb[3]) ? wdata[8*3+7:8*3] : ram[radr1][8*3+7:8*3];
        end else begin
            rdata1 <= ram[radr1];
        end
        if (wready && radr2 == wadr) begin
            rdata2[8*0+7:8*0] <= (wstrb[0]) ? wdata[8*0+7:8*0] : ram[radr2][8*0+7:8*0];
            rdata2[8*1+7:8*1] <= (wstrb[1]) ? wdata[8*1+7:8*1] : ram[radr2][8*1+7:8*1];
            rdata2[8*2+7:8*2] <= (wstrb[2]) ? wdata[8*2+7:8*2] : ram[radr2][8*2+7:8*2];
            rdata2[8*3+7:8*3] <= (wstrb[3]) ? wdata[8*3+7:8*3] : ram[radr2][8*3+7:8*3];
        end else begin
            rdata2 <= ram[radr2];
        end
    end

    if (wready) begin
        if (wstrb[0]) ram[wadr][8*0+7:8*0] <= wdata[8*0+7:8*0];
        if (wstrb[1]) ram[wadr][8*1+7:8*1] <= wdata[8*1+7:8*1];
        if (wstrb[2]) ram[wadr][8*2+7:8*2] <= wdata[8*2+7:8*2];
        if (wstrb[3]) ram[wadr][8*3+7:8*3] <= wdata[8*3+7:8*3];
    end
end

endmodule
`endif // RV32C_ENABLED
`endif // HAVE_MEM2PORTS

`ifdef HAVE_MEM1PORT
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

    reg         [31: 0] ram [(SIZE/4)-1: 0];
    reg         [31: 0] data;
    wire   [ADDRW-1: 0] adr;
    integer             i;
    integer             file;
    integer             r;

assign adr[ADDRW-1: 0] = addr[ADDRW+1: 2];

function [7:0] getb;
    input [31:0] address;
begin
    if (address[31:ADDRW+2] != 0) begin
        $display("Address %08x out of range", address);
    end

    case(address[1:0])
        0: getb = ram[address[ADDRW+1: 2]][8*0+7:8*0];
        1: getb = ram[address[ADDRW+1: 2]][8*1+7:8*1];
        2: getb = ram[address[ADDRW+1: 2]][8*2+7:8*2];
        3: getb = ram[address[ADDRW+1: 2]][8*3+7:8*3];
    endcase
end
endfunction

function [7:0] setb;
    input [31:0] address;
    input [7:0] din;
begin
    if (address[31:ADDRW+2] != 0) begin
        $display("Address %08x out of range", address);
    end

    case(address[1:0])
        0: ram[address[ADDRW+1: 2]][8*0+7:8*0] = din[7:0];
        1: ram[address[ADDRW+1: 2]][8*1+7:8*1] = din[7:0];
        2: ram[address[ADDRW+1: 2]][8*2+7:8*2] = din[7:0];
        3: ram[address[ADDRW+1: 2]][8*3+7:8*3] = din[7:0];
    endcase

    setb = din;
end
endfunction

`ifndef SYNTHESIS
initial begin
    file = $fopen(FILE, "rb");
    if (file != 0) begin
        for (i=0; i<SIZE/4; i=i+1) begin
            r = $fread(data, file);
            if (r != 0) begin
                ram[i][8*0+7:8*0] = data[8*3+7:8*3];
                ram[i][8*1+7:8*1] = data[8*2+7:8*2];
                ram[i][8*2+7:8*2] = data[8*1+7:8*1];
                ram[i][8*3+7:8*3] = data[8*0+7:8*0];
            end else if ($test$plusargs("no-meminit") == 0) begin
                ram[i] = 32'h0;
            end
        end
        $fclose(file);
    end else begin
        $display("Warning: can not open file %s", FILE);
        $finish(0);
    end
end
`endif

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
            if (wstrb[0]) ram[adr][8*0+7:8*0] <= wdata[8*0+7:8*0];
            if (wstrb[1]) ram[adr][8*1+7:8*1] <= wdata[8*1+7:8*1];
            if (wstrb[2]) ram[adr][8*2+7:8*2] <= wdata[8*2+7:8*2];
            if (wstrb[3]) ram[adr][8*3+7:8*3] <= wdata[8*3+7:8*3];
        end else begin
            rdata <= ram[adr];
        end
    end
end

endmodule
`endif // HAVE_MEM1PORT
/* verilator lint_on DECLFILENAME */
/* verilator coverage_on */

