PREFIX = riscv64-unknown-elf-
ARCH   = -march=rv32i -mabi=ilp32
CFLAGS = $(ARCH) -Os -ffreestanding -nostdlib
LDFLAGS = $(ARCH) -T linker.ld -nostdlib
BUILD_DIR = build
HEX_INSTALL = ../mem/firmware.hex
FIRMWARE_ELF = $(BUILD_DIR)/firmware.elf
FIRMWARE_HEX = $(BUILD_DIR)/firmware.hex
FIRMWARE_LST = $(BUILD_DIR)/firmware.lst
FIRMWARE_MAP = $(BUILD_DIR)/firmware.map
START_OBJ = $(BUILD_DIR)/start.o
MAIN_OBJ = $(BUILD_DIR)/main.o
CPUTEST ?= cputest
CPUTEST_SRC = cputest/$(CPUTEST).s
CPUTEST_EXPECTED_SRC = cputest/$(CPUTEST).expected
CPUTEST_OBJ = $(BUILD_DIR)/$(CPUTEST).o
CPUTEST_ELF = $(BUILD_DIR)/$(CPUTEST).elf
CPUTEST_HEX = $(BUILD_DIR)/$(CPUTEST).hex
CPUTEST_LST = $(BUILD_DIR)/$(CPUTEST).lst
CPUTEST_MAP = $(BUILD_DIR)/$(CPUTEST).map
CPUTEST_HEX_INSTALL = ../mem/$(CPUTEST).hex
CPUTEST_EXPECTED_INSTALL = ../mem/$(CPUTEST).expected

all: $(FIRMWARE_HEX) $(FIRMWARE_LST) $(FIRMWARE_MAP)

cputest: $(CPUTEST_HEX) $(CPUTEST_LST) $(CPUTEST_MAP) $(CPUTEST_EXPECTED_INSTALL)

$(BUILD_DIR):
	mkdir -p $@

$(FIRMWARE_HEX): $(FIRMWARE_ELF)
	$(PREFIX)objcopy -O verilog --verilog-data-width=4 $< $@
	@mkdir -p $(dir $(HEX_INSTALL))
	cp $@ $(HEX_INSTALL)
$(FIRMWARE_LST): $(FIRMWARE_ELF)
	$(PREFIX)objdump -d -S $< > $@
$(FIRMWARE_MAP): $(FIRMWARE_ELF)
	$(PREFIX)objdump -t $< > $@

$(CPUTEST_HEX): $(CPUTEST_ELF)
	$(PREFIX)objcopy -O verilog --verilog-data-width=4 $< $@
	@mkdir -p $(dir $(CPUTEST_HEX_INSTALL))
	cp $@ $(CPUTEST_HEX_INSTALL)
$(CPUTEST_LST): $(CPUTEST_ELF)
	$(PREFIX)objdump -d -S $< > $@
$(CPUTEST_MAP): $(CPUTEST_ELF)
	$(PREFIX)objdump -t $< > $@

$(CPUTEST_EXPECTED_INSTALL): $(CPUTEST_EXPECTED_SRC)
	@mkdir -p $(dir $@)
	cp $< $@

$(FIRMWARE_ELF): $(START_OBJ) $(MAIN_OBJ)
	$(PREFIX)gcc $(LDFLAGS) -o $@ $^ -lgcc

$(CPUTEST_ELF): $(CPUTEST_OBJ)
	$(PREFIX)gcc $(LDFLAGS) -o $@ $^ -lgcc

$(START_OBJ): start.S | $(BUILD_DIR)
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

$(MAIN_OBJ): main.c | $(BUILD_DIR)
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

$(CPUTEST_OBJ): $(CPUTEST_SRC) | $(BUILD_DIR)
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

.PHONY: all cputest clean
clean:
	rm -rf $(BUILD_DIR) *.o cputest/*.o *.elf *.hex *.lst *.map ../mem/*.hex ../mem/*.expected
