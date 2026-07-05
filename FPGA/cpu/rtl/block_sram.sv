module block_sram #(
    parameter int WORD_ADDR_BITS = 10
) (
    bus_slave_interface.slave bus
);

localparam int NUM_WORDS = 2 ** WORD_ADDR_BITS;

(* ram_style = "block" *)
logic [31:0] sram_word_array [0 : NUM_WORDS-1];

logic [31:0] sram_word_out = '0;

assign bus.rd_data = sram_word_out;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        bus.rd_valid <= 1'b0;
        bus.wr_ack   <= 1'b0;
        bus.error    <= 1'b0;
    end else begin
        bus.rd_valid <= bus.valid && (~|bus.wr_strobe);
        bus.wr_ack   <= bus.valid && (|bus.wr_strobe);
        bus.error    <= 1'b0;

        if (bus.valid && (|bus.wr_strobe)) begin
            for (int i = 0; i < 4; i++) begin
                if (bus.wr_strobe[i]) begin
                    sram_word_array[bus.addr][8*i +: 8] <= bus.wr_data[8*i +: 8];
                end
            end
        end

        if (bus.valid && (~|bus.wr_strobe)) begin
            sram_word_out <= sram_word_array[bus.addr];
        end
    end
end

endmodule
