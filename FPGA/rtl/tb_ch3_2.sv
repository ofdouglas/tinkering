`timescale 1ns / 1ps
`default_nettype none

module pipe2_valid (
  input        clk,
  input        rst_n,
  input        in_valid,
  input  [7:0] in_data,
  output reg   out_valid,
  output reg [7:0] out_data
);
    reg [7:0] buf_data;
    reg       buf_valid;

    always@(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            out_valid <= 0;
            buf_valid <= 0;
            out_data <= 0;
            buf_data <= 0;
        end else if (in_valid) begin
            buf_valid <= in_valid;
            out_valid <= buf_valid;
            buf_data <= in_data;
            out_data <= buf_data;
        end
    end
endmodule

module testbench ();
    reg clk;
    reg rst_n;
    reg in_valid;
    reg [7:0] in_data;
    wire out_valid;
    wire [7:0] out_data;

    // Emulate DUT pipeline
    reg [7:0] queue [1:0];

    pipe2_valid dut (
        .clk(clk),
        .rst_n(rst_n),
        .in_valid(in_valid),
        .in_data(in_data),
        .out_valid(out_valid),
        .out_data(out_data)
    );

    always #5 clk = ~clk;

    initial begin
        clk = 0;
        rst_n = 0;
        in_valid = 0;
        in_data = 0;
        queue = 0;

        repeat (2) @posedge(clk);
        rst_n = 1;
        @posedge(clk);




    end

    task run_cycle();
        begin
            @posedge(clk);
            if (in_valid == 1) begin
                
            end


        end
    endtask


endmodule

`default_nettype wire
