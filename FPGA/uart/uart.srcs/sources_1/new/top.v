`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/11/2026 03:46:33 PM
// Design Name: 
// Module Name: top
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


module top(
    input clk,
    output led,
    output uart_rx_out
    );

    reg rst_n = 0;
    reg [7:0] tx_data = 0;
    reg tx_valid = 0;
    reg init = 0;
    wire tx_ready;

    always @(posedge clk) begin
        if (!init) begin
            rst_n <= 0;
            init <= 1;
            tx_data <= "H";
        end else begin
            rst_n <= 1;
            tx_valid <= 1;
            tx_data <= "H";
        end
    end

    uart dut (
        .clk(clk),
        .rst_n(rst_n),
        .tx_data(tx_data),
        .tx_valid(tx_valid),
        .tx_ready(tx_ready),
        .tx_out(uart_rx_out)
    );

    assign led = uart_rx_out;

endmodule
