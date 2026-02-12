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
    output uart_rx_out,
    input uart_tx_in
    );

    reg rst_n = 0;
    reg [7:0] tx_data = 0;
    reg tx_valid = 0;
    reg init = 0;
    wire tx_ready;
    wire [7:0] rx_data;
    wire rx_valid;

    always @(posedge clk) begin
        if (!init) begin
            rst_n <= 0;
            init <= 1;
        end else begin
            rst_n <= 1;
            if (rx_valid) begin
                tx_valid <= 1;
                tx_data <= rx_data;
            end else if (tx_valid && !tx_ready) begin
                tx_valid <= 0;
            end
        end
    end

    uart dut (
        .clk(clk),
        .rst_n(rst_n),
        .tx_data(tx_data),
        .tx_valid(tx_valid),
        .tx_ready(tx_ready),
        .tx_out(uart_rx_out),
        .rx_data(rx_data),
        .rx_valid(rx_valid),
        // .rx_ack(rx_ack),
        .rx_in(uart_tx_in)
    );

    assign led = uart_rx_out;

endmodule
