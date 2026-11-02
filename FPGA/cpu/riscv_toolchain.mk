# Locate riscv64-unknown-elf-* if it is not already on PATH.
# Vivado ships a copy under /opt/amd-fpga/<ver>/gnu/riscv/lin/bin.
PREFIX ?= riscv64-unknown-elf-

ifeq ($(shell command -v $(PREFIX)gcc 2>/dev/null),)
XILINX_RISCV_BIN := $(lastword $(sort $(wildcard \
	/opt/amd-fpga/*/gnu/riscv/lin/bin \
	/opt/Xilinx/*/gnu/riscv/lin/bin)))
ifneq ($(XILINX_RISCV_BIN),)
export PATH := $(XILINX_RISCV_BIN):$(PATH)
endif
endif
