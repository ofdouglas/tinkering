PREFIX = riscv64-unknown-elf-
CPU_DIR = ..
FPGA_DIR = $(CPU_DIR)/..
ARCH   = -march=rv32i -mabi=ilp32
CFLAGS = $(ARCH) -Os -ffreestanding -nostdlib
LDFLAGS = $(ARCH) -T linker.ld -nostdlib
# ELF addresses start at BOOT_ROM 0x00010000; make the Verilog image array-relative.
HEX_ADDR_OFFSET = -0x00010000
BUILD_DIR = build
HEX_INSTALL = ../mem/firmware.hex
FIRMWARE_ELF = $(BUILD_DIR)/firmware.elf
FIRMWARE_HEX = $(BUILD_DIR)/firmware.hex
FIRMWARE_LST = $(BUILD_DIR)/firmware.lst
FIRMWARE_MAP = $(BUILD_DIR)/firmware.map
START_OBJ = $(BUILD_DIR)/start.o
MAIN_OBJ = $(BUILD_DIR)/main.o
SYSTEM_SIM_DIR = $(BUILD_DIR)/system_tb
SYSTEM_SIM_BIN = $(SYSTEM_SIM_DIR)/Vsystem_tb
VIVADO_BAT ?= $(lastword $(wildcard /mnt/c/Xilinx/Vivado/*/bin/vivado.bat))
VIVADO ?= $(if $(VIVADO_BAT),cmd.exe /C $(VIVADO_BAT),vivado)
SYSTEM_TB_SRCS = \
	$(CPU_DIR)/sim/test_data_pkg.sv \
	$(CPU_DIR)/rtl/cpu_config_pkg.sv \
	$(CPU_DIR)/rtl/bus_interface.sv \
	$(CPU_DIR)/rtl/rv32defs.sv \
	$(CPU_DIR)/rtl/rv32cpu.sv \
	$(CPU_DIR)/rtl/block_rom.sv \
	$(CPU_DIR)/rtl/block_sram.sv \
	$(CPU_DIR)/rtl/reset_control.sv \
	$(CPU_DIR)/rtl/system_peripherals.sv \
	$(CPU_DIR)/rtl/gpio.sv \
	$(CPU_DIR)/rtl/uart_periph.sv \
	$(CPU_DIR)/rtl/rv32_machine_timers.sv \
	$(FPGA_DIR)/rtl/uart.sv \
	$(CPU_DIR)/rtl/system.sv \
	$(CPU_DIR)/sim/system_tb.sv

all: $(FIRMWARE_HEX) $(FIRMWARE_LST) $(FIRMWARE_MAP)

cputest:
	$(MAKE) -C ../test-fw sim

sim-system: $(HEX_INSTALL) $(SYSTEM_SIM_BIN)
	$(SYSTEM_SIM_BIN) +FIRMWARE_HEX=$(HEX_INSTALL)

sim-system-verilator: sim-system

sim-system-vivado: $(HEX_INSTALL)
	@if [ "$(VIVADO)" = "vivado" ] && ! command -v vivado >/dev/null 2>&1; then \
		echo "Vivado not found on PATH."; \
		echo "Run from a Vivado shell, or pass VIVADO='cmd.exe /C /mnt/c/path/to/Vivado/bin/vivado.bat'"; \
		exit 127; \
	fi
	$(VIVADO) -mode batch -source $(CPU_DIR)/scripts/sim.tcl

$(BUILD_DIR):
	mkdir -p $@

$(FIRMWARE_HEX): $(FIRMWARE_ELF)
	$(PREFIX)objcopy --change-addresses $(HEX_ADDR_OFFSET) -O verilog --verilog-data-width=4 $< $@
	@mkdir -p $(dir $(HEX_INSTALL))
	cp $@ $(HEX_INSTALL)
$(FIRMWARE_LST): $(FIRMWARE_ELF)
	$(PREFIX)objdump -d -S $< > $@
$(FIRMWARE_MAP): $(FIRMWARE_ELF)
	$(PREFIX)objdump -t $< > $@

$(FIRMWARE_ELF): $(START_OBJ) $(MAIN_OBJ) linker.ld
	$(PREFIX)gcc $(LDFLAGS) -o $@ $(START_OBJ) $(MAIN_OBJ) -lgcc

$(START_OBJ): start.S linker.ld | $(BUILD_DIR)
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

$(MAIN_OBJ): main.c mem_map.h linker.ld | $(BUILD_DIR)
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

$(SYSTEM_SIM_BIN): $(SYSTEM_TB_SRCS) $(HEX_INSTALL) | $(SYSTEM_SIM_DIR)
	verilator --binary -sv --timing -Wno-TIMESCALEMOD -Wno-WIDTHEXPAND \
		--top-module system_tb --Mdir $(SYSTEM_SIM_DIR) $(SYSTEM_TB_SRCS)

$(SYSTEM_SIM_DIR):
	mkdir -p $@

.PHONY: all cputest sim-system sim-system-verilator sim-system-vivado clean
clean:
	rm -rf $(BUILD_DIR) *.o *.elf *.hex *.lst *.map ../mem/firmware.hex
