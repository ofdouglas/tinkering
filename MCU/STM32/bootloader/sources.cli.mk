# Source list for command-line builds.
# Application: C++ (main.cpp). ST HAL / CMSIS: C.

CPP_SOURCES = \
	Src/main.cpp \
	Src/bsp.cpp \
	$(FIRMWARE_ROOT)/crc/crc.cpp \
	$(FIRMWARE_ROOT)/hdlc/hdlc.cpp \
	$(FIRMWARE_ROOT)/logging/log.cpp \
	$(FIRMWARE_ROOT)/bootloader/mcu/bootloader.cpp \
	$(FIRMWARE_ROOT)/bootloader/protocol.cpp

C_SOURCES = \
	Src/stm32f7xx_hal_msp.c \
	Src/stm32f7xx_it.c \
	Src/syscalls.c \
	Src/sysmem.c \
	Src/system_stm32f7xx.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart_ex.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash.c \
	Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_flash_ex.c

ASM_SOURCES = \
	Startup/startup_stm32f746nghx.s
