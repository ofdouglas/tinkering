// Dual-port Block ROM
module block_rom #(
    parameter int WORD_ADDR_BITS = 11,
    parameter int ADDR_MSB = WORD_ADDR_BITS + 1
) (
    bus_slave_interface.slave bus_main,
    bus_slave_read_port.slave bus_port2
);

localparam int NUM_WORDS = 2 ** WORD_ADDR_BITS;

(* rom_style = "block" *)
logic [31:0] rom_word_array [0 : NUM_WORDS-1];

logic [WORD_ADDR_BITS-1:0] main_word_addr;
logic [WORD_ADDR_BITS-1:0] port2_word_addr;
logic write_error, main_word_addr_error, port2_word_addr_error;

assign main_word_addr        = bus_main.addr[WORD_ADDR_BITS+1:2];
assign port2_word_addr       = bus_port2.addr[WORD_ADDR_BITS+1:2];
assign write_error           = bus_main.valid && (|bus_main.wr_strobe);

generate
    if (ADDR_MSB > WORD_ADDR_BITS + 1) begin : gen_main_addr_error
        assign main_word_addr_error = bus_main.valid &&
                                      (|bus_main.addr[ADDR_MSB:WORD_ADDR_BITS+2]);
    end else begin : gen_no_main_addr_error
        assign main_word_addr_error = 1'b0;
    end

    if (ADDR_MSB > WORD_ADDR_BITS + 1) begin : gen_port2_addr_error
        assign port2_word_addr_error = bus_port2.valid &&
                                       (|bus_port2.addr[ADDR_MSB:WORD_ADDR_BITS+2]);
    end else begin : gen_no_port2_addr_error
        assign port2_word_addr_error = 1'b0;
    end
endgenerate

always_ff @(posedge bus_main.clk) begin
    if (!bus_main.rst_n) begin
        bus_main.rd_valid  <= 1'b0;
        bus_main.error     <= 1'b0;
        bus_main.wr_ack    <= 1'b0;
        bus_main.rd_data   <= '0;
        bus_port2.rd_valid <= 1'b0;
        bus_port2.error    <= 1'b0;
        bus_port2.rd_data  <= '0;
    end else begin
        bus_main.rd_valid  <= bus_main.valid && (~|bus_main.wr_strobe);
        bus_main.error     <= write_error || main_word_addr_error;
        bus_main.wr_ack    <= 1'b0;
        bus_main.rd_data   <= rom_word_array[main_word_addr];
        bus_port2.rd_data  <= rom_word_array[port2_word_addr];
        bus_port2.rd_valid <= bus_port2.valid && (~port2_word_addr_error);
        bus_port2.error    <= port2_word_addr_error;
    end
end

`ifdef SYNTHESIS
initial begin
    $readmemh("firmware.hex", rom_word_array);
end
`else
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
        $fatal(1, "block_rom: could not load firmware hex - run make in firmware/ and pass +FIRMWARE_HEX=<path> if needed");
    end
    $fclose(rom_fd);
    $readmemh(rom_path, rom_word_array);
    $display("block_rom: loaded %s, %0d words, [0]=0x%08x", rom_path, NUM_WORDS, rom_word_array[0]);
end
`endif

endmodule
