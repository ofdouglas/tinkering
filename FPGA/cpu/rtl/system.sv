import cpu_config_pkg::*;

module system(
    input  logic clk_in,
    output logic [7:0] led,
    output logic uart_tx_out,
    input logic uart_rx_in
);

logic rst_n;
logic clk;
logic clk_locked;
logic system_rst_n;
assign system_rst_n = rst_n && clk_locked;

reset_control reset_controller(
    .clk(clk),
    .rst_n(rst_n)
);

`ifdef VERILATOR
logic verilator_clk_div = 1'b0;
always_ff @(posedge clk_in) begin
    verilator_clk_div <= ~verilator_clk_div;
end
assign clk = verilator_clk_div;
assign clk_locked = 1'b1;
`else
clk_wiz_0 clk_wiz(
    .clk_out1(clk),
    .reset(1'b0),
    .locked(clk_locked),
    .clk_in1(clk_in)
);
`endif

// Instruction ROM for the CPU (private access)
bus_slave_interface #(.ADDR_MSB(MEMORY_ADDR_MSB)) rom_fetch();
// Alternate ROM access from main system bus, for loading data from ROM
bus_slave_read_port #(.ADDR_MSB(MEMORY_ADDR_MSB)) rom_alt_port();
block_rom #(
    .WORD_ADDR_BITS(MEMORY_ADDR_MSB-1),
    .ADDR_MSB(MEMORY_ADDR_MSB)
) cpu_rom (
    .bus_main(rom_fetch.slave),
    .bus_port2(rom_alt_port.slave)
);
logic [MEMORY_ADDR_MSB:2] rom_fetch_response_addr;
logic rom_fetch_response_matches;

always_ff @(posedge clk) begin
    if (!system_rst_n) begin
        rom_fetch_response_addr <= '0;
    end else begin
        rom_fetch_response_addr <= rom_fetch.addr;
    end
end
assign rom_fetch_response_matches = rom_fetch.rd_valid && (rom_fetch_response_addr == rom_fetch.addr);

// Data RAM for the CPU
bus_slave_interface #(.ADDR_MSB(MEMORY_ADDR_MSB)) sram_bus();
block_sram #(.WORD_ADDR_BITS(MEMORY_ADDR_MSB-1)) cpu_sram (
    .bus(sram_bus.slave)
);

// TODO: get rid of debug LEDs once we have better signaling for HW faults
logic [7:0] gpio_led_enables;
logic [3:0] debug_leds;
logic [63:0] mtime;
logic mti_irq;
logic mei_irq;
assign mei_irq = 1'b0;

// Peripheral bus
bus_slave_interface #(.ADDR_MSB(PERIPH_ADDR_MSB+2)) periph_bus();
system_peripherals #(.ADDR_MSB(PERIPH_ADDR_MSB+2)) peripherals (
    .bus(periph_bus.slave),
    .sys_gpio_led_enables(gpio_led_enables),
    .uart_tx_out(uart_tx_out),
    .uart_rx_in(uart_rx_in),
    .mti_irq(mti_irq),
    .mtime(mtime)
);
assign led = {debug_leds, gpio_led_enables[3:0]};


// System bus. address_bus[31:28] selects regions
logic [31:0] address_bus;
logic [31:2] cpu_word_address;
logic [31:0] data_read_bus;
logic [31:0] data_write_bus;
logic [ 3:0] byte_enables;
logic memory_access_valid;
logic memory_rd_valid;
logic memory_wr_ack;
logic memory_access_error;
logic cpu_request;
logic cpu_trap;
logic rom_trap;
logic periph_trap;
logic unaligned_write_trap;

logic is_write;
assign is_write = |byte_enables;
assign address_bus = {cpu_word_address, 2'b00};

always_comb begin
    logic aligned_write;
    case (address_bus[1:0])
        // 1, 2, or 4 bytes
        2'b00 : aligned_write = (byte_enables == 4'b0001) || (byte_enables == 4'b0011) || (byte_enables == 4'b1111);
        // 1 byte
        2'b01 : aligned_write = (byte_enables == 4'b0010);
        // 1 or 2 bytes
        2'b10 : aligned_write = (byte_enables == 4'b0100) || (byte_enables == 4'b1100);
        // 1 byte
        2'b11 : aligned_write = (byte_enables == 4'b1000);
        default : aligned_write = 0;
    endcase

    unaligned_write_trap = cpu_request && is_write && !aligned_write;
end

logic [3:0] region;
assign region = address_bus[REGION_BITS_MSB:REGION_BITS_LSB];

logic region_rom, region_ram, region_periph;
assign region_rom     = (region == REGION_BOOT_ROM);
assign region_ram     = (region == REGION_SHARED_RAM);
assign region_periph  = (region == REGION_PERIPH);

always_comb begin
    memory_access_valid = 1'b0;
    memory_rd_valid = 1'b0;
    memory_wr_ack = 1'b0;
    periph_trap = 1'b0;

    if (region_rom) begin
        memory_rd_valid = rom_alt_port.rd_valid;
    end else if (region_ram) begin
        memory_rd_valid = sram_bus.rd_valid;
        memory_wr_ack = sram_bus.wr_ack;
    end else if (region_periph) begin
        memory_rd_valid = periph_bus.rd_valid;
        memory_wr_ack = periph_bus.wr_ack;
        periph_trap = periph_bus.error;
    end
    memory_access_valid = is_write ? memory_wr_ack : memory_rd_valid;
end

// Read bus multiplexing
always_comb begin
    data_read_bus = '0;
    
    if (region_rom) begin
        data_read_bus = rom_alt_port.rd_data;
    end else if (region_ram) begin
        data_read_bus = sram_bus.rd_data;
    end else if (region_periph) begin
        data_read_bus = periph_bus.rd_data;
    end
end


assign rom_fetch.clk   = clk;
assign rom_fetch.rst_n = system_rst_n;
assign rom_fetch.valid = 1'b1;
assign rom_fetch.wr_strobe = '0;
assign rom_fetch.wr_data = '0;
assign rom_alt_port.valid = region_rom && cpu_request && !is_write;
assign rom_alt_port.addr = address_bus[MEMORY_ADDR_MSB:2];
assign rom_trap = rom_fetch.error || rom_alt_port.error;

assign sram_bus.clk   = clk;
assign sram_bus.rst_n = system_rst_n;
assign sram_bus.valid = region_ram && cpu_request;
assign sram_bus.wr_strobe = byte_enables;
assign sram_bus.wr_data = data_write_bus;
assign sram_bus.addr = address_bus[MEMORY_ADDR_MSB:2];

assign periph_bus.clk   = clk;
assign periph_bus.rst_n = system_rst_n;
assign periph_bus.valid = region_periph && cpu_request;
assign periph_bus.wr_strobe = byte_enables;
assign periph_bus.wr_data = data_write_bus;
assign periph_bus.addr = address_bus[PERIPH_ADDR_MSB+2:2];


assign memory_access_error = cpu_request &&
                             (unaligned_write_trap ||
                              (region_rom && is_write) ||
                              !(region_rom || region_ram || region_periph));
assign cpu_trap = memory_access_error || rom_trap;

// Latch HW faults
always_ff @(posedge clk) begin
    if (!system_rst_n) begin
        debug_leds <= '0;
    end else begin
        debug_leds[3] <= memory_access_error  ? 1'b1 : debug_leds[3];
        debug_leds[2] <= rom_trap             ? 1'b1 : debug_leds[2];
        debug_leds[1] <= periph_trap          ? 1'b1 : debug_leds[1];
        debug_leds[0] <= unaligned_write_trap ? 1'b1 : debug_leds[0];
    end
end

rv32cpu cpu(
    .clk               (clk),
    .rst_n             (system_rst_n),
    .instruction_fetch (rom_fetch.rd_data),
    .fetch_addr        (rom_fetch.addr),
    .fetch_valid       (rom_fetch_response_matches),
    .mti_irq           (mti_irq),
    .mei_irq           (mei_irq),
    .valid             (cpu_request),
    .wr_strobe         (byte_enables),
    .wr_data           (data_write_bus),
    .addr              (cpu_word_address),
    .rd_valid          (memory_rd_valid),
    .rd_data           (data_read_bus),
    .wr_ack            (memory_wr_ack),
    .error             (memory_access_error)
);

endmodule

