# Makefile build for motor-controller (STM32F746NGHx).
# Application: C++ (main.cpp). ST HAL / FreeRTOS / CMSIS: C.
#
# Prerequisites: make + arm-none-eabi-gcc/g++ on PATH.
#   WSL/Linux:  sudo apt install make gcc-arm-none-eabi
#
# From this directory:
#   make              # build -> build/motor-controller.elf
#   make clean
#   make flash        # OpenOCD + ST-Link
#   make reset

TARGET = motor-controller
BUILD_DIR = build
STM32_LINK_CXX = 1
LINKER_SCRIPT = STM32F746NGHX_FLASH.ld

INCLUDES = \
	-IInc \
	-IDrivers/STM32F7xx_HAL_Driver/Inc \
	-IDrivers/STM32F7xx_HAL_Driver/Inc/Legacy \
	-IMiddlewares/Third_Party/FreeRTOS/Source/include \
	-IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS \
	-IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 \
	-IDrivers/CMSIS/Device/ST/STM32F7xx/Include \
	-IDrivers/CMSIS/Include

include sources.cli.mk
include ../mk/stm32f7.mk
