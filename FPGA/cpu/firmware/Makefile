THIRD_PARTY_DIR ?= $(HOME)/Design/third_party
FIRMWARE_DIR = ../../../Firmware
CPU_DIR = ..
FPGA_DIR = $(CPU_DIR)/..
include $(CPU_DIR)/riscv_toolchain.mk
ARCH   = -march=rv32i_zicsr_zmmul -mabi=ilp32
INC_DIRS = . $(sort $(patsubst %/,%,$(dir $(wildcard */*.h)))) $(FIRMWARE_DIR)/time
CFLAGS = $(ARCH) -g -O3 -ffreestanding -nostdlib $(addprefix -I,$(INC_DIRS))
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
ARITHMETIC_BIN = $(BUILD_DIR)/arithmetic_helpers.bin

BOARD_TTY ?= /dev/nexys-video
RESET_TTY ?= /dev/ttl234
# Select a board test or the coremark / coremark_validation benchmark.
BOARD_TEST ?= arithmetic_helpers
BOARD_TEST_IMAGE ?= $(BUILD_DIR)/$(BOARD_TEST).bin
TEST_TIMEOUT ?= $(if $(filter coremark coremark_validation,$(BOARD_TEST)),180,10)

SIM_APP_ELF = $(BUILD_DIR)/led_toggle.elf
SIM_APP_BIN = $(BUILD_DIR)/led_toggle.bin
SIM_APP_INSTALL = $(MEM_DIR)/led_toggle.bin

SHARED_SRCS_C = drivers/gpio.c drivers/time.c drivers/uart.c system/debug.c util/ringbuf.c
SHARED_SRCS_S = system/exception.S system/start.S
SHARED_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SHARED_SRCS_C)) \
              $(patsubst %.S,$(BUILD_DIR)/%.o,$(SHARED_SRCS_S))

BL_OBJS = $(BUILD_DIR)/bootloader/main.o $(SHARED_OBJS)
APP_OBJS = $(BUILD_DIR)/app/main.o $(SHARED_OBJS)
SIM_APP_OBJS = $(BUILD_DIR)/app/led_toggle.o

SYSTEM_SIM_DIR = $(BUILD_DIR)/system_tb
SYSTEM_SIM_BIN = $(SYSTEM_SIM_DIR)/Vsystem_tb
RTL_BUILD_ID ?= $(shell git -C ../../.. rev-parse --short=8 HEAD)
SYSTEM_SIM_BUILD_ID_STAMP = $(SYSTEM_SIM_DIR)/.rtl-build-id-$(RTL_BUILD_ID)
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
	$(CPU_DIR)/rtl/watchdog.sv \
	$(FPGA_DIR)/rtl/queue.sv \
	$(FPGA_DIR)/rtl/uart.sv \
	$(CPU_DIR)/rtl/system.sv \
	$(CPU_DIR)/sim/system_tb.sv \
	$(CPU_DIR)/rtl/debug_uart_controller.sv

WDOG_RESET_MAIN_O = $(BUILD_DIR)/bootloader/wdog_reset_main.o
WDOG_RESET_WDOG_O = $(BUILD_DIR)/drivers/watchdog.o
WDOG_RESET_OBJS = $(filter-out $(BUILD_DIR)/bootloader/main.o,$(BL_OBJS)) \
	$(WDOG_RESET_MAIN_O) $(WDOG_RESET_WDOG_O)
WDOG_RESET_ELF = $(BUILD_DIR)/bootloader_wdog_reset.elf
WDOG_RESET_HEX = $(BUILD_DIR)/bootloader_wdog_reset.hex
WDOG_RESET_LST = $(BUILD_DIR)/bootloader_wdog_reset.lst

HOST_CC ?= gcc
HOST_CFLAGS = -DHOST_BUILD -O2 -Wall -Wextra -I.
HOST_BL_DIR = $(BUILD_DIR)/host
HOST_BL = $(HOST_BL_DIR)/bootloader
HOST_BL_SRCS = bootloader/main.c \
	bootloader/host/uart.c \
	bootloader/host/time.c \
	bootloader/host/platform.c

.PHONY: all bootloader app sim-app cputest \
	sim-system sim-system-verilator sim-system-vivado sim-system-wdog-reset \
	host-bootloader test-download board-test flash serial reset clean

all: bootloader app sim-app

bootloader: $(HEX_INSTALL) $(BOOTLOADER_LST) $(BOOTLOADER_MAP) $(BOOTLOADER_SECTIONS)

app: $(APP_BIN) $(APP_LST)

.PHONY: arithmetic-helpers flash-arithmetic-helpers
arithmetic-helpers: $(ARITHMETIC_BIN)

flash-arithmetic-helpers: $(ARITHMETIC_BIN)
	python3 downloader.py "$<" "$(BOARD_TTY)"

BOARD_TEST_ELFS = $(addprefix $(BUILD_DIR)/,arithmetic_helpers.elf chacha20.elf xorshift32.elf)
$(BOARD_TEST_ELFS): $(BUILD_DIR)/%.elf: $(BUILD_DIR)/tests/%.o $(SHARED_OBJS) $(APP_LDSCRIPT)
	$(PREFIX)g++ $(LDFLAGS) -T $(APP_LDSCRIPT) -o $@ $(filter %.o,$^) -lgcc

include benchmarks/coremark/coremark.mk

sim-app: $(SIM_APP_INSTALL)

flash: $(APP_BIN)
	python3 downloader.py "$<" "$(BOARD_TTY)"

# Firmware prints a newline-terminated PASS or FAIL verdict after boot.
board-test: $(BOARD_TEST_IMAGE)
	python3 -u downloader.py "$<" "$(BOARD_TTY)" --board-test "$(RESET_TTY)" --test-timeout "$(TEST_TIMEOUT)"

serial:
	picocom -b 115200 "$(BOARD_TTY)" --imap lfcrlf

reset:
	stty -F "$(RESET_TTY)" 115200 raw -echo -opost && printf '\217' > "$(RESET_TTY)"


host-bootloader: $(HOST_BL)

test-download: $(HOST_BL)
	@mkdir -p $(HOST_BL_DIR)
	python3 -c 'import pathlib; p=pathlib.Path("$(HOST_BL_DIR)/test_image.bin"); p.write_bytes(bytes(range(64)))'
	python3 downloader.py --host-test $(HOST_BL_DIR)/test_image.bin \
		--bootloader $(HOST_BL) \
		--image-out $(HOST_BL_DIR)/received.bin

cputest:
	$(MAKE) -C ../test-fw sim

sim-system: $(HEX_INSTALL) $(SIM_APP_INSTALL) $(SYSTEM_SIM_BIN)
	$(SYSTEM_SIM_BIN) +FIRMWARE_HEX=$(HEX_INSTALL) +APP_BIN=$(SIM_APP_BIN)

sim-system-verilator: sim-system

sim-system-wdog-reset: $(WDOG_RESET_HEX) $(SYSTEM_SIM_BIN)
	@echo "=== system_tb watchdog reset (throwaway bootloader) ==="
	$(SYSTEM_SIM_BIN) +FIRMWARE_HEX=$(WDOG_RESET_HEX) +WATCHDOG_RESET_TEST

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

$(BOOTLOADER_ELF): $(BL_OBJS) $(BOOTLOADER_LDSCRIPT) | $(BUILD_DIR)
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
	$(PREFIX)objdump -d $< > $@

$(BOOTLOADER_SECTIONS): $(BOOTLOADER_ELF)
	$(PREFIX)objdump -h $< > $@

$(BOOTLOADER_MAP): $(BOOTLOADER_ELF)
	$(PREFIX)objdump -t $< > $@


$(WDOG_RESET_ELF): $(WDOG_RESET_OBJS) $(BOOTLOADER_LDSCRIPT) | $(BUILD_DIR)
	$(PREFIX)g++ $(CPPFLAGS) $(LDFLAGS) -T $(BOOTLOADER_LDSCRIPT) -o $@ $(WDOG_RESET_OBJS) -lgcc

$(WDOG_RESET_HEX): $(WDOG_RESET_ELF)
	@COPY_DATA_LMA=$$($(PREFIX)nm -g $< | awk '$$3=="_copy_data" {print $$1; exit}'); \
	COPY_DATA_ROM=$$((0x$$COPY_DATA_LMA - 0x10000)); \
	$(PREFIX)objcopy \
		--only-section=.boot_rom_section \
		--change-section-address .boot_rom_section=0 \
		--only-section=.data \
		--change-section-address .data=$$COPY_DATA_ROM \
		-O verilog --verilog-data-width=4 $< $@
	$(PREFIX)objdump -d -S $< > $(WDOG_RESET_LST)

$(APP_ELF): $(APP_OBJS) $(APP_LDSCRIPT) | $(BUILD_DIR)
	$(PREFIX)g++ $(CPPFLAGS) $(LDFLAGS) -T $(APP_LDSCRIPT) -o $@ $(APP_OBJS) -lgcc

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
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

$(SIM_APP_INSTALL): $(SIM_APP_BIN) | $(MEM_DIR)
	cp $< $@

$(BUILD_DIR)/%.o: %.S Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(PREFIX)g++ $(CPPFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(PREFIX)g++ $(CPPFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(PREFIX)g++ $(CPPFLAGS) -MMD -MP -c $< -o $@

$(SYSTEM_SIM_BIN): $(SYSTEM_TB_SRCS) $(HEX_INSTALL) $(SYSTEM_SIM_BUILD_ID_STAMP) | $(SYSTEM_SIM_DIR)
	verilator --binary -sv --timing -Wno-TIMESCALEMOD -Wno-WIDTHEXPAND \
		"-DRTL_BUILD_ID=32'h$(RTL_BUILD_ID)" \
		--top-module system_tb --Mdir $(SYSTEM_SIM_DIR) $(SYSTEM_TB_SRCS)

$(SYSTEM_SIM_BUILD_ID_STAMP): | $(SYSTEM_SIM_DIR)
	rm -f $(SYSTEM_SIM_DIR)/.rtl-build-id-*
	touch $@

$(SYSTEM_SIM_DIR):
	mkdir -p $@

$(HOST_BL): $(HOST_BL_SRCS) bootloader/host/platform.h | $(HOST_BL_DIR)
	$(HOST_CC) $(HOST_CFLAGS) -o $@ $(HOST_BL_SRCS)

$(HOST_BL_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) *.o *.elf *.hex *.lst *.map *.sections \
		$(MEM_DIR)/bootloader.hex $(MEM_DIR)/main_app.hex \
		$(MEM_DIR)/led_toggle.bin $(MEM_DIR)/firmware.hex

-include $(wildcard $(BUILD_DIR)/*/*.d)
