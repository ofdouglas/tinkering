module uart_periph(
    bus_slave_interface.slave bus,

    output logic uart_tx_out,
    input logic uart_rx_in
);


logic [7:0] uart_reg8_mux = '0;
assign bus.rd_data = {24'b0, uart_reg8_mux};
assign bus.ready = 1'b1;  // 1-cycle peripheral; interconnect uses rd_valid/wr_ack

logic       uart_tx_strobe;     // To UART component

// Offset 0 -- Status Register
logic       uart_rx_valid_w;    // From UART component
logic       uart_tx_ready;      // From UART component

// Offset 4 -- Transmit Data Register
logic [7:0] uart_reg_tx_data;    // To UART component

// Offset 8 -- Receive Data Register
logic [7:0] uart_rx_data_r;      // From UART component

uart uart_component(
    .clk(bus.clk),
    .rst_n(bus.rst_n),
    .tx_data(uart_reg_tx_data),
    .tx_valid(uart_tx_strobe),
    .tx_ready(uart_tx_ready),
    .tx_out(uart_tx_out),
    .rx_data(uart_rx_data_r),
    .rx_valid(uart_rx_valid_w),
    .rx_in(uart_rx_in)
    );


// Write to transmit data register
always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        uart_reg_tx_data <= '0;
        uart_tx_strobe   <= 0;
        bus.wr_ack       <= 0;
    end else begin
        if (uart_tx_ready && bus.valid && bus.wr_strobe[0] && (bus.addr[3:2] == 2'b01)) begin
            uart_reg_tx_data <= bus.wr_data[7:0];
            uart_tx_strobe <= 1;
            bus.wr_ack <= 1;
        end else begin
            uart_tx_strobe <= 0;
            bus.wr_ack <= 0;
        end
    end
end

// Read from various registers
always_comb begin
    uart_reg8_mux = '0;
    bus.rd_valid = 0;

    if (bus.valid && ~|bus.wr_strobe) begin
        bus.rd_valid = 1;
        case (bus.addr[3:2])
            2'b00 : uart_reg8_mux = {6'b0, uart_rx_valid_w, uart_tx_ready};
            2'b01 : uart_reg8_mux = uart_reg_tx_data;
            2'b10 : uart_reg8_mux = uart_rx_data_r;
            default : bus.rd_valid = 0;
        endcase
    end
end

endmodule
