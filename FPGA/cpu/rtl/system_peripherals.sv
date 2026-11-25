import cpu_config_pkg::*;

module system_peripherals #(
    parameter int ADDR_MSB = PERIPH_ADDR_MSB + 3
) (
    bus_slave_interface.slave bus,
    
    // GPIO (LED control)
    output logic [7:0]  sys_gpio_led_enables,

    // UART (Debug UART)
    output logic        uart_irq_out,
    output logic        uart_tx_out,
    input  logic        uart_rx_in,

    // SPI
    output logic        spi_clk,
    output logic        spi_cs_n,
    output logic        spi_mosi,
    input  logic        spi_miso,

    // RISC-V Machine Timer (Timer/Counter)
    output logic        mti_irq,
    output logic [63:0] mtime,

    // Watchdog (System Reset)
    output logic        reset_request
);

localparam logic [2:0] PERIPH_GPIO = 3'd0;
localparam logic [2:0] PERIPH_UART = 3'd1;
localparam logic [2:0] PERIPH_RV32_MACHINE_TIMERS = 3'd2;
localparam logic [2:0] PERIPH_WATCHDOG = 3'd3;
localparam logic [2:0] PERIPH_SPI = 3'd4;

// RISC-V Machine Timer
bus_slave_interface #(.ADDR_MSB(PERIPH_ADDR_MSB)) rv32_machine_timer_bus();
rv32_machine_timers rv32_machine_timer (
    .bus(rv32_machine_timer_bus.slave),
    .mti_irq(mti_irq),
    .mtime(mtime)
);

// GPIO (LED control)
bus_slave_interface #(.ADDR_MSB(PERIPH_ADDR_MSB)) gpio_bus();
gpio system_gpio (
    .bus(gpio_bus.slave),
    .led_driver_enables(sys_gpio_led_enables)
);

// Debug UART
bus_slave_interface #(.ADDR_MSB(PERIPH_ADDR_MSB)) uart_bus();
uart_periph uart0 (
    .bus(uart_bus.slave),
    .uart_irq_out(uart_irq_out),
    .uart_tx_out(uart_tx_out),
    .uart_rx_in(uart_rx_in)
);

// Watchdog
bus_slave_interface #(.ADDR_MSB(PERIPH_ADDR_MSB)) watchdog_bus();
watchdog_peripheral watchdog0 (
    .bus(watchdog_bus.slave),
    .reset_request(reset_request)
);

// SPI
bus_slave_interface #(.ADDR_MSB(PERIPH_ADDR_MSB)) spi_bus();
spi_periph spi0 (
    .bus(spi_bus.slave),
    .sck(spi_clk),
    .cs_n(spi_cs_n),
    .mosi(spi_mosi),
    .miso(spi_miso)
);

assign rv32_machine_timer_bus.clk = bus.clk;
assign rv32_machine_timer_bus.rst_n = bus.rst_n;
assign gpio_bus.clk = bus.clk;
assign gpio_bus.rst_n = bus.rst_n;
assign uart_bus.clk = bus.clk;
assign uart_bus.rst_n = bus.rst_n;
assign watchdog_bus.clk = bus.clk;
assign watchdog_bus.rst_n = bus.rst_n;
assign spi_bus.clk = bus.clk;
assign spi_bus.rst_n = bus.rst_n;

logic [2:0] peripheral_select;
logic [PERIPH_ADDR_MSB:2] periph_reg_offset;

assign peripheral_select = bus.addr[ADDR_MSB:PERIPH_ADDR_MSB+1];
assign periph_reg_offset = bus.addr[PERIPH_ADDR_MSB:2];

// Default assignments for peripheral buses to avoid latches
assign gpio_bus.addr      = periph_reg_offset;
assign gpio_bus.valid     = (peripheral_select == PERIPH_GPIO) ? bus.valid     : 1'b0;
assign gpio_bus.wr_strobe = (peripheral_select == PERIPH_GPIO) ? bus.wr_strobe : 4'b0000;
assign gpio_bus.wr_data   = (peripheral_select == PERIPH_GPIO) ? bus.wr_data   : '0;

assign uart_bus.addr      = periph_reg_offset;
assign uart_bus.valid     = (peripheral_select == PERIPH_UART) ? bus.valid     : 1'b0;
assign uart_bus.wr_strobe = (peripheral_select == PERIPH_UART) ? bus.wr_strobe : 4'b0000;
assign uart_bus.wr_data   = (peripheral_select == PERIPH_UART) ? bus.wr_data   : '0;

assign rv32_machine_timer_bus.addr      = periph_reg_offset;
assign rv32_machine_timer_bus.valid     = (peripheral_select == PERIPH_RV32_MACHINE_TIMERS) ? bus.valid     : 1'b0;
assign rv32_machine_timer_bus.wr_strobe = (peripheral_select == PERIPH_RV32_MACHINE_TIMERS) ? bus.wr_strobe : 4'b0000;
assign rv32_machine_timer_bus.wr_data   = (peripheral_select == PERIPH_RV32_MACHINE_TIMERS) ? bus.wr_data   : '0;

assign watchdog_bus.addr      = periph_reg_offset;
assign watchdog_bus.valid     = (peripheral_select == PERIPH_WATCHDOG) ? bus.valid     : 1'b0;
assign watchdog_bus.wr_strobe = (peripheral_select == PERIPH_WATCHDOG) ? bus.wr_strobe : 4'b0000;
assign watchdog_bus.wr_data   = (peripheral_select == PERIPH_WATCHDOG) ? bus.wr_data   : '0;

assign spi_bus.addr      = periph_reg_offset;
assign spi_bus.valid     = (peripheral_select == PERIPH_SPI) ? bus.valid     : 1'b0;
assign spi_bus.wr_strobe = (peripheral_select == PERIPH_SPI) ? bus.wr_strobe : 4'b0000;
assign spi_bus.wr_data   = (peripheral_select == PERIPH_SPI) ? bus.wr_data   : '0;


// Peripheral bus mux
always_comb begin
    bus.error    = 1'b0;
    bus.wr_ack   = 1'b0;
    bus.rd_valid = 1'b0;
    bus.rd_data  = '0;

    unique case (peripheral_select)
        PERIPH_GPIO: begin
            bus.wr_ack   = gpio_bus.wr_ack;
            bus.rd_data  = gpio_bus.rd_data;
            bus.rd_valid = gpio_bus.rd_valid;
            bus.error    = gpio_bus.error;
        end
        PERIPH_UART: begin
            bus.wr_ack   = uart_bus.wr_ack;
            bus.rd_data  = uart_bus.rd_data;
            bus.rd_valid = uart_bus.rd_valid;
            bus.error    = uart_bus.error;
        end
        PERIPH_RV32_MACHINE_TIMERS: begin
            bus.wr_ack   = rv32_machine_timer_bus.wr_ack;
            bus.rd_data  = rv32_machine_timer_bus.rd_data;
            bus.rd_valid = rv32_machine_timer_bus.rd_valid;
            bus.error    = rv32_machine_timer_bus.error;
        end
        PERIPH_WATCHDOG: begin
            bus.wr_ack   = watchdog_bus.wr_ack;
            bus.rd_data  = watchdog_bus.rd_data;
            bus.rd_valid = watchdog_bus.rd_valid;
            bus.error    = watchdog_bus.error;
        end
        PERIPH_SPI: begin
            bus.wr_ack   = spi_bus.wr_ack;
            bus.rd_data  = spi_bus.rd_data;
            bus.rd_valid = spi_bus.rd_valid;
            bus.error    = spi_bus.error;
        end
        default: begin
            bus.error    = bus.valid;
            bus.wr_ack   = 1'b0;
            bus.rd_valid = 1'b0;
            bus.rd_data  = '0;
        end
    endcase
end

endmodule
