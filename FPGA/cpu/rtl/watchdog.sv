/* A simple watchdog timer peripheral with a built-in prescaler.
 */
module watchdog_peripheral(
    bus_slave_interface.slave bus,
    output logic reset_request
);

/* Watchdog register map (word addresses). Registers are {MSB ... LSB}
    base [0] = Control Reg: {30'bx | 1'b reset_module | 1'b enable_counting}    R/W
    base [1] = Config Reg:  {16'b watchdog_max | 16'b prescaler_max}            R/W
    base [2] = Service Reg: {16'bx | 16'b service_key}                          Write-only (reads as 0)

    `enable_counting` cannot be cleared by SW. Write the stop sequence to the Service reg to 
    stop counting. Writing 1 to `reset_module` will reset all module registers only if `enable_counting` 
    is cleared; otherwise it has no effect. Writes to the config register while counting is enabled
    will have no effect.

    Writing an undefined value to the Service Reg will trigger a system reset. Only the 4 defined key 
    values may be written. Writing the key values in an incorrect sequence will not cause an immediate
    reset, but the desired action (watchdog service or counter stop) will not be taken.

    Example watchdog usage:
        // Watchdog init routine:
        watchdog.config  = 0x0BB7'00FF;  // WDG interval of 3,000 counts; f_count = f_clk / 256.
        watchdog.control = 1 << 0;       // enable_counting

        ...
        // Watchdog kick routine:
        watchdog.service = 0xA703;
        watchdog.service = 0xF8FC;

        // Watchdog stop routine:
        watchdog.service = 0x6AED;
        watchdog.service = 0x9106;

        // (Optional) reset all regs in order to reconfigure the watchdog
        // (for a bootloader-to-app handoff, for example)
        watchdog.control = 1 << 1;          // reset_module
        watchdog.config  = kWdgAppConfig;   // Apply new configuration
        watchdog.control = 1 << 0;          // enable_counting
 */

// To service the watchdog, these values must be written to the Service Reg in sequence.
localparam logic [15:0] kServiceKey1 = 16'hA703;
localparam logic [15:0] kServiceKey2 = 16'hF8FC;

// To stop the watchdog, these values must be written to the Service Reg in sequence.
localparam logic [15:0] kServiceStopKey1 = 16'h6AED;
localparam logic [15:0] kServiceStopKey2 = 16'h9106;

/******************************************************************************
 * Registers
 *****************************************************************************/

// Tracks if the latest write to the key reg was kServiceKey1.
logic key1_matched;

// Tracks if the latest write to the key reg was kServiceStopKey1.
logic stop_key1_matched;

// Counter registers must only be modified while this bit is cleared.
logic counting_enabled;

// Sticky / set-only flag which requests a system reset.
logic system_reset_flag;

// Dynamic flag which requests a reset of the module registers.
logic module_reset_flag;

// Strobe indicating the watchdog_counter will be reset to 0.
logic service_complete;

// Counts from 0 to max, then resets to 0 and increments watchdog_counter by 1.
logic [15:0] prescaler_counter;
logic [15:0] prescaler_max;

// Counts from 0 to max. If it reaches max, a reset_request is issued. Service
// resets it to 0.
logic [15:0] watchdog_counter;
logic [15:0] watchdog_max;


/******************************************************************************
 * Logic
 *****************************************************************************/

assign reset_request = system_reset_flag;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n || module_reset_flag) begin
        if (!bus.rst_n) begin
            system_reset_flag <= 1'b0;
        end
        key1_matched      <= 1'b0;
        stop_key1_matched <= 1'b0;
        counting_enabled  <= 1'b0;
        module_reset_flag <= 1'b0;
        service_complete  <= 1'b0;
        prescaler_counter <= '0;
        prescaler_max     <= '0;
        watchdog_counter  <= '0;
        watchdog_max      <= '0;
        bus.wr_ack        <= 1'b0;
        bus.rd_valid      <= 1'b0;
        bus.rd_data       <= '0;
        bus.error         <= 1'b0;
    end else begin
        bus.wr_ack    <= 1'b0;
        bus.rd_valid  <= 1'b0;
        bus.error     <= 1'b0;

        if (counting_enabled) begin
            if (service_complete) begin
                service_complete  <= 1'b0;
                watchdog_counter  <= '0;
                prescaler_counter <= '0;
            end else if (prescaler_counter == prescaler_max) begin
                prescaler_counter <= '0;
                if (watchdog_counter == watchdog_max) begin
                    system_reset_flag <= 1'b1;
                end else begin
                    watchdog_counter  <= watchdog_counter + 1'b1;
                end
            end else begin
                prescaler_counter <= prescaler_counter + 1'b1;
            end
        end

        if (bus.valid && |bus.wr_strobe) begin
            unique case (bus.addr[3:2])
                2'd0: begin
                    bus.wr_ack        <= 1'b1;
                    if (bus.wr_strobe[0]) begin
                        counting_enabled  <= counting_enabled  | bus.wr_data[0];
                        module_reset_flag <= module_reset_flag | (bus.wr_data[1] && !counting_enabled);
                    end
                end
                2'd1: begin
                    bus.wr_ack        <= 1'b1;
                    if (!counting_enabled) begin
                        watchdog_max  <= &bus.wr_strobe[3:2] ? bus.wr_data[31:16] : watchdog_max;
                        prescaler_max <= &bus.wr_strobe[1:0] ? bus.wr_data[15:0]  : prescaler_max;
                    end
                end
                2'd2: begin
                    bus.wr_ack        <= 1'b1;
                    key1_matched      <= 1'b0;
                    stop_key1_matched <= 1'b0;
                    if (&bus.wr_strobe[1:0]) begin
                        unique case (bus.wr_data[15:0])
                            // kServiceKey1 followed by kServiceKey2 services the watchdog.
                            kServiceKey1: begin
                                key1_matched      <= 1'b1;
                            end
                            kServiceKey2: begin
                                service_complete  <= key1_matched;
                            end
                            // kServiceStopKey1 followed by kServiceStopKey2 pauses counting.
                            kServiceStopKey1: begin
                                stop_key1_matched <= 1'b1;
                            end
                            kServiceStopKey2: begin
                                counting_enabled  <= stop_key1_matched ? 1'b0 : counting_enabled;
                            end
                            // Writing an undefined key value triggers a system reset.
                            default: begin
                                system_reset_flag <= 1'b1;
                            end
                        endcase
                    end
                end
                default: begin
                    bus.error <= 1'b1;
                end
            endcase
        end else if (bus.valid && ~|bus.wr_strobe) begin
            unique case (bus.addr[3:2])
                2'd0: begin
                    bus.rd_data  <= {31'b0, counting_enabled};
                    bus.rd_valid <= 1'b1;
                end
                2'd1: begin
                    bus.rd_data  <= {watchdog_max, prescaler_max};
                    bus.rd_valid <= 1'b1;
                end
                2'd2: begin
                    bus.rd_data  <= '0;
                    bus.rd_valid <= 1'b1;
                end
                default: begin
                    bus.error <= 1'b1;
                end
            endcase
        end
    end
end

endmodule
