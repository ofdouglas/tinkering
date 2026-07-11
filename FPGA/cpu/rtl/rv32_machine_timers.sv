module rv32_machine_timers (
    bus_slave_interface.slave bus,
    output logic [31:0] timer_low_value,
    output logic [31:0] timer_high_value,
    output logic mti_irq
);

////////////////////////////////////////////////////////////
// Register Definitions
////////////////////////////////////////////////////////////
// Time Low:   0
// Time High:  4
// Comp Low:   8
// Comp High: 12
localparam int TIMER_LOW_REG_ADDR  = 0;
localparam int TIMER_HIGH_REG_ADDR = 4;
localparam int COMP_LOW_REG_ADDR   = 8;
localparam int COMP_HIGH_REG_ADDR  = 12;
////////////////////////////////////////////////////////////

// Omit the top 8 bits to optimize resources, since they will never be used
logic [55:0] timer_value = '0;
logic [55:0] comp_value = '1;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        timer_low_value <= 0;
        timer_high_value <= 0;
        mti_irq <= 0;
    end else begin
        // Handle writes to the compare registers
        logic is_write = bus.valid && |bus.wr_strobe;
        logic is_word_write = is_write && (bus.wr_strobe == 4'b1111);

        if (is_word_write) begin
            case (bus.addr)
                COMP_LOW_REG_ADDR:  comp_value[31:0] <= bus.wr_data;
                COMP_HIGH_REG_ADDR: comp_value[55:32] <= bus.wr_data[23:0];
                default:            bus.error <= 1'b1;
            endcase
        end else if (is_write) begin
            bus.error <= 1'b1;
        end else begin
            bus.error <= 1'b0;
        end

        // Handle reads from the time registers
        case (bus.addr)
            TIMER_LOW_REG_ADDR:  bus.rd_data <= timer_value[31:0];
            TIMER_HIGH_REG_ADDR: bus.rd_data <= {8'b0, timer_value[55:32]};
            COMP_LOW_REG_ADDR:   bus.rd_data <= comp_value[31:0];
            COMP_HIGH_REG_ADDR:  bus.rd_data <= {8'b0, comp_value[55:32]};
            default:             bus.rd_data <= 'X;
        endcase
        bus.rd_valid <= bus.valid && (~|bus.wr_strobe);

        // Update the timer and interrupt status
        mti_irq <= (timer_value >= comp_value);
        timer_value <= timer_value + 1;
    end
end

endmodule