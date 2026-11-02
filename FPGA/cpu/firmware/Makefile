FIRMWARE_DIR = ../../../Firmware
CPU_DIR = ..
FPGA_DIR = $(CPU_DIR)/..
include $(CPU_DIR)/riscv_toolchain.mk
ARCH   = -march=rv32i_zicsr -mabi=ilp32
INC_DIRS = . $(sort $(patsubst %/,%,$(dir $(wildcard */*.h)))) $(FIRMWARE_DIR)/time
CFLAGS = $(ARCH) -O0 -ffreestanding -nostdlib $(addprefix -I,$(INC_DIRS))
CPP_DISABLE_FLAGS = -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables
CPPFLAGS = $(CFLAGS) $(CPP_DISABLE_FLAGS)
LDFLAGS = $(ARCH) -nostdlib -Wl,--build-id=none -Wl,--print-memory-usage

BUILD_DIR = build
MEM_DIR = ../mem
BOOTLOADER_LDSCRIPT = linker/bootloader.ld
APP_LDSCRIPT = linker/app.ld
SIM_APP_LDSCRIPT = linker/sram_text.ld

BOOTLOADER_ELF = $(BUILD_DIR)/bootloader.elf
BOOTLOADER_HEX = $(BUILD_DIR)/bootloader.hex
BOOTLOADER_LST = $(BUILD_DIR)/bootloader.lst
BOOTLOADER_MAP = $(BUILD_DIR)/bootloader.map
BOOTLOADER_SECTIONS = $(BUILD_DIR)/bootloader.elf.sections
HEX_INSTALL = $(MEM_DIR)/bootloader.hex

APP_ELF = $(BUILD_DIR)/main_app.elf
APP_BIN = $(BUILD_DIR)/main_app.bin
APP_HEX = $(BUILD_DIR)/main_app.hex
APP_LST = $(BUILD_DIR)/main_app.lst
APP_HEX_INSTALL = $(MEM_DIR)/main_app.hex

SIM_APP_ELF = $(BUILD_DIR)/led_toggle.elf
SIM_APP_BIN = $(BUILD_DIR)/led_toggle.bin
SIM_APP_INSTALL = $(MEM_DIR)/led_toggle.bin

SHARED_SRCS_C = drivers/gpio.c drivers/time.c drivers/uart.c system/debug.c util/ringbuf.c
SHARED_SRCS_S = system/exception.S system/start.S
SHARED_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SHARED_SRCS_C)) \
              $(patsubst %.S,$(BUILD_DIR)/%.o,$(SHARED_SRCS_S))

BL_SRCS_C = booloader/main.c $(SHARED_SRCS_C)
BL_SRCS_S = $(SHARED_SRCS_S)
BL_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(BL_SRCS_C)) \
          $(patsubst %.S,$(BUILD_DIR)/%.o,$(BL_SRCS_S))

APP_SRCS_CPP = app/main.cpp
APP_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(APP_SRCS_CPP)) $(SHARED_OBJS)

SIM_APP_OBJS = $(BUILD_DIR)/app/led_toggle.o

FW_DEPS = $(wildcard *.c *.cpp *.s *.S *.h *.ld) \
          $(wildcard */*.c */*.cpp */*.h */*.s */*.S */*.ld)

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

.PHONY: all bootloader app sim-app cputest \
	sim-system sim-system-verilator sim-system-vivado clean

all: bootloader app sim-app

bootloader: $(HEX_INSTALL) $(BOOTLOADER_LST) $(BOOTLOADER_MAP) $(BOOTLOADER_SECTIONS)

app: $(APP_HEX_INSTALL) $(APP_BIN) $(APP_LST)

sim-app: $(SIM_APP_INSTALL)

cputest:
	$(MAKE) -C ../test-fw sim

sim-system: $(HEX_INSTALL) $(SIM_APP_INSTALL) $(SYSTEM_SIM_BIN)
	$(SYSTEM_SIM_BIN) +FIRMWARE_HEX=$(HEX_INSTALL) +APP_BIN=$(SIM_APP_BIN)

sim-system-verilator: sim-system

sim-system-vivado: $(HEX_INSTALL) $(SIM_APP_INSTALL)
	@if [ "$(VIVADO)" = "vivado" ] && ! command -v vivado >/dev/null 2>&1; then \
		echo "Vivado not found on PATH."; \
		echo "Run from a Vivado shell, or pass VIVADO='cmd.exe /C /mnt/c/path/to/Vivado/bin/vivado.bat'"; \
		exit 127; \
	fi
	$(VIVADO) -mode batch -source $(CPU_DIR)/scripts/sim.tcl

$(BUILD_DIR):
	mkdir -p $@

$(MEM_DIR):
	mkdir -p $@

$(BOOTLOADER_ELF): $(BL_OBJS) $(FW_DEPS) | $(BUILD_DIR)
	$(PREFIX)g++ $(CPPFLAGS) $(LDFLAGS) -T $(BOOTLOADER_LDSCRIPT) -o $@ $(BL_OBJS) -lgcc

$(BOOTLOADER_HEX): $(BOOTLOADER_ELF)
	# Emit BOOT_ROM and the .data load image at its ROM-relative address.
	@COPY_DATA_LMA=$$($(PREFIX)nm -g $< | awk '$$3=="_copy_data" {print $$1; exit}'); \
	COPY_DATA_ROM=$$((0x$$COPY_DATA_LMA - 0x10000)); \
	$(PREFIX)objcopy \
		--only-section=.boot_rom_section \
		--change-section-address .boot_rom_section=0 \
		--only-section=.data \
		--change-section-address .data=$$COPY_DATA_ROM \
		-O verilog --verilog-data-width=4 $< $@

$(HEX_INSTALL): $(BOOTLOADER_HEX) | $(MEM_DIR)
	cp $< $@

$(BOOTLOADER_LST): $(BOOTLOADER_ELF)
	$(PREFIX)objdump -d -S $< > $@

$(BOOTLOADER_SECTIONS): $(BOOTLOADER_ELF)
	$(PREFIX)objdump -h $< > $@

$(BOOTLOADER_MAP): $(BOOTLOADER_ELF)
	$(PREFIX)objdump -t $< > $@

$(APP_ELF): $(APP_OBJS) $(FW_DEPS) | $(BUILD_DIR)
	$(PREFIX)g++ $(CPPFLAGS) $(LDFLAGS) -T $(APP_LDSCRIPT) -o $@ $(APP_OBJS) -lgcc

$(APP_BIN): $(APP_ELF)
	$(PREFIX)objcopy -O binary $< $@

$(APP_HEX): $(APP_BIN)
	$(PREFIX)objcopy -I binary -O verilog --verilog-data-width=4 \
		--change-section-address .data=0 $< $@

$(APP_HEX_INSTALL): $(APP_HEX) | $(MEM_DIR)
	cp $< $@

$(APP_LST): $(APP_ELF)
	$(PREFIX)objdump -d -S $< > $@

$(SIM_APP_ELF): $(SIM_APP_OBJS) $(SIM_APP_LDSCRIPT) | $(BUILD_DIR)
	$(PREFIX)g++ $(CPPFLAGS) $(LDFLAGS) -T $(SIM_APP_LDSCRIPT) -o $@ $(SIM_APP_OBJS) -lgcc

$(SIM_APP_BIN): $(SIM_APP_ELF)
	$(PREFIX)objcopy -O binary $< $@

$(SIM_APP_INSTALL): $(SIM_APP_BIN) | $(MEM_DIR)
	cp $< $@

$(BUILD_DIR)/%.o: %.S $(FW_DEPS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(PREFIX)g++ $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c $(FW_DEPS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(PREFIX)g++ $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp $(FW_DEPS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(PREFIX)g++ $(CPPFLAGS) -c $< -o $@

$(SYSTEM_SIM_BIN): $(SYSTEM_TB_SRCS) $(HEX_INSTALL) | $(SYSTEM_SIM_DIR)
	verilator --binary -sv --timing -Wno-TIMESCALEMOD -Wno-WIDTHEXPAND \
		--top-module system_tb --Mdir $(SYSTEM_SIM_DIR) $(SYSTEM_TB_SRCS)

$(SYSTEM_SIM_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) *.o *.elf *.hex *.lst *.map *.sections \
		$(MEM_DIR)/bootloader.hex $(MEM_DIR)/main_app.hex \
		$(MEM_DIR)/led_toggle.bin $(MEM_DIR)/firmware.hex
