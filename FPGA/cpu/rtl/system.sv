import cpu_config_pkg::*;

module system(
    input  logic clk,
    output logic [7:0] led,
    output logic uart_tx_out,
    input logic uart_rx_in
);

localparam int RESET_CYCLES = 16;
logic [$clog2(RESET_CYCLES + 1) - 1:0] reset_cnt = '0;
logic rst_n;

always_ff @(posedge clk) begin
    if (reset_cnt < RESET_CYCLES) begin
        reset_cnt <= reset_cnt + 1'b1;
        rst_n     <= 1'b0;
    end else begin
        rst_n     <= 1'b1;
    end
end

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

// LED Register
logic [31:0] led_bits;          // Offset 0
assign led[3:0] = led_bits[3:0];
assign led[4] = 0;
assign led[5] = 0;
assign led[6] = rom_trap;
assign led[7] = cpu_trap;

// UART Registers
logic [7:0] uart_reg_tx_data;   // Offset 4
logic       uart_tx_ready;
logic       uart_tx_strobe;
logic [7:0] uart_rx_data_r;
logic       uart_rx_valid_w;




// System bus. address_bus[19:0] is used. [19:16] select regions
logic [31:0] address_bus;
logic [31:0] data_read_bus;
logic [31:0] data_write_bus;
logic [ 3:0] byte_enables;
logic memory_access_valid;
logic cpu_request;
logic cpu_trap;
logic rom_trap;

logic is_write;
assign is_write = |byte_enables;

logic [3:0] region;
assign region = address_bus[19:16];

logic region_rom, region_ram, region_led, region_uart;
assign region_rom  = (region == REGION_ROM);
assign region_ram  = (region == REGION_RAM);
assign region_led  = (region == REGION_LED);
assign region_uart = (region == REGION_UART);

logic led_write, uart_write;
assign led_write    = region_led && cpu_request && is_write;
assign uart_write   = region_uart && cpu_request && is_write;

always_comb begin
    unique case (region)
        REGION_ROM : memory_access_valid = !is_write && rom_bus.rd_valid;
        REGION_RAM : memory_access_valid = is_write ? sram_bus.wr_ack : sram_bus.rd_valid;
        REGION_LED : memory_access_valid = 1;
        REGION_UART : memory_access_valid = 1;
        default : memory_access_valid = 0;
    endcase
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


// Bus multiplexing
always_comb begin
    data_read_bus = '0;
    if (cpu_request && !is_write) begin
       unique case (region)
            REGION_ROM  : data_read_bus = rom_bus.rd_data;
            REGION_RAM  : data_read_bus = sram_bus.rd_data;
            REGION_LED  : data_read_bus = led_bits;
            REGION_UART : data_read_bus = uart_module_out;
            default     : data_read_bus = '0;
        endcase
    end else begin
        data_read_bus = '0;
    end
end


// UART module registers
always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        uart_reg_tx_data <= '0;
        uart_tx_strobe   <= 1'b0;
    end else begin
        uart_tx_strobe <= 1'b0;
        if (uart_tx_ready && uart_write && byte_enables[0] && address_bus[3:2] == 2'b01) begin
            uart_reg_tx_data <= data_write_bus[7:0];
            uart_tx_strobe   <= 1'b1;
        end
    end
end

logic [7:0]  uart_module_out;
always_comb begin
    case (address_bus[3:2])
        2'b00 : uart_module_out = {5'b0, uart_tx_strobe, uart_rx_valid_w, uart_tx_ready};
        2'b01 : uart_module_out = uart_reg_tx_data;
        2'b10 : uart_module_out = uart_rx_data_r;
        default : uart_module_out = '0;
    endcase
end

// LED registers
always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        led_bits <= '0;
    end else begin
        for (int i = 0; i < 4; i++) begin
            if (led_write && byte_enables[i]) begin
                led_bits[8*i +: 8] <= data_write_bus[8*i +: 8];
            end
        end
    end
end


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

  uart uart1(
    .clk(clk),
    .rst_n(rst_n),
    .tx_data(uart_reg_tx_data),
    .tx_valid(uart_tx_strobe),
    .tx_ready(uart_tx_ready),
    .tx_out(uart_tx_out),
    .rx_data(uart_rx_data_r),
    .rx_valid(uart_rx_valid_w),
    .rx_in(uart_rx_in)
    );

endmodule

