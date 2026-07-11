import cpu_config_pkg::*;

module system(
    input  logic clk,
    output logic [7:0] led,
    output logic uart_tx_out,
    input logic uart_rx_in
);

logic rst_n;
reset_control reset_controller(
    .clk(clk),
    .rst_n(rst_n)
);

// Instruction ROM for the CPU
bus_slave_interface #(.ADDR_MSB(MEMORY_ADDR_MSB)) rom_bus();
block_rom cpu_rom (
    .bus(rom_bus.slave)
);

localparam int ROM_WORD_ADDR_BITS = MEMORY_ADDR_MSB - 1;
localparam int ROM_WORDS = 2 ** ROM_WORD_ADDR_BITS;
localparam logic [31:0] NOP_INSTRUCTION = 32'h0000_0013;

logic [31:0] instruction_rom [0:ROM_WORDS-1];
logic [31:0] instruction_fetch;
logic [31:2] fetch_addr;
logic fetch_valid;
logic fetch_in_range;
logic [ROM_WORD_ADDR_BITS-1:0] fetch_word_addr;

assign fetch_word_addr = fetch_addr[MEMORY_ADDR_MSB:2];
assign fetch_in_range = (fetch_addr[31:MEMORY_ADDR_MSB+1] == '0);
assign instruction_fetch = fetch_in_range ? instruction_rom[fetch_word_addr] : NOP_INSTRUCTION;
assign fetch_valid = rst_n && fetch_in_range;

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
            rom_path = "../../../../../cpu/mem/firmware.hex";
            rom_fd = $fopen(rom_path, "r");
        end
    end else begin
        rom_fd = $fopen(rom_path, "r");
    end

    if (rom_fd == 0) begin
        $fatal(1, "system: could not load firmware hex; run make in firmware/ and pass +FIRMWARE_HEX=<path> if needed");
    end
    $fclose(rom_fd);
    $readmemh(rom_path, instruction_rom);
end

// Data RAM for the CPU
bus_slave_interface #(.ADDR_MSB(MEMORY_ADDR_MSB)) sram_bus();
block_sram #(.WORD_ADDR_BITS(MEMORY_ADDR_MSB-1)) cpu_sram (
    .bus(sram_bus.slave)
);

// GPIO (LED control)
bus_slave_interface #(.ADDR_MSB(PERIPH_ADDR_MSB)) gpio_bus();
logic [7:0] sys_gpio_led_enables;
gpio system_gpio (
    .bus(gpio_bus.slave),
    .led_driver_enables(sys_gpio_led_enables)
);
assign led[7:0] = {cpu_trap, unaligned_write_trap, rom_trap, 1'b0, sys_gpio_led_enables[3:0]};

// Debug UART
bus_slave_interface #(.ADDR_MSB(PERIPH_ADDR_MSB)) uart_bus();
uart_periph uart0 (
    .bus(uart_bus.slave),
    .uart_tx_out(uart_tx_out),
    .uart_rx_in(uart_rx_in)
);

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

logic region_rom, region_ram, region_led, region_uart;
assign region_rom  = (region == REGION_BOOT_ROM);
assign region_ram  = (region == REGION_SHARED_RAM);
assign region_led  = (region == REGION_PERIPH) && (address_bus[PERIPH_ADDR_MSB+1] == 1'b0);
assign region_uart = (region == REGION_PERIPH) && (address_bus[PERIPH_ADDR_MSB+1] == 1'b1);

always_comb begin
    memory_access_valid = 1'b0;
    memory_rd_valid = 1'b0;
    memory_wr_ack = 1'b0;
    if (region_rom) begin
        memory_rd_valid = rom_bus.rd_valid;
    end else if (region_ram) begin
        memory_rd_valid = sram_bus.rd_valid;
        memory_wr_ack = sram_bus.wr_ack;
    end else if (region_led) begin
        memory_rd_valid = gpio_bus.rd_valid;
        memory_wr_ack = gpio_bus.wr_ack;
    end else if (region_uart) begin
        memory_rd_valid = uart_bus.rd_valid;
        memory_wr_ack = uart_bus.wr_ack;
    end
    memory_access_valid = is_write ? memory_wr_ack : memory_rd_valid;
end

// Read bus multiplexing
always_comb begin
    data_read_bus = '0;
    if (region_rom) begin
        data_read_bus = rom_bus.rd_data;
    end else if (region_ram) begin
        data_read_bus = sram_bus.rd_data;
    end else if (region_led) begin
        data_read_bus = gpio_bus.rd_data;
    end else if (region_uart) begin
        data_read_bus = uart_bus.rd_data;
    end
end


assign rom_bus.clk   = clk;
assign rom_bus.rst_n = rst_n;
assign rom_bus.valid = region_rom && cpu_request && !is_write;
assign rom_bus.wr_strobe = '0;
assign rom_bus.wr_data = '0;
assign rom_bus.addr = address_bus[MEMORY_ADDR_MSB:2];
assign rom_trap = rom_bus.error;

assign sram_bus.clk   = clk;
assign sram_bus.rst_n = rst_n;
assign sram_bus.valid = region_ram && cpu_request;
assign sram_bus.wr_strobe = byte_enables;
assign sram_bus.wr_data = data_write_bus;
assign sram_bus.addr = address_bus[MEMORY_ADDR_MSB:2];

assign gpio_bus.clk   = clk;
assign gpio_bus.rst_n = rst_n;
assign gpio_bus.valid = region_led && cpu_request;
assign gpio_bus.wr_strobe = byte_enables;
assign gpio_bus.wr_data = data_write_bus;
assign gpio_bus.addr = address_bus[PERIPH_ADDR_MSB:2];

assign uart_bus.clk   = clk;
assign uart_bus.rst_n = rst_n;
assign uart_bus.valid = region_uart && cpu_request;
assign uart_bus.wr_strobe = byte_enables;
assign uart_bus.wr_data = data_write_bus;
assign uart_bus.addr = address_bus[PERIPH_ADDR_MSB:2];

assign memory_access_error = cpu_request &&
                             (unaligned_write_trap ||
                              (region_rom && is_write) ||
                              !(region_rom || region_ram || region_led || region_uart));
assign cpu_trap = memory_access_error || rom_trap;

rv32cpu cpu(
    .clk               (clk),
    .rst_n             (rst_n),
    .instruction_fetch (instruction_fetch),
    .fetch_addr        (fetch_addr),
    .fetch_valid       (fetch_valid),
    .ext_irq           (1'b0),
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

