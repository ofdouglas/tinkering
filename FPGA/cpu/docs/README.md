# CPU Docs

Brief onboarding notes for agents working under `FPGA/cpu`. Keep design details in the focused docs below; this file should stay small.

## Project Context

This tree is for RV32 softcore CPU and SoC-on-FPGA work. The main core today is a simple 5-stage RV32I CPU with the strongest validation coverage in Verilator CPU-level tests.

The current FPGA target is Digilent Nexys Video (`XC7A200T`). SoC/peripheral work exists for UART, GPIO, memory, and timer integration, but CPU simulation remains the default validation path.

## Organization

- `rtl/` - CPU, SoC wrapper, bus, SRAM/ROM, GPIO/UART peripherals.
- `sim/` - Verilator testbenches and helpers; `cpu_tb.sv` is the main CPU validation bench, `system_tb.sv` is the firmware/system smoke bench.
- `test-fw/` - RV32I asm/C validation programs, expected register/SRAM data, and the default `make`/Verilator flow.
- `firmware/` - Demo firmware built into `mem/firmware.hex` for `system_tb` and Vivado.
- `scripts/` - Vivado Tcl scripts. The generated FPGA project lives at `FPGA/vivado/cpu`.
- `docs/` - Working notes: `validation_work.md`, `rtl_work.md`, `isa_notes.md`.

## Tools And Validation

Verilator is the default simulator for day-to-day validation. The test firmware flow uses `riscv64-unknown-elf-` tools; Spike is optional for ISA reference checks. Vivado is used for FPGA project generation, xsim, and synthesis.

CPU validation commands from `FPGA/cpu/test-fw`:

```sh
make sim
make sim TEST=load_use VERBOSE=1 DUMP_REGS=1
make sim MEM_LATENCY=2
make spike-ref
```

Firmware and Vivado entry points:

```sh
make -C FPGA/cpu/firmware
vivado -mode batch -source FPGA/cpu/scripts/create_project.tcl
vivado -mode batch -source FPGA/cpu/scripts/sim.tcl
```

`system_tb.sv` is the current Verilator SoC smoke bench, but it still needs a first-class Make target to build firmware, compile the bench, and pass `+FIRMWARE_HEX=...`.

For synthesis, open or regenerate `FPGA/vivado/cpu/cpu.xpr` and run `FPGA/cpu/scripts/build.tcl`.
