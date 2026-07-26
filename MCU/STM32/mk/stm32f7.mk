# STM32F7 shared Makefile entry — include from project Makefile after TARGET and sources.

MK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

include $(MK_DIR)toolchain.mk
include $(MK_DIR)stm32f7-flags.mk
include $(MK_DIR)rules.mk
include $(MK_DIR)openocd.mk
