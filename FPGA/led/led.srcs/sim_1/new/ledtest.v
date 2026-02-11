`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/11/2026 09:19:00 AM
// Design Name: 
// Module Name: ledtest
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

`include "ledmod.v"

module ledtest;
  reg clk = 0;
  reg [2:0] sw = 0;
  wire led;

  ledmod dut(
    .clk(clk),
    .sw(sw),
    .led(led)
  );

always #5 clk = ~clk;

always #20 sw = sw + 1;

endmodule