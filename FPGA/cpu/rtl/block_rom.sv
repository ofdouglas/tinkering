module block_rom #(
    parameter int WORD_ADDR_BITS = 16
) (
    bus_slave_interface.slave bus
);

localparam int NUM_WORDS = 2 ** WORD_ADDR_BITS;

(* rom_style = "block" *)
logic [31:0] rom_word_array [0 : NUM_WORDS-1];

logic [31:0] rom_word_out = '0;

assign bus.rd_data = rom_word_out;

always_ff @(posedge bus.clk) begin
    logic valid;
    valid = bus.valid && (~|bus.wr_strobe);

    if (!bus.rst_n) begin
        bus.rd_valid <= 1'b0;
        bus.error    <= 1'b0;
    end else if (valid) begin
        rom_word_out <= rom_word_array[bus.addr];
    end

    bus.rd_valid <= valid;
    bus.error    <= bus.valid && (|bus.wr_strobe);
end

initial begin
    string rom_path;
    int rom_fd;

    if (!$value$plusargs("FIRMWARE_HEX=%s", rom_path)) begin
        rom_path = "firmware.hex";
        rom_fd = $fopen(rom_path, "r");
        if (rom_fd == 0) begin
            rom_path = "mem/firmware.hex";
            rom_fd = $fopen(rom_path, "r");
        end
        if (rom_fd == 0) begin
            // xsim cwd is cpu.sim/sim_1/behav/xsim.
            rom_path = "../../../../../cpu/mem/firmware.hex";
            rom_fd = $fopen(rom_path, "r");
        end
    end else begin
        rom_fd = $fopen(rom_path, "r");
    end

    if (rom_fd == 0) begin
        $fatal(1, "block_rom: could not load firmware hex — run make in firmware/ and pass +FIRMWARE_HEX=<path> if needed");
    end
    $fclose(rom_fd);
    $readmemh(rom_path, rom_word_array);
    $display("block_rom: loaded %s, %0d words, [0]=0x%08x", rom_path, NUM_WORDS, rom_word_array[0]);
end

endmodule
