module uart_periph(
    bus_slave_interface.slave bus,
    output logic uart_tx_out,
    input  logic uart_rx_in
);

logic       uart_tx_strobe;
logic       uart_rx_valid_w;
logic       uart_tx_ready;
logic [7:0] uart_reg_tx_data;
logic [7:0] uart_rx_data_r;
logic [7:0] uart_reg8_mux = '0;

assign bus.rd_data = {24'b0, uart_reg8_mux};

uart uart_component(
    .clk      (bus.clk),
    .rst_n    (bus.rst_n),
    .tx_data  (uart_reg_tx_data),
    .tx_valid (uart_tx_strobe),
    .tx_ready (uart_tx_ready),
    .tx_out   (uart_tx_out),
    .rx_data  (uart_rx_data_r),
    .rx_valid (uart_rx_valid_w),
    .rx_in    (uart_rx_in)
);

// Write to transmit data register
always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        uart_reg_tx_data <= '0;
        uart_tx_strobe   <= 1'b0;
        bus.wr_ack       <= 1'b0;
    end else begin
        uart_tx_strobe <= 1'b0;
        bus.wr_ack     <= 1'b0;

        if (bus.valid && bus.wr_strobe[0] && (bus.addr[3:2] == 2'b01)) begin
            uart_reg_tx_data <= bus.wr_data[7:0];
            uart_tx_strobe   <= 1'b1;
            bus.wr_ack       <= 1'b1;
        end
    end
end

// Read from various registers
always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        uart_reg8_mux <= '0;
        bus.rd_valid  <= 1'b0;
        bus.error     <= 1'b0;
    end else begin
        bus.rd_valid <= 1'b0;
        bus.error    <= 1'b0;

        if (bus.valid && ~|bus.wr_strobe) begin
            unique case (bus.addr[3:2])
                2'b00: begin
                    uart_reg8_mux <= {6'b0, uart_rx_valid_w, uart_tx_ready};
                    bus.rd_valid  <= 1'b1;
                end
                2'b01: begin
                    uart_reg8_mux <= uart_reg_tx_data;
                    bus.rd_valid  <= 1'b1;
                end
                2'b10: begin
                    uart_reg8_mux <= uart_rx_data_r;
                    bus.rd_valid  <= 1'b1;
                end
                default: begin
                    bus.error <= 1'b1;
                end
            endcase
        end
    end
end

endmodule
