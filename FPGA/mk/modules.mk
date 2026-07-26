# Registered RTL modules for Verilator unit sim (make sim MODULE=<name>).
MODULES := crc uart led

crc_RTL := rtl/crc.sv
crc_TB  := tb/tb_crc.sv
crc_TOP := tb_crc

uart_RTL := rtl/uart.sv
uart_TB  := tb/tb_uart.sv
uart_TOP := uart_test

led_RTL := rtl/led.sv
led_TB  := tb/tb_led.sv
led_TOP := tb_led
