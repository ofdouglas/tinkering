`timescale 1ns / 1ps

module pipe1_en (
  input logic        clk,
  input logic        rst_n,
  input logic        en,
  input logic  [7:0] in,
  output logic [7:0] out
);
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            out <= '0;
        end else if (en) out <= in;
    end
endmodule

module testbench ();
    logic clk = 0;
    logic rst_n = 0;
    logic en = 0;
    logic [7:0] in;
    logic [7:0] out;
    logic [7:0] expected;

    pipe1_en dut(
        .clk(clk),
        .rst_n(rst_n),
        .en(en),
        .in(in),
        .out(out)
    );

    always #5 clk = ~clk;

    initial begin
        rst_n = 0;
        in = 0;
        en = 0;
        repeat (3) @(posedge clk);
        rst_n = 1;
        @(posedge clk);

        verify(8'h12, 1'b0);
        verify(8'h34, 1'b0);
        verify(8'hAB, 1'b1);
        verify(8'hCD, 1'b0);
        verify(8'hCD, 1'b1);

        rst_n = 0;
        en = 0;
        in = 0;
        repeat (2) @(posedge clk);
        rst_n = 1;

        verify(8'h56, 1'b0);    
        verify(8'h78, 1'b1);
        verify(8'h9A, 1'b0);   
        verify(8'hBC, 1'b1);      

        $finish;
    end

    task verify(input [7:0] in_val, input en_val);
        begin
            @(negedge clk);
            in = in_val;
            en = en_val; 
            expected = en_val ? in_val : out;
            @(negedge clk);
            assert(out == expected) else $fatal("ERROR at t=%0t: in=%h en=%h out=%h expected=%h",
                $time, in_val, en_val, out, expected);
        end
    endtask
endmodule