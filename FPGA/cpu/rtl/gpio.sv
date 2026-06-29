module gpio(
    bus_slave_interface.slave bus,
    output logic [7:0] led_driver_enables
    // TODO: switches, buttons, pmod GPIOs
);

logic [31:0] gpio_word_out = '0;
logic          idle = 1'b1;

assign bus.rd_data = gpio_word_out;

logic [7:0] led_reg;
assign led_driver_enables = led_reg;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        idle         <= 1'b1;
        bus.rd_valid <= 1'b0;
        bus.wr_ack   <= 1'b0;
        bus.error    <= 1'b0;
        led_reg      <= '0;
    end else begin
        bus.error  <= 1'b0;
        bus.wr_ack <= 1'b0;

        unique case (bus.addr)
            0: begin // LED bits register
                if (idle && bus.valid && |bus.wr_strobe) begin
                    if (bus.wr_strobe[0]) begin
                        led_reg    <= bus.wr_data[7:0];
                        bus.wr_ack <= 1'b1;
                    end else begin
                        bus.error <= 1'b1;
                    end
                end

                if (idle && bus.valid && (~|bus.wr_strobe)) begin
                    gpio_word_out <= {24'b0, led_reg};
                    idle          <= 1'b0;
                    bus.rd_valid  <= 1'b1;
                end else if (bus.rd_valid && !bus.valid) begin
                    idle         <= 1'b1;
                    bus.rd_valid <= 1'b0;
                end
            end

            // TODO: add other GPIO registers

            default:
                bus.error <= bus.valid;
        endcase
    end
end

endmodule
