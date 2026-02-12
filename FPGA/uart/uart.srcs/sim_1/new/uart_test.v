`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/11/2026 02:49:41 PM
// Design Name: 
// Module Name: uart_test
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


module uart_test();

    reg clk = 0;
    reg rst_n = 0;
    reg [7:0] tx_data = 0;
    reg tx_valid = 0;
    wire tx_out;
    wire tx_ready;

    uart dut (
        .clk(clk),
        .rst_n(rst_n),
        .tx_data(tx_data),
        .tx_valid(tx_valid),
        .tx_ready(tx_ready),
        .tx_out(tx_out)
    );


    always #10 clk = ~clk;

    initial begin
        rst_n = 0;
        #10 rst_n = 1;
        tx_data = 8'hA7;
        tx_valid = 1;
        // #10 tx_valid = 0;
        #1000000000 $finish;
    end
  
endmodule
