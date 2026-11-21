module uart_periph(
    bus_slave_interface.slave bus,
    output logic uart_irq_out,
    output logic uart_tx_out,
    input  logic uart_rx_in
);

// From the peripheral
logic       uart_rx_valid_strobe;
logic [7:0] uart_rx_data_comb;

logic       uart_rx_fifo_not_empty;
logic       uart_rx_fifo_not_full;
logic       uart_rx_fifo_read_strobe;
logic       uart_rx_fifo_write_strobe;
logic       uart_rx_fifo_overrun_flag;
logic [7:0] uart_rx_fifo_output_comb;

logic       uart_tx_strobe;
logic       uart_tx_ready;
logic       uart_rx_valid_irq_enable;
logic       uart_tx_ready_irq_enable;

logic [7:0] uart_reg_tx_data;
logic [7:0] uart_reg8_mux = '0;
logic       tx_write_active = 1'b0;
logic       rx_read_active = 1'b0;



assign bus.rd_data = {24'b0, uart_reg8_mux};

queue #(
    .WIDTH (8),
    .DEPTH (16)
) rx_fifo(
    .clk        (bus.clk),
    .rst_n      (bus.rst_n),
    .in_data    (uart_rx_data_comb),
    .in_valid   (uart_rx_fifo_write_strobe),
    .in_ready   (uart_rx_fifo_not_full),
    .out_data   (uart_rx_fifo_output_comb),
    .out_valid  (uart_rx_fifo_not_empty),
    .out_strobe (uart_rx_fifo_read_strobe)
);

uart uart_component(
    .clk      (bus.clk),
    .rst_n    (bus.rst_n),
    .tx_data  (uart_reg_tx_data),
    .tx_valid (uart_tx_strobe),
    .tx_ready (uart_tx_ready),
    .tx_out   (uart_tx_out),
    .rx_data  (uart_rx_data_comb),
    .rx_valid (uart_rx_valid_strobe),
    .rx_in    (uart_rx_in)
);

// Interrupt output. Enable bits are owned by the register-access process.
always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        uart_irq_out <= 1'b0;
    end else begin
        uart_irq_out <= (uart_rx_fifo_not_empty && uart_rx_valid_irq_enable)
                      | (uart_tx_ready && uart_tx_ready_irq_enable);
    end
end

// Register access and RX capture in one process so bus.error / IRQ enables
// have a single driver.
always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        uart_reg_tx_data <= '0;
        uart_tx_strobe   <= 1'b0;
        bus.wr_ack       <= 1'b0;
        tx_write_active  <= 1'b0;
        rx_read_active   <= 1'b0;
        uart_rx_valid_irq_enable <= 1'b0;
        uart_tx_ready_irq_enable <= 1'b0;
        uart_rx_fifo_read_strobe <= 1'b0;
        uart_rx_fifo_write_strobe <= 1'b0;
        uart_rx_fifo_overrun_flag <= 1'b0;
        uart_reg8_mux <= '0;
        bus.rd_valid  <= 1'b0;
        bus.error     <= 1'b0;
    end else begin
        uart_rx_fifo_read_strobe <= 1'b0;
        uart_rx_fifo_write_strobe <= 1'b0;

        uart_tx_strobe <= 1'b0;
        bus.wr_ack     <= 1'b0;
        bus.rd_valid   <= 1'b0;
        bus.error      <= 1'b0;

        if (!bus.valid) begin
            // TODO: could consolidate to one 'busy' signal? could use bus.wr_ack / bus.rd_valid?
            tx_write_active <= 1'b0;
            rx_read_active  <= 1'b0;
        end else if (!tx_write_active && bus.wr_strobe[0]) begin
            unique case (bus.addr[3:2])
                2'd1: begin
                    uart_rx_valid_irq_enable <= bus.wr_data[1];
                    uart_tx_ready_irq_enable <= bus.wr_data[0];
                    bus.wr_ack       <= 1'b1;
                    tx_write_active  <= 1'b1;
                end
                2'd2: begin
                    uart_reg_tx_data <= bus.wr_data[7:0];
                    uart_tx_strobe   <= 1'b1;
                    bus.wr_ack       <= 1'b1;
                    tx_write_active  <= 1'b1;
                end
                default: begin
                    bus.error <= 1'b1;
                end
            endcase
        end else if (!rx_read_active && ~|bus.wr_strobe) begin
            unique case (bus.addr[3:2])
                2'd0: begin
                    uart_reg8_mux <= {4'b0, uart_rx_fifo_overrun_flag, !uart_rx_fifo_not_full,
                                            uart_rx_fifo_not_empty, uart_tx_ready};
                    bus.rd_valid  <= 1'b1;
                    rx_read_active <= 1'b1;
                end
                2'd1: begin
                    uart_reg8_mux <= {6'b0, uart_rx_valid_irq_enable, uart_tx_ready_irq_enable};
                    bus.rd_valid  <= 1'b1;
                    rx_read_active <= 1'b1;
                end
                2'd2: begin
                    uart_reg8_mux <= uart_reg_tx_data;
                    bus.rd_valid  <= 1'b1;
                    rx_read_active <= 1'b1;
                end
                2'd3: begin
                    // TODO: consider using upper bits of the 32-bit read result to signal validity
                    uart_rx_fifo_read_strobe <= 1'b1;
                    uart_reg8_mux <= uart_rx_fifo_output_comb;
                    bus.rd_valid  <= 1'b1;
                    rx_read_active <= 1'b1;
                end
                default: begin
                    bus.error <= 1'b1;
                end
            endcase
        end

        // Same-cycle RX overwrite: keep after register access so a new byte wins.
        if (uart_rx_valid_strobe) begin
            if (uart_rx_fifo_not_full) begin
                uart_rx_fifo_write_strobe <= 1'b1;
            end else begin
                // TODO: add a way to clear the flag
                uart_rx_fifo_overrun_flag <= 1'b1;
            end
        end
    end
end

endmodule
