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

// Data RAM for the CPU
bus_slave_interface #(.ADDR_MSB(MEMORY_ADDR_MSB)) sram_bus();
block_sram cpu_sram (
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

// System bus. address_bus[19:0] is used. [19:16] select regions
logic [31:0] address_bus;
logic [31:0] data_read_bus;
logic [31:0] data_write_bus;
logic [ 3:0] byte_enables;
logic memory_access_valid;
logic cpu_request;
logic cpu_trap;
logic rom_trap;
logic unaligned_write_trap;

logic is_write;
assign is_write = |byte_enables;

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
assign region = address_bus[19:16];

logic region_rom, region_ram, region_led, region_uart;
assign region_rom  = (region == REGION_ROM);
assign region_ram  = (region == REGION_RAM);
assign region_led  = (region == REGION_LED);
assign region_uart = (region == REGION_UART);

always_comb begin
    unique case (region)
        REGION_ROM : memory_access_valid = !is_write && rom_bus.rd_valid;
        REGION_RAM : memory_access_valid = is_write ? sram_bus.wr_ack : sram_bus.rd_valid;
        REGION_LED : memory_access_valid = is_write ? gpio_bus.wr_ack : gpio_bus.rd_valid;
        REGION_UART : memory_access_valid = is_write ? uart_bus.wr_ack : uart_bus.rd_valid;
        default : memory_access_valid = 0;
    endcase
end

// Read bus multiplexing
always_comb begin
    data_read_bus = '0;
    if (cpu_request && !is_write) begin
       unique case (region)
            REGION_ROM  : data_read_bus = rom_bus.rd_data;
            REGION_RAM  : data_read_bus = sram_bus.rd_data;
            REGION_LED  : data_read_bus = gpio_bus.rd_data;
            REGION_UART : data_read_bus = uart_bus.rd_data;
            default     : data_read_bus = '0;
        endcase
    end else begin
        data_read_bus = '0;
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


 picorv32 cpu(
    .clk(clk),
    .resetn(rst_n),
    .trap(cpu_trap),
    .mem_valid(cpu_request),
    .mem_instr(),
    .mem_ready(memory_access_valid),
    .mem_addr(address_bus),
    .mem_wdata(data_write_bus),
    .mem_wstrb(byte_enables),
    .mem_rdata(data_read_bus),
    .mem_la_read(),
    .mem_la_write(),
    .mem_la_addr(),
    .mem_la_wdata(),
    .mem_la_wstrb(),
    .pcpi_valid(),
    .pcpi_insn(),
    .pcpi_rs1(),
    .pcpi_rs2(),
    .pcpi_wr(),
    .pcpi_rd(),
    .pcpi_wait(),
    .pcpi_ready(),
    .irq('0),
    .eoi(),
    .trace_valid(),
    .trace_data()
 );

endmodule

