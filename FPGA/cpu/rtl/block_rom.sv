module block_rom (
    bus_slave_interface.slave bus
);

localparam int WORD_ADDR_BITS = $bits(bus.addr);
localparam int NUM_WORDS = 2 ** WORD_ADDR_BITS;

(* rom_style = "block" *)
logic [31:0] rom_word_array [0 : NUM_WORDS-1];

logic [31:0] rom_word_out = '0;
logic          idle = 1'b1;

assign bus.rd_data = rom_word_out;
assign bus.wr_ack  = 1'b0;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        idle         <= 1'b1;
        bus.rd_valid <= 1'b0;
        bus.error    <= 1'b0;
    end else if (idle && bus.valid && (~|bus.wr_strobe)) begin
        idle         <= 1'b0;
        bus.rd_valid <= 1'b1;
        rom_word_out <= rom_word_array[bus.addr];
    end else if (bus.rd_valid && !bus.valid) begin
        idle         <= 1'b1;
        bus.rd_valid <= 1'b0;
    end

    bus.error <= bus.valid && (|bus.wr_strobe);
end

initial begin
    // xsim cwd is cpu.sim/sim_1/behav/xsim — firmware.hex must be copied there (see scripts/sim.tcl)
    $readmemh("firmware.hex", rom_word_array);
    if (rom_word_array[0] === 'x) begin
        $readmemh("../../../../../cpu/mem/firmware.hex", rom_word_array);
    end
    if (rom_word_array[0] === 'x) begin
        $fatal(1, "block_rom: could not load firmware.hex — run make in firmware/ and copy hex for sim");
    end else begin
        $display("block_rom: loaded %0d words, [0]=0x%08x", NUM_WORDS, rom_word_array[0]);
    end
end

endmodule
