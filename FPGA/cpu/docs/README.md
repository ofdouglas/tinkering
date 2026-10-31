# CPU Docs

Brief onboarding notes for agents working under `FPGA/cpu`. Keep design details in the focused docs below; this file should stay small.

## Project Context

This tree is for RV32 softcore CPU and SoC-on-FPGA work. The main core today is a simple 5-stage RV32I CPU with the strongest validation coverage in Verilator CPU-level tests.

The current FPGA target is Digilent Nexys Video (`XC7A200T`). SoC/peripheral work exists for UART, GPIO, memory, and timer integration, but CPU simulation remains the default validation path.

## Organization

- `rtl/` - CPU, SoC wrapper, bus, SRAM/ROM, GPIO/UART peripherals.
- `sim/` - Verilator testbenches and helpers; `cpu_tb.sv` is the main CPU validation bench, `system_tb.sv` is the firmware/system smoke bench.
- `test-fw/` - RV32I asm/C validation programs, expected register/SRAM data, and the default `make`/Verilator flow. See `AGENTS.md` for the asm `verify`/snapshot convention.
- `firmware/` - Demo firmware built into `mem/firmware.hex` for `system_tb` and Vivado.
- `scripts/` - Vivado Tcl scripts. The generated FPGA project lives at `FPGA/vivado/cpu`.
- `docs/` - Working notes: `validation_work.md`, `rtl_work.md`, `isa_notes.md`.

## Tools And Validation

Verilator is the default simulator for day-to-day validation. The test firmware flow uses `riscv64-unknown-elf-` tools; Spike is optional for ISA reference checks. Vivado is used for FPGA project generation, xsim, and synthesis.

CPU validation from `FPGA/` or `FPGA/cpu`:

```sh
make cpu-test                 # from FPGA/: cpu_tb + system_tb
make test                     # from FPGA/cpu: same
make cputest                  # cpu_tb only
make sim-system               # system_tb (Verilator; builds firmware)
make -C test-fw sim TEST=load_use VERBOSE=1 DUMP_REGS=1
make -C test-fw sim MEM_LATENCY=2
make -C test-fw spike-ref
```

Firmware and Vivado entry points:

```sh
make firmware
make sim-system-vivado
vivado -mode batch -source FPGA/cpu/scripts/create_project.tcl
vivado -mode batch -source FPGA/cpu/scripts/sim.tcl
```

For synthesis, open or regenerate `FPGA/vivado/cpu/cpu.xpr` and run `FPGA/cpu/scripts/build.tcl`.
