`timescale 1ns / 1ps

module uart_test();
    reg clk = 0;
    reg rst_n = 0;

    reg [7:0] tx_data = 0;
    reg tx_valid = 0;
    wire tx_out;
    wire tx_ready;

    wire [7:0] rx_data;
    wire rx_valid;
    // reg rx_ack = 0;
    wire rx_in;

    uart #(.kLoopback(1)) dut (
        .clk(clk),
        .rst_n(rst_n),

        .tx_data(tx_data),
        .tx_valid(tx_valid),
        .tx_ready(tx_ready),
        .tx_out(tx_out),

        .rx_data(rx_data),
        .rx_valid(rx_valid),
        // .rx_ack(rx_ack),
        .rx_in(rx_in)
    );


    always #10 clk = ~clk;

    initial begin
        rst_n = 0;
        #10 rst_n = 1;
        tx_data = 8'hA7;
        tx_valid = 1;
        #1000 tx_valid = 0;
        #200000 tx_valid = 1;
        #1000 tx_valid = 0;
        #200000 $finish;
    end
  
endmodule
