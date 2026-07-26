# STM32F7 CPU / FPU / MCU flags and default preprocessor definitions.

CPU = -mcpu=cortex-m7
FPU = -mfpu=fpv5-sp-d16 -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU)

C_DEFS ?= -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx

CFLAGS = $(MCU) $(C_DEFS) $(INCLUDES) -std=gnu11 -g3 -O0 \
	-ffunction-sections -fdata-sections -Wall \
	-fstack-usage --specs=nano.specs

CXXFLAGS = $(MCU) $(C_DEFS) $(INCLUDES) -std=gnu++11 -g3 -O0 \
	-ffunction-sections -fdata-sections -Wall \
	-fno-exceptions -fno-rtti -fno-threadsafe-statics \
	-fstack-usage --specs=nano.specs

LDFLAGS = $(MCU) -T$(LINKER_SCRIPT) --specs=nosys.specs \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map -Wl,--gc-sections -static \
	--specs=nano.specs \
	-Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group
