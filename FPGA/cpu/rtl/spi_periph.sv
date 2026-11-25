module spi_periph (
    bus_slave_interface.slave bus,

    // output logic spi_irq_out,

    // SPI bus
    output logic sck,
    output logic cs_n,
    output logic mosi, 
    input  logic miso
);

/* SPI register map (word addresses). Registers are {MSB ... LSB}
    base [0] = Control Reg: {30'bx | 1'b reset_module | 1'b enable_tx_rx}       R/W
    base [1] = Config Reg:  {16'b placeholder_max | 16'b placeholder_max}       RESERVED; DO NOT ACCESS
    base [2] = Status Reg:  {29'bx | 1'b cs_active | 1'b rx_dr_valid | 1'b tx_dr_ready }        W1C/RO
    base [3] = RX Data Reg: {32'b receive_data, zero-padded}                    RO
    base [4] = TX Data Reg: {32'b/16'b/8'b transmit_data}                       Write-only (reads as 0)  
*/

// SPI peripheral interface
logic [7:0] data_in;
logic       valid_in;
logic       in_ready;
logic [7:0] data_out;
logic       valid_out;

// Internal Registers
logic [31:0] rx_buffer, tx_buffer;
logic [2:0]  bytes_to_send;
logic tx_rx_enabled_reg;
logic rx_dr_pending_reg;
logic cs_active_reg;
logic config_placeholder_reg;
logic reset_module_reg;
logic register_access;
logic tx_dr_ready;

assign data_in = tx_buffer[7:0];
assign tx_dr_ready = tx_rx_enabled_reg && in_ready && (bytes_to_send == '0);
assign cs_n = !cs_active_reg;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n || reset_module_reg) begin
        rx_buffer         <= '0;
        tx_buffer         <= '0;
        bytes_to_send     <= '0;
        tx_rx_enabled_reg <= 1'b0;
        rx_dr_pending_reg <= 1'b0;
        cs_active_reg     <= 1'b0;
        config_placeholder_reg <= 1'b0;
        reset_module_reg  <= 1'b0;
        register_access   <= 1'b0;
        bus.rd_data       <= '0;
        bus.rd_valid      <= 1'b0;
        bus.wr_ack        <= 1'b0;
        bus.error         <= 1'b0;
        valid_in          <= 1'b0;
    end else begin
        bus.rd_data    <= '0;
        bus.rd_valid   <= 1'b0;
        bus.wr_ack     <= 1'b0;
        bus.error      <= 1'b0;
        valid_in       <= 1'b0;

        if (!bus.valid) begin
            register_access <= 1'b0;
        end

        if (tx_rx_enabled_reg && (bytes_to_send != '0)) begin
            if (valid_out) begin
                rx_buffer     <= {rx_buffer[31:8], data_out};
                bytes_to_send <= bytes_to_send - 1'b1;
                if (bytes_to_send == 3'd1) begin
                    rx_dr_pending_reg <= 1'b1;
                end
            end else if (in_ready) begin
                valid_in      <= 1'b1;
            end
        end

        if (bus.valid && !register_access) begin
            register_access <= 1'b1;

            if (|bus.wr_strobe) begin
                bus.wr_ack <= 1'b1;
                unique case (bus.addr[4:2])
                    3'd0: begin // CONTROL reg
                        if (bus.wr_strobe[0]) begin
                            {reset_module_reg, tx_rx_enabled_reg} <= bus.wr_data[1:0];
                        end
                    end
                    3'd1: begin // CONFIG reg
                        if (bus.wr_strobe[0]) begin
                            config_placeholder_reg <= bus.wr_data[0];
                        end
                    end
                    3'd2: begin // STATUS reg
                        if (bus.wr_strobe[0] && bus.wr_data[2]) begin
                            cs_active_reg <= 1'b0;
                        end
                    end
                    3'd3: ; // RX Data reg   (No-op)
                    3'd4: begin // TX Data reg
                        if (tx_dr_ready) begin
                            tx_buffer <= bus.wr_data;
                            case (bus.wr_strobe)
                                4'b0001: begin
                                    bytes_to_send <= 3'd1;
                                    cs_active_reg <= 1'b1;
                                end
                                4'b0011: begin
                                    bytes_to_send <= 3'd2;
                                    cs_active_reg <= 1'b1;
                                end
                                4'b1111: begin
                                    bytes_to_send <= 3'd4;
                                    cs_active_reg <= 1'b1;
                                end
                                default: begin
                                    bus.error <= 1'b1;
                                end
                            endcase
                        end
                    end
                    default: begin
                        bus.error <= 1'b1;
                    end
                endcase
            end else begin
                bus.rd_valid   <= 1'b1;
                unique case (bus.addr[4:2])
                    3'd0: begin // CONTROL reg
                        bus.rd_data    <= {31'b0, tx_rx_enabled_reg};
                    end
                    3'd1: begin // CONFIG reg
                        bus.rd_data    <= '0;
                    end
                    3'd2: begin // STATUS reg
                        bus.rd_data    <= {29'b0, cs_active_reg, rx_dr_pending_reg, tx_dr_ready};
                    end
                    3'd3: begin // RX Data reg
                        bus.rd_data       <= rx_buffer;
                        rx_dr_pending_reg <= '0;
                    end
                    3'd4: begin // TX Data reg
                        bus.rd_data       <= '0;
                    end
                    default: begin
                        bus.error <= 1'b1;
                        bus.rd_valid   <= 1'b0;
                    end
                endcase
            end
        end
    end
end

spi_master spi_master_inst (
        .clk(bus.clk),
        .rst_n(bus.rst_n),
        .data_in(data_in),
        .valid_in(valid_in),
        .in_ready(in_ready),
        .data_out(data_out),
        .valid_out(valid_out),
        .sck(sck),
        .mosi(mosi),
        .miso(miso)
);

endmodule
