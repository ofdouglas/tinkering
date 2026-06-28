
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

logic cpu_trap;  

// Instruction ROM for the CPU
(* rom_style = "block" *)
logic [31:0] cpu_rom_array [0:1023];
logic [31:0] cpu_rom_out;

// Data RAM for the CPU
logic [31:0] data_ram_array [0:1023];
logic [31:0] data_ram_out;

// LED Register
logic [31:0] led_bits;          // Offset 0
assign led[6:0] = led_bits[6:0];
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
logic cpu_request;
logic ram_request;
logic led_request;
logic uart_request;

// Memory regions (address_bus[19:16])
localparam [3:0] ADDR_MUX_ROM    = 4'b0000;
localparam [3:0] ADDR_MUX_RAM    = 4'b0001;
localparam [3:0] ADDR_MUX_LED    = 4'b0010;
localparam [3:0] ADDR_MUX_UART   = 4'b0011;


// Bus multiplexing
always_comb begin
    ram_request = cpu_request && (address_bus[19:16] == ADDR_MUX_RAM);
    led_request = cpu_request && (address_bus[19:16] == ADDR_MUX_LED);  
    uart_request = cpu_request && (address_bus[19:16] == ADDR_MUX_UART);  

    if (byte_enables == '0) begin
        case (address_bus[19:16])
            ADDR_MUX_ROM : data_read_bus = cpu_rom_out;
            ADDR_MUX_RAM : data_read_bus = data_ram_out;
            ADDR_MUX_LED : data_read_bus = led_bits;  // TODO 
            ADDR_MUX_UART : data_read_bus = uart_module_out;  // TODO 
            default : data_read_bus = '0;
        endcase
    end else begin
        data_read_bus = '0;
    end
end


// ROM
always_comb begin
    cpu_rom_out = cpu_rom_array[address_bus[11:2]];
end
initial
    $readmemh("firmware.hex", cpu_rom_array);


// RAM
assign data_ram_out = data_ram_array[address_bus[11:2]];
always_ff @(posedge clk) begin
    for (int i = 0; i < 4; i++) begin
        if (ram_request && byte_enables[i]) begin
            data_ram_array[address_bus[11:2]][8*i +: 8] <= data_write_bus[8*i +: 8];
        end
    end
end

// UART module registers
always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        uart_reg_tx_data <= '0;
        uart_tx_strobe   <= 1'b0;
    end else begin
        uart_tx_strobe <= 1'b0;
        if (uart_tx_ready && uart_request && byte_enables[0] && address_bus[3:2] == 2'b01) begin
            uart_reg_tx_data <= data_write_bus[7:0];
            uart_tx_strobe   <= 1'b1;
        end
    end
end

logic        uart_rx_valid_w;
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
            if (led_request && byte_enables[i]) begin
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
    .mem_ready('1),
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

