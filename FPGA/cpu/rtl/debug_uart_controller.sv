module debug_uart_controller(
    input  logic clk,
    input  logic rst_n,
    input  logic debug_uart_rx,
    output logic debug_uart_tx,
    output logic debug_reset_request
);

uart uart_inst(
    .clk(clk),
    .rst_n(rst_n),
    .tx_data(debug_uart_tx_data),
    .tx_valid(debug_uart_tx_valid),
    .tx_ready(debug_uart_tx_ready),
    .tx_out(debug_uart_tx),
    .rx_data(debug_uart_rx_data),
    .rx_valid(debug_uart_rx_valid),
    .rx_in(debug_uart_rx)
);

// Arbitrary extended character set codepoint, which is unused by many systems.
// Used to signal a system reset request.
localparam byte unsigned CHAR_RESET_FLAG = 8'h8F; 

logic [7:0] debug_uart_rx_data;
logic [7:0] debug_uart_tx_data;
logic debug_uart_rx_valid;
logic debug_uart_tx_valid;
logic debug_uart_tx_ready;

always_ff @(posedge clk) begin
    if (!rst_n) begin
        debug_uart_tx_data  <= '0;
        debug_uart_tx_valid <= '0;
        debug_reset_request <= '0;
    end else begin
        if (debug_uart_rx_valid) begin
            debug_uart_tx_data  <= debug_uart_rx_data;
            debug_uart_tx_valid <= 1'b1;

            if (debug_uart_rx_data == CHAR_RESET_FLAG) begin
                debug_reset_request <= 1'b1;
            end
        end else begin
            debug_uart_tx_valid <= '0;
            debug_reset_request <= '0;
        end
    end
end

endmodule