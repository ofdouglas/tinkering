`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/11/2026 02:48:58 PM
// Design Name: 
// Module Name: uart
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

module uart ( 
    input wire       clk,
    input wire       rst_n,
    input wire [7:0] tx_data,
    input wire       tx_valid,
    output reg       tx_ready,
    output reg       tx_out
    );

    // 1 MHz clock enable signal from 100 MHz clock
    localparam kClockDivisionFactor = 868;
    reg [9:0] clock_div_counter = 0;
    reg clock_enable = 0;

    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            clock_div_counter <= 0;
            clock_enable <= 0;
        end else if (clock_div_counter == (kClockDivisionFactor - 1)) begin
            clock_div_counter <= 0;
            clock_enable <= 1;
        end else begin
            clock_div_counter <= clock_div_counter + 1;
            clock_enable <= 0;
        end
    end

    // Transmit buffer and implicit state machine
    reg [9:0] tx_buffer = 0;
    reg [3:0] tx_bit_count = 0;
    reg tx_active = 0;

    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            tx_out <= 1;
            tx_ready <= 1;
            tx_active <= 0;
        end else if (tx_active) begin
            if (clock_enable) begin
                tx_out <= tx_buffer[0];
                tx_buffer <= {1'b0, tx_buffer[9:1]};
                tx_bit_count <= tx_bit_count + 1'b1;
                if (tx_bit_count == 4'd9) begin
                    tx_active <= 0;
                    tx_ready <= 1;
                end
            end
        end else if (tx_valid) begin
            // Store start and stop bits to simplify shift-out logic
            tx_buffer <= {1'b1, tx_data, 1'b0};  
            tx_bit_count <= 0;
            tx_active <= 1;
            tx_ready <= 0;
        end else if (!tx_active) begin
            tx_out <= 1;
            tx_ready <= 1;
        end
    end

endmodule
