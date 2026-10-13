TARGET := f746g-disco
BUILD_DIR := build

THIRD_PARTY := ../../../third_party
DEVICE_DIR := $(THIRD_PARTY)/cmsis-device-f7
CMSIS_CORE_DIR := $(THIRD_PARTY)/CMSIS_6/CMSIS/Core
LINKER_SCRIPT := STM32F746NGHX_FLASH.ld

TOOLCHAIN_PREFIX ?= arm-none-eabi-
CC := $(TOOLCHAIN_PREFIX)gcc
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy
SIZE := $(TOOLCHAIN_PREFIX)size

CPU_FLAGS := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard
CPPFLAGS := \
	-DSTM32F746xx \
	-I$(DEVICE_DIR)/Include \
	-I$(CMSIS_CORE_DIR)/Include
CFLAGS := $(CPU_FLAGS) -std=c11 -Og -g3 \
	-ffunction-sections -fdata-sections \
	-Wall -Wextra
ASFLAGS := $(CPU_FLAGS) $(CPPFLAGS) -x assembler-with-cpp
LDFLAGS := $(CPU_FLAGS) -T$(LINKER_SCRIPT) \
	--specs=nano.specs --specs=nosys.specs \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
	-Wl,--gc-sections

OBJECTS := \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/startup_stm32f746xx.o
DEPS := $(OBJECTS:.o=.d)

OPENOCD ?= openocd
OPENOCD_INTERFACE ?= interface/stlink.cfg
OPENOCD_TARGET ?= target/stm32f7x.cfg
GDB ?= gdb-multiarch
GDB_HOST ?= localhost
GDB_PORT ?= 3333

.PHONY: all clean size flash reset gdbserver gdb

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex size

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LINKER_SCRIPT)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/main.o: main.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/startup_stm32f746xx.o: startup_stm32f746xx.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -MMD -MP -c $< -o $@

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) --format=berkeley $<

flash: $(BUILD_DIR)/$(TARGET).elf
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) \
		-c "program $< verify reset exit"

reset:
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) \
		-c "init; reset; exit"

gdbserver:
	$(OPENOCD) -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET)

gdb: $(BUILD_DIR)/$(TARGET).elf
	$(GDB) $< -ex "target remote $(GDB_HOST):$(GDB_PORT)"

serial:
	picocom -b 19200 --imap lfcrlf /dev/ttyACM0

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
