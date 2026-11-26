# Registered RTL modules for Verilator unit sim (make sim MODULE=<name>).
MODULES := crc uart queue led watchdog spi_master spi_periph

crc_RTL := rtl/crc.sv
crc_TB  := tb/tb_crc.sv
crc_TOP := tb_crc

uart_RTL := rtl/uart.sv
uart_TB  := tb/tb_uart.sv
uart_TOP := uart_test

queue_RTL := rtl/queue.sv
queue_TB  := tb/tb_queue.sv
queue_TOP := queue_test

led_RTL := rtl/led.sv
led_TB  := tb/tb_led.sv
led_TOP := tb_led

watchdog_RTL := cpu/rtl/watchdog.sv cpu/rtl/bus_interface.sv
watchdog_TB  := cpu/sim/watchdog_tb.sv
watchdog_TOP := watchdog_tb

spi_master_RTL := rtl/spi_master.sv
spi_master_TB  := tb/tb_spi_master.sv
spi_master_TOP := tb_spi_master

spi_periph_RTL := cpu/rtl/bus_interface.sv rtl/spi_master.sv cpu/rtl/spi_periph.sv
spi_periph_TB  := cpu/sim/spi_periph_tb.sv
spi_periph_TOP := spi_periph_tb
