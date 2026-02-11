`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/28/2025 03:46:10 PM
// Design Name: 
// Module Name: ledmod
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


module ledmod(
    input wire       clk,
    input wire [2:0] sw,
    output reg       led
    );
    localparam WIDTH = 27;
    localparam [WIDTH-1:0] N = 50_000_000;

    reg [WIDTH-1:0] counter = 0;
    initial led = 0;
    
    always@(posedge clk) begin
      if (counter >= N) begin
        led <= ~led;
        counter <= 0;
      end else begin
        counter <= counter + sw;
      end
    end
endmodule