# Makefile build for bootloader (STM32F746NGHx).
# Application: C++ (main.cpp). ST HAL / CMSIS: C.
#
# Prerequisites: make + arm-none-eabi-gcc/g++ on PATH.
#   WSL/Linux:  sudo apt install make gcc-arm-none-eabi
#
# From this directory:
#   make              # build -> build/bootloader.elf
#   make clean
#   make flash        # OpenOCD + ST-Link
#   make reset

TARGET = bootloader
BUILD_DIR = build
STM32_LINK_CXX = 1
LINKER_SCRIPT = STM32F746NGHX_BOOTLOADER.ld
FIRMWARE_ROOT = ../../../Firmware

# VPATH = Src:$(FIRMWARE_ROOT)/crc:$(FIRMWARE_ROOT)/hdlc

INCLUDES = \
	-I$(FIRMWARE_ROOT) \
	-IInc \
	-IDrivers/STM32F7xx_HAL_Driver/Inc \
	-IDrivers/STM32F7xx_HAL_Driver/Inc/Legacy \
	-IDrivers/CMSIS/Device/ST/STM32F7xx/Include \
	-IDrivers/CMSIS/Include

include sources.cli.mk
include ../mk/stm32f7.mk

CXXFLAGS := $(subst -std=gnu++11,-std=gnu++14,$(CXXFLAGS))
