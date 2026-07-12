PREFIX = riscv64-unknown-elf-
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

all: $(FIRMWARE_HEX) $(FIRMWARE_LST) $(FIRMWARE_MAP)

cputest:
	$(MAKE) -C ../test-fw sim

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

.PHONY: all cputest clean
clean:
	rm -rf $(BUILD_DIR) *.o *.elf *.hex *.lst *.map ../mem/firmware.hex
