module gpio(
    bus_slave_interface.slave bus,

    output logic [7:0] led_driver_enables
    // TODO: switches, buttons, pmod GPIOs
);

logic [31:0] gpio_word_out = '0;
assign bus.rd_data = gpio_word_out;

logic [ 7:0] led_reg;
assign led_driver_enables = led_reg;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        bus.ready <= 1;
        bus.rd_valid <= 0;
        bus.wr_ack <= 0;
        bus.error <= 0;
        led_reg <= '0;
    end else begin
        bus.error <= 0;

        case (bus.addr)
        0 : begin // LED bits register
            if (bus.ready && bus.valid && |bus.wr_strobe) begin  // Write
                if (bus.wr_strobe[0]) begin
                    led_reg <= bus.wr_data[7:0];
                    bus.wr_ack <= 1;
                end else begin
                    bus.error <= 1;
                end
            end else begin
                bus.wr_ack <= 0;
            end

            if (bus.ready && bus.valid && (~|bus.wr_strobe)) begin  // Read
                gpio_word_out <= {24'b0, led_reg};
                bus.ready <= 0;
                bus.rd_valid <= 1;
            end else if (bus.rd_valid && !bus.valid) begin
                bus.ready <= 1;
                bus.rd_valid <= 0;
            end
        end

        // TODO: add other GPIO registers

        default :
            bus.error <= 1;
        endcase
    end
end

endmodule
