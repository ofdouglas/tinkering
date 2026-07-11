import cpu_config_pkg::*;

module system_peripherals #(
    parameter int ADDR_MSB = MEMORY_ADDR_MSB
) (
    bus_slave_interface.slave bus,
    
    // GPIO (LED control)
    output logic [7:0]  sys_gpio_led_enables,

    // UART (Debug UART)
    output logic        uart_tx_out,
    input  logic        uart_rx_in

    // RISC-V Machine Timer (Timer/Counter)
    output logic [31:0] timer_low_value,
    output logic [31:0] timer_high_value,
    output logic        mti_irq
);

typedef enum logic [3:0] {
    PERIPH_GPIO = 4'h0,
    PERIPH_UART = 4'h1,
    PERIPH_RV32_MACHINE_TIMERS = 4'h2,
} sys_periph_reg_t;

// RISC-V Machine Timer Registers
bus_slave_interface #(.ADDR_MSB(PERIPH_ADDR_MSB)) rv32_machine_timer_bus();
rv32_machine_timers rv32_machine_timers (
    .bus(rv32_machine_timer_bus.slave),
    .timer_low_value(timer_low_value),
    .timer_high_value(timer_high_value),
    .mti_irq(mti_irq)
);
assign rv32_machine_timers.clk = bus.clk;
assign rv32_machine_timers.rst_n = bus.rst_n;

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
    .uart_tx_out(uart_tx_out),
    .uart_rx_in(uart_rx_in)
);

assign system_gpio.clk = bus.clk;
assign system_gpio.rst_n = bus.rst_n;
assign uart0.clk = bus.clk;
assign uart0.rst_n = bus.rst_n;

logic [ADDR_MSB:PERIPH_ADDR_MSB+1] peripheral_select;
logic [PERIPH_ADDR_MSB:2] periph_reg_offset;

assign peripheral_select = bus.addr[ADDR_MSB:PERIPH_ADDR_MSB+1];
assign periph_reg_offset = bus.addr[PERIPH_ADDR_MSB:2];

// Default assignments for peripheral buses to avoid latches
assign gpio_bus.slave.addr      = periph_reg_offset;
assign gpio_bus.slave.valid     = (peripheral_select == PERIPH_GPIO) ? bus.valid     : 1'b0;
assign gpio_bus.slave.wr_strobe = (peripheral_select == PERIPH_GPIO) ? bus.wr_strobe : 4'b0000;
assign gpio_bus.slave.wr_data   = (peripheral_select == PERIPH_GPIO) ? bus.wr_data   : 32'bX;

assign uart_bus.slave.addr      = periph_reg_offset;
assign uart_bus.slave.valid     = (peripheral_select == PERIPH_UART) ? bus.valid     : 1'b0;
assign uart_bus.slave.wr_strobe = (peripheral_select == PERIPH_UART) ? bus.wr_strobe : 4'b0000;
assign uart_bus.slave.wr_data   = (peripheral_select == PERIPH_UART) ? bus.wr_data   : 32'bX;

assign rv32_machine_timer_bus.slave.addr      = periph_reg_offset;
assign rv32_machine_timer_bus.slave.valid     = (peripheral_select == PERIPH_RV32_MACHINE_TIMERS) ? bus.valid     : 1'b0;
assign rv32_machine_timer_bus.slave.wr_strobe = (peripheral_select == PERIPH_RV32_MACHINE_TIMERS) ? bus.wr_strobe : 4'b0000;
assign rv32_machine_timer_bus.slave.wr_data   = (peripheral_select == PERIPH_RV32_MACHINE_TIMERS) ? bus.wr_data   : 32'bX;

// Peripheral bus mux
always_comb begin
    bus.error    = 1'b0;
    bus.wr_ack   = 1'b0;
    bus.rd_valid = 1'b0;
    bus.rd_data  = 'X;

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
        default: begin
            bus.error    = bus.valid;
            bus.wr_ack   = 1'b0;
            bus.rd_valid = 1'b0;
            bus.rd_data  = 'X;
        end
    endcase
end
