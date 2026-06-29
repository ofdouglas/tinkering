module reset_control(
    input logic clk,
    output logic rst_n

    // TODO: SW reset request
    // TODO: user pushbutton reset
);


localparam int RESET_CYCLES = 8;
logic [$clog2(RESET_CYCLES + 1) - 1:0] reset_cnt = '0;
logic rst_n;

always_ff @(posedge clk) begin
    if (reset_cnt < RESET_CYCLES) begin
        reset_cnt <= reset_cnt + 1'b1;
        rst_n     <= 1'b0;
    end else begin
        rst_n     <= 1'b1;
    end
end

endmodule