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

module uart #(parameter kLoopback = 0)(
    input wire       clk,
    input wire       rst_n,

    input wire [7:0] tx_data,
    input wire       tx_valid,
    output reg       tx_ready,
    output reg       tx_out,

    output reg [7:0] rx_data,
    output reg       rx_valid,
    // input reg        rx_ack,
    input wire       rx_in
    );

    // 16x oversampled 115200 baud from 100 MHz clock
    localparam kClockDivisionFactor = 54;
    localparam kOversamplingFactor = 16;
    reg [9:0] clock_div_counter = 0;
    reg [3:0] oversampling_counter = 0;
    reg rx_clk_en = 0;
    reg tx_clk_en = 0;

    // Generate RX clock enable
    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            clock_div_counter <= 0;
            rx_clk_en <= 0;
        end else if (clock_div_counter == (kClockDivisionFactor - 1)) begin
            clock_div_counter <= 0;
            rx_clk_en <= 1;
        end else begin
            clock_div_counter <= clock_div_counter + 1;
            rx_clk_en <= 0;
        end
    end

    // Generate TX clock enable
    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            oversampling_counter <= 0;
            tx_clk_en <= 0;
        end else begin
            tx_clk_en <= 0;
            if (rx_clk_en) begin
                if (oversampling_counter == (kOversamplingFactor - 1)) begin
                    oversampling_counter <= 0;
                    tx_clk_en <= 1;
                end else begin
                    oversampling_counter <= oversampling_counter + 1;
                end
            end
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
            if (tx_clk_en) begin
                tx_out <= tx_buffer[0];
                tx_buffer <= {1'b0, tx_buffer[9:1]};
                tx_bit_count <= tx_bit_count + 1'b1;
                if (tx_bit_count == 4'd10) begin
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

    // Receive synchronizer
    reg [1:0] rx_sync = 0;
    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rx_sync <= 0;
        end else begin
            rx_sync <= {kLoopback ? tx_out : rx_in, rx_sync[1]};
        end
    end

    // Receive shift registers and implicit state machine
    reg [15:0] rx_samples = 0;
    reg [3:0] rx_sample_count = 0;
    reg [8:0] rx_bits = 0;
    reg [3:0] rx_bit_count = 0;
    reg rx_active = 0;

    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            rx_samples <= 0;
            rx_sample_count <= 0;
            rx_bits <= 0;
            rx_bit_count <= 0;
            rx_active <= 0;
            rx_valid <= 0;
            rx_data <= 0;
        end else begin
            rx_valid <= 0;
            if (rx_clk_en) begin
                rx_samples <= {rx_sync[0], rx_samples[15:1]};
                rx_sample_count <= rx_sample_count + 1;

                // Normal receive path
                if (rx_active && rx_sample_count == 8) begin
                    if (rx_bit_count == 9) begin    // All bits received
                        rx_active <= 0;
                        if (rx_bits[8] == 1) begin  // Validate stop bit
                            rx_data <= rx_bits[7:0];
                            rx_valid <= 1;
                        end
                    end else begin                  // Normal bit received
                        rx_bit_count <= rx_bit_count + 1;
                        // TODO: majority vote
                        rx_bits <= {rx_samples[8], rx_bits[8:1]};
                    end

                // Start-bit detection
                end else if (!rx_active && rx_samples[1:0] == 2'b01) begin
                    rx_active <= 1;
                    rx_sample_count <= 0;
                    rx_bit_count <= 0;
                    rx_data <= 0;
                end
            end
        end
    end

endmodule
