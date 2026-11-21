module reset_control(
    input logic clk,
    output logic rst_n,

    input logic reset_request
    // TODO: distinct reset types, and register to read cause of reset
);


localparam int RESET_CYCLES = 8;
logic [$clog2(RESET_CYCLES + 1) - 1:0] reset_cnt = '0;

always_ff @(posedge clk) begin
    if (reset_request) begin
        reset_cnt <= '0;
        rst_n     <= 1'b0;
    end else if (reset_cnt < RESET_CYCLES) begin
        reset_cnt <= reset_cnt + 1'b1;
        rst_n     <= 1'b0;
    end else begin
        rst_n     <= 1'b1;
    end
end

endmodule