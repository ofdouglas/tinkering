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


module ledmod #(
    parameter WIDTH = 27,
    parameter [WIDTH-1:0] N = 50_000_000
    )(
    input wire       clk,
    input wire       rst_n,
    input wire [2:0] sw,
    output reg       led
    );


    reg [WIDTH-1:0] counter = 0;
    initial led = 0;
    
    always@(posedge clk or negedge rst_n) begin
      if (!rst_n) begin
        led <= 1'b0;
        counter <= {WIDTH{1'b0}};
      end else if (counter >= N) begin
        led <= ~led;
        counter <= {WIDTH{1'b0}};
      end else begin
        counter <= counter + sw;
      end
    end
endmodule