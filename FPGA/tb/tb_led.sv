`timescale 1ns / 1ps
`default_nettype none

module tb_led();
    logic clk = 0;
    logic rst_n = 0;
    logic [2:0] sw = 0;
    logic led;
    int num_led_edges = 0;
    int expected_num_rising_edges = 0;
    
    ledmod #(.WIDTH(8), .N(100)) dut (
        .clk(clk),
        .rst_n(rst_n),
        .sw(sw),
        .led(led)
    ); 

    always #5 clk = ~clk;

    always_ff @(posedge led) begin
        num_led_edges <= num_led_edges + 1;
    end

    initial begin
        static int toggle_periods [7] = '{100, 50, 34, 25, 20, 17, 14};
    
        rst_n = 0;
        sw = '0;
        repeat (2) @(posedge clk);
        rst_n = 1;

        for (int i = 0; i < 7; i++) verify(i + 1, toggle_periods[i]);
        $finish;
    end

    task verify(input int sw_val, input int ticks_till_toggle);
        int prev_num_edges;
        prev_num_edges = num_led_edges;
        sw = sw_val;
        repeat (ticks_till_toggle + 2) @(posedge clk);
        expected_num_rising_edges = prev_num_edges + 1;
        assert(num_led_edges == expected_num_rising_edges) else $fatal("ERROR at t=%0t: sw=%d edges=%d expected=%d",
            $time, sw_val, num_led_edges, expected_num_rising_edges);
        prev_num_edges = num_led_edges;
        repeat (ticks_till_toggle) @(posedge clk);
    endtask

endmodule

`default_nettype wire