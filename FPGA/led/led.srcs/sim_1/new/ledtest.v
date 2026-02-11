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

module ledtest;
  reg clk = 0;
  reg rst_n = 0;
  reg [2:0] sw = 0;
  wire led;

  ledmod #(.WIDTH(8), .N(100)) dut 
  ( .clk(clk),
    .rst_n(rst_n),
    .sw(sw),
    .led(led)
  );

always #5 clk = ~clk;

  task run_test_case(input [2:0] sw_value, input integer delay_ns);
    begin
      rst_n = 0;
      #5;
      rst_n = 1;
      sw = sw_value;
      #delay_ns;
    end
  endtask


  initial begin
    run_test_case(3'b00, 100);
    run_test_case(3'b01, 2000);
    run_test_case(3'b10, 1000);
    run_test_case(3'b11, 1000);
    $finish;
  end

endmodule