# CPU/SoC verification from FPGA/cpu.
# Verilator is the default. Vivado xsim is optional (sim-system-vivado).

.PHONY: help test cputest sim-system sim-system-verilator sim-system-vivado firmware

.DEFAULT_GOAL := help

help:
	@echo "CPU/SoC tests"
	@echo ""
	@echo "  make test                 Run test-fw (cpu_tb) and system_tb"
	@echo "  make cputest              Run cpu/test-fw Verilator tests (MEM_LATENCY=1,2,4)"
	@echo "  make sim-system           Run system_tb (Verilator, builds firmware)"
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

sim-system sim-system-verilator: firmware
	@echo "=== system_tb (Verilator) ==="
	$(MAKE) -C firmware sim-system

sim-system-vivado: firmware
	$(MAKE) -C firmware sim-system-vivado

firmware:
	$(MAKE) -C firmware
