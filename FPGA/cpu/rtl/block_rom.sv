module block_rom (
    bus_slave_interface.slave bus
);

localparam int WORD_ADDR_BITS = $bits(bus.addr);
localparam int NUM_WORDS = 2 ** WORD_ADDR_BITS;

(* rom_style = "block" *)
logic [31:0] rom_word_array [0 : NUM_WORDS-1];

logic [31:0] rom_word_out = '0;
assign bus.rd_data = rom_word_out;

always_ff @(posedge bus.clk) begin
    if (!bus.rst_n) begin
        bus.ready <= 1;
        bus.rd_valid <= 0;
        bus.error <= 0;
    end else if (bus.ready && bus.valid && (~|bus.wr_strobe)) begin
        bus.ready <= 0;
        bus.rd_valid <= 1;
        rom_word_out <= rom_word_array[bus.addr];
    end else if (bus.rd_valid && !bus.valid) begin
        bus.ready <= 1;
        bus.rd_valid <= 0;
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