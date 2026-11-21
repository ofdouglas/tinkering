# CPU/SoC verification from FPGA/cpu.
# Verilator is the default. Vivado xsim is optional (sim-system-vivado).

.PHONY: help test cputest sim-system sim-system-verilator sim-system-vivado sim-system-wdog-reset firmware sim-watchdog

.DEFAULT_GOAL := help

help:
	@echo "CPU/SoC tests"
	@echo ""
	@echo "  make test                 Run test-fw (cpu_tb) and system_tb"
	@echo "  make cputest              Run cpu/test-fw Verilator tests (MEM_LATENCY=1,2,4)"
	@echo "  make sim-watchdog         Run watchdog_periph unit testbench"
	@echo "  make sim-system           Run system_tb (Verilator, builds firmware)"
	@echo "  make sim-system-wdog-reset Throwaway bootloader + system_tb watchdog reset"
	@echo "  make sim-system-vivado    Run system_tb in Vivado xsim"
	@echo "  make firmware             Build bootloader, main_app, and sim payload"

test: cputest sim-system
	@echo ""
	@echo "CPU project tests passed (cpu_tb + system_tb)"

cputest:
	@echo "=== cpu_tb (test-fw) ==="
	$(MAKE) -C test-fw sim
	@echo "=== cpu_tb (test-fw) MEM_LATENCY=2 ==="
	$(MAKE) -C test-fw sim MEM_LATENCY=2
	@echo "=== cpu_tb (test-fw) MEM_LATENCY=4 ==="
	$(MAKE) -C test-fw sim MEM_LATENCY=4

WATCHDOG_SIM_DIR := build/watchdog_tb
WATCHDOG_SIM_BIN := $(WATCHDOG_SIM_DIR)/Vwatchdog_tb
WATCHDOG_TB_SRCS := \
	rtl/bus_interface.sv \
	rtl/watchdog.sv \
	sim/watchdog_tb.sv

sim-watchdog: $(WATCHDOG_SIM_BIN)
	@echo "=== watchdog_tb (Verilator) ==="
	$(WATCHDOG_SIM_BIN)

$(WATCHDOG_SIM_BIN): $(WATCHDOG_TB_SRCS) | $(WATCHDOG_SIM_DIR)
	verilator --binary -sv --timing -Wno-TIMESCALEMOD -Wno-UNOPTFLAT \
		--top-module watchdog_tb --Mdir $(WATCHDOG_SIM_DIR) $(WATCHDOG_TB_SRCS)

$(WATCHDOG_SIM_DIR):
	mkdir -p $@

sim-system sim-system-verilator: firmware
	@echo "=== system_tb (Verilator) ==="
	$(MAKE) -C firmware sim-system

sim-system-wdog-reset:
	$(MAKE) -C firmware sim-system-wdog-reset

sim-system-vivado: firmware
	$(MAKE) -C firmware sim-system-vivado

firmware:
	$(MAKE) -C firmware
