module block_sram (
    bus_slave_interface.slave bus
);

localparam int WORD_ADDR_BITS = $bits(bus.addr);
localparam int NUM_WORDS = 2 ** WORD_ADDR_BITS;

(* ram_style = "block" *)
logic [31:0] sram_word_array [0 : NUM_WORDS-1];

logic [31:0] sram_word_out = '0;
assign bus.rd_data = sram_word_out;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        bus.ready <= 1;
        bus.rd_valid <= 0;
        bus.wr_ack <= 0;
        bus.error <= 0;
    end else begin
        // Write to SRAM
        if (bus.ready && bus.valid && (|bus.wr_strobe)) begin
            bus.wr_ack <= 1;
            for (int i = 0; i < 4; i++) begin
                if (bus.wr_strobe[i]) begin
                    sram_word_array[bus.addr][8*i +: 8] <= bus.wr_data[8*i +: 8];
                end
            end
        end else begin
            bus.wr_ack <= 0;
        end
        
        // Read from SRAM
        if (bus.ready && bus.valid && (~|bus.wr_strobe)) begin
            bus.ready <= 0;
            bus.rd_valid <= 1;
            sram_word_out <= sram_word_array[bus.addr];
        end else if (bus.rd_valid && !bus.valid) begin
            bus.ready <= 1;
            bus.rd_valid <= 0;
        end
    end
end

endmodule