# Command-line build for STM32CubeIDE project "config" (STM32F746NGHx, Debug).
# Settings match .cproject Debug configuration. CubeIDE remains source of truth.
#
# Prerequisites: make + arm-none-eabi-gcc on PATH.
#   WSL/Linux:  sudo apt install make gcc-arm-none-eabi
#   Windows:    use CubeIDE GNU Tools, or set GCC_PATH to .../tools/bin
#
# From this directory (config/):
#   make              # build -> build/config.elf
#   make clean        # remove build/
#
# After adding sources in CubeIDE, refresh sources.cli.mk from Debug/objects.list.

TARGET = config
BUILD_DIR = build

# STM32CubeIDE 2.0.0 bundled GNU Tools for STM32 13.3.rel1 (from Debug/config.map)
CUBE_GCC_WIN = C:/ST/STM32CubeIDE_2.0.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712/tools/bin

# Toolchain: GCC_PATH overrides; else CubeIDE Windows tools; else arm-none-eabi-* on PATH.
ifdef GCC_PATH
  GNU_BIN = $(GCC_PATH)/
  TOOL_SUFFIX = $(if $(wildcard $(GNU_BIN)arm-none-eabi-gcc.exe),.exe,)
else ifneq ($(wildcard $(CUBE_GCC_WIN)/arm-none-eabi-gcc.exe),)
  GNU_BIN = $(CUBE_GCC_WIN)/
  TOOL_SUFFIX = .exe
else ifneq ($(shell which arm-none-eabi-gcc 2>/dev/null),)
  GNU_BIN =
  TOOL_SUFFIX =
else
  $(error arm-none-eabi-gcc not found — install gcc-arm-none-eabi or set GCC_PATH)
endif

CC = $(GNU_BIN)arm-none-eabi-gcc$(TOOL_SUFFIX)
AS = $(GNU_BIN)arm-none-eabi-gcc$(TOOL_SUFFIX)
SZ = $(GNU_BIN)arm-none-eabi-size$(TOOL_SUFFIX)
MKDIR = mkdir -p

CPU = -mcpu=cortex-m7
FPU = -mfpu=fpv5-sp-d16 -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU)

# From .cproject Debug: USE_HAL_DRIVER, STM32F746xx, DEBUG
C_DEFS = -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx

# From .cproject Debug include paths
C_INCLUDES = \
	-IInc \
	-IDrivers/STM32F7xx_HAL_Driver/Inc \
	-IDrivers/STM32F7xx_HAL_Driver/Inc/Legacy \
	-IMiddlewares/Third_Party/FreeRTOS/Source/include \
	-IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
	-IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 \
	-IMiddlewares/Third_Party/FatFs/src \
	-IMiddlewares/ST/STM32_USB_Host_Library/Core/Inc \
	-IMiddlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc \
	-IDrivers/CMSIS/Device/ST/STM32F7xx/Include \
	-IDrivers/CMSIS/Include

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -std=gnu11 -g3 -O0 \
	-ffunction-sections -fdata-sections -Wall \
	-fstack-usage --specs=nano.specs

# Match CubeIDE Debug link flags (relative linker script path)
LDFLAGS = $(MCU) -TSTM32F746NGHX_FLASH.ld --specs=nosys.specs \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map -Wl,--gc-sections -static \
	--specs=nano.specs -Wl,--start-group -lc -lm -Wl,--end-group

include sources.cli.mk

C_OBJS = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
ASM_OBJS = $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.s=.o))
OBJECTS = $(C_OBJS) $(ASM_OBJS)
DEPS = $(OBJECTS:.o=.d)

OPENOCD ?= openocd
OPENOCD_IF ?= interface/stlink.cfg
OPENOCD_TGT ?= target/stm32f7x.cfg

.PHONY: all clean size flash reset

all: $(BUILD_DIR)/$(TARGET).elf size

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) STM32F746NGHX_FLASH.ld
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(CC) -c $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -o $@ $<

$(BUILD_DIR)/%.o: %.s
	@$(MKDIR) $(dir $@)
	$(AS) -c $(MCU) $(C_DEFS) $(C_INCLUDES) -x assembler-with-cpp \
		--specs=nano.specs -MMD -MP -MF $(@:.o=.d) -o $@ $<

size: $(BUILD_DIR)/$(TARGET).elf
	$(SZ) --format=berkeley $<

flash: $(BUILD_DIR)/$(TARGET).elf
	$(OPENOCD) -f $(OPENOCD_IF) -f $(OPENOCD_TGT) \
		-c "init" -c "program $< verify reset" -c "shutdown"

reset:
	$(OPENOCD) -f $(OPENOCD_IF) -f $(OPENOCD_TGT) \
		-c "init" -c "reset" -c "shutdown"

gdbserver:
	openocd -f interface/stlink.cfg -f target/stm32f7x.cfg

gdb:
	gdb-multiarch $(BUILD_DIR)/$(TARGET).elf -ex "target remote :3333"

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
