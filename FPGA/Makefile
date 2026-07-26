# Verilator unit sim for shared RTL blocks under rtl/ and tb/.
# CPU/SoC flows remain under cpu/test-fw and cpu/firmware.

FPGA_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
MODULE    ?=
TRACE     ?= 0
SIM_ARGS  ?=

include mk/verilator.mk

.PHONY: help list cpu-test

.DEFAULT_GOAL := help

help:
	@echo "FPGA Verilator unit sim"
	@echo ""
	@echo "  make list                  List registered modules"
	@echo "  make sim MODULE=<name>     Run simulation"
	@echo "  make trace MODULE=<name>   Run sim, write FST (no viewer)"
	@echo "  make wave MODULE=<name>    trace + open in Cursor (Surfer extension)"
	@echo "                             WAVE_VIEWER=gtkwave for GTKWave instead"
	@echo "  make lint                  Lint all modules"
	@echo "  make lint-one MODULE=<name>"
	@echo "  make clean                 Remove build/"
	@echo ""
	@echo "  make cpu-test              Run CPU Verilator tests (cpu/test-fw)"
	@echo ""
	@echo "Modules: $(MODULES)"

list:
	@echo $(MODULES)

cpu-test:
	$(MAKE) -C cpu/test-fw sim
