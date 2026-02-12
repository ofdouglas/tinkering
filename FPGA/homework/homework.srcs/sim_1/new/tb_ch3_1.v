`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/12/2026 12:53:31 PM
// Design Name: 
// Module Name: tb_ch3_1_tb
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

module pipe1_en (
  input        clk,
  input        rst_n,
  input        en,
  input  [7:0] in,
  output reg [7:0] out
);

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            out <= 0;
        end else if (en == 1) begin
            out <= in;
        end
    end

endmodule

module testbench ();
    reg clk = 0;
    reg rst_n = 0;
    reg en = 0;
    reg [7:0] in;
    wire [7:0] out;

    pipe1_en dut(
        .clk(clk),
        .rst_n(rst_n),
        .en(en),
        .in(in),
        .out(out)
    );

    always #5 clk = ~clk;
    integer errors = 0;

    initial begin
        rst_n = 0;
        in = 0;
        en = 0;
        repeat (3) @(posedge clk);
        rst_n = 1;
        @(posedge clk);

        verify(8'h12, 1'b0, 8'h00);
        verify(8'h34, 1'b0, 8'h00);
        verify(8'hAB, 1'b1, 8'hAB);
        verify(8'hCD, 1'b0, 8'hAB);
        verify(8'hCD, 1'b1, 8'hCD);

        rst_n = 0;
        en = 0;
        in = 0;
        repeat (2) @(posedge clk);
        rst_n = 1;

        verify(8'h56, 1'b0, 8'h00);
        verify(8'h78, 1'b1, 8'h78);
        verify(8'h9A, 1'b0, 8'h78);   
        verify(8'hBC, 1'b1, 8'hBC);      

        if (errors == 0)
            $display("PASS");
        else
            $display("FAIL: %0d errors", errors);
        $finish;
    end

    task verify(input [7:0] in_val, input en_val, input [7:0] expected);
        begin
            @(negedge clk);
            in = in_val;
            en = en_val; 
            @(negedge clk);
            if (out !== expected) begin
                errors = errors + 1;
                $display("ERROR at t=%0t: in=%h en=%h out=%h expected=%h",
                    $time, in_val, en_val, out, expected);
            end
        end
    endtask

endmodule