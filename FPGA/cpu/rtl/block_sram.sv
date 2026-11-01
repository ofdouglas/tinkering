module block_sram #(
    parameter int WORD_ADDR_BITS = 11
) (
    bus_slave_interface.slave bus_main,
    bus_slave_read_port.slave bus_port2
);

localparam int NUM_WORDS = 2 ** WORD_ADDR_BITS;

(* ram_style = "block" *)
logic [31:0] sram_word_array [0 : NUM_WORDS-1];

logic [31:0] main_output   = '0;
logic [31:0] second_output = '0;

assign bus_main.rd_data  = main_output;
assign bus_port2.rd_data = second_output;

always_ff @(posedge bus_main.clk) begin
    if (!bus_main.rst_n) begin
        bus_main.rd_valid  <= 1'b0;
        bus_main.wr_ack    <= 1'b0;
        bus_main.error     <= 1'b0;
        bus_port2.rd_valid <= 1'b0;
        bus_port2.error    <= 1'b0;
    end else begin
        bus_main.rd_valid <= bus_main.valid && (~|bus_main.wr_strobe);
        bus_main.wr_ack   <= bus_main.valid && (|bus_main.wr_strobe);
        bus_main.error    <= 1'b0;

        if (bus_main.valid && (|bus_main.wr_strobe)) begin
            for (int i = 0; i < 4; i++) begin
                if (bus_main.wr_strobe[i]) begin
                    sram_word_array[bus_main.addr][8*i +: 8] <= bus_main.wr_data[8*i +: 8];
                end
            end
        end

        if (bus_main.valid && (~|bus_main.wr_strobe)) begin
            main_output <= sram_word_array[bus_main.addr];
        end

        bus_port2.rd_valid <= bus_port2.valid;
        bus_port2.error    <= 1'b0;

        if (bus_port2.valid) begin
            second_output <= sram_word_array[bus_port2.addr];
        end
    end
end

endmodule
