`timescale 1ns / 1ps

module system_tb;

    localparam time CLK_PERIOD = 10ns;  // 100 MHz, matches Nexys Video sysclk

    logic clk;
    logic [7:0] led;
    logic uart_tx_out;
    logic uart_rx_in;

    system dut (
        .clk         (clk),
        .led         (led),
        .uart_tx_out (uart_tx_out),
        .uart_rx_in  (uart_rx_in)
    );

    initial begin
        clk = 1'b0;
        forever #(CLK_PERIOD / 2) clk = ~clk;
    end

    initial begin
        logic [7:0] led_prev = '0;
        forever begin
            @(posedge clk);
            if (led !== led_prev) begin
                $display("[%0t] led -> %0b", $time, led);
                led_prev = led;
            end
        end
    end

    initial begin
        #500us;
        $display("[%0t] simulation finished", $time);
        $finish;
    end

endmodule
