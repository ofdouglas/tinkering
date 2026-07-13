module rv32_machine_timers (
    bus_slave_interface.slave bus,
    output logic [63:0] mtime,
    output logic mti_irq
);

////////////////////////////////////////////////////////////
// Register Definitions
////////////////////////////////////////////////////////////
localparam int TIMER_LOW_REG_ADDR  = 0; // Word address
localparam int TIMER_HIGH_REG_ADDR = 1;
localparam int COMP_LOW_REG_ADDR   = 2;
localparam int COMP_HIGH_REG_ADDR  = 3;
////////////////////////////////////////////////////////////

// Omit the top 8 bits to optimize resources, since they will never be used
logic [55:0] timer_value, comp_value;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        timer_value <= '0;
        comp_value  <= '1;
        mti_irq     <= 0;
        mtime       <= '0;
        bus.error   <= 1'b0;
        bus.wr_ack  <= 1'b0;
        bus.rd_valid <= 1'b0;
        bus.rd_data <= '0;
    end else begin
        // Handle writes to the compare registers
        logic is_write = bus.valid && |bus.wr_strobe;
        logic is_word_write = is_write && (bus.wr_strobe == 4'b1111);
        logic is_read = bus.valid && (~|bus.wr_strobe);

        bus.error  <= 1'b0;
        bus.wr_ack <= 1'b0;
        bus.rd_valid <= 1'b0;
        bus.rd_data <= '0;

        if (is_word_write) begin
            case (bus.addr)
                COMP_LOW_REG_ADDR:  begin
                    comp_value[31:0] <= bus.wr_data;
                    bus.wr_ack <= 1'b1;
                end
                COMP_HIGH_REG_ADDR: begin
                    comp_value[55:32] <= bus.wr_data[23:0];
                    bus.wr_ack <= 1'b1;
                end
                default:            begin
                    bus.error <= 1'b1;
                end
            endcase
        end else if (is_write) begin
            bus.error <= 1'b1;
        end else if (is_read) begin
            // Handle reads from the time registers
            case (bus.addr)
                TIMER_LOW_REG_ADDR: begin
                    bus.rd_data <= timer_value[31:0];
                    bus.rd_valid <= 1'b1;
                end
                TIMER_HIGH_REG_ADDR: begin
                    bus.rd_data <= {8'b0, timer_value[55:32]};
                    bus.rd_valid <= 1'b1;
                end
                COMP_LOW_REG_ADDR:   begin
                    bus.rd_data <= comp_value[31:0];
                    bus.rd_valid <= 1'b1;
                end
                COMP_HIGH_REG_ADDR:  begin
                    bus.rd_data <= {8'b0, comp_value[55:32]};
                    bus.rd_valid <= 1'b1;
                end
                default:             begin
                    bus.error <= 1'b1;
                end
            endcase
        end
        

        // Update the timer and interrupt status
        mti_irq <= (timer_value >= comp_value);
        timer_value <= timer_value + 1;
        mtime       <= {8'h00, timer_value};
    end
end

endmodule