# FPGA

Shared RTL (`rtl/`), unit testbenches (`tb/`), and the RV32 CPU/SoC project (`cpu/`).

## Verilator unit sim

From `FPGA/` (WSL or Linux with `verilator` and `g++` installed):

```sh
make list
make sim MODULE=crc
make trace MODULE=crc   # generate FST only
make wave MODULE=crc    # generate + open in Cursor (Surfer extension)
make lint
make clean
```

Registered modules: `crc`,  `uart`, `led`.

Artifacts go to `build/sim/<module>/`. Waveforms: `build/sim/<module>/<top>.fst`.

Pass extra plusargs: `make sim MODULE=crc SIM_ARGS='+verbose'` (if the TB supports them).

## CPU validation (unchanged)

```sh
make cpu-test
# or
make -C cpu/test-fw sim
make -C cpu/test-fw sim TEST=load_use VERBOSE=1
```

## Vivado

- **CPU/SoC FPGA**: `vivado -mode batch -source cpu/scripts/create_project.tcl`
- **Unit bring-up**: GUI project under `vivado/sandbox/` (not driven by this Makefile)
- Use Vivado xsim when you want the integrated waveform viewer or full SoC sim with firmware

## Layout

| Path | Contents |
|------|----------|
| `rtl/` | Reusable blocks (`uart`, `crc`, `led`, …) |
| `tb/` | Unit testbenches (`tb_*`) |
| `cpu/rtl/` | CPU, bus, SoC wrappers, `*_periph.sv` |
| `cpu/sim/` | `cpu_tb`, `system_tb` |
| `mk/` | Shared Makefile rules |
| `build/` | Verilator sim output (gitignored) |
| `vivado/` | Generated Vivado projects (gitignored at repo root) |

## Notes

- Verilator needs `--timing` for `#` delays in testbenches.
- Verilator builds always include `--trace-fst`; dumps are written only when `+trace` is passed.
- `make wave` opens `.fst` in Cursor via the [Surfer](https://marketplace.visualstudio.com/items?itemName=surfer-project.surfer) extension. From WSL, Cursor is auto-detected under `%LOCALAPPDATA%/Programs/cursor/`. Override with `CURSOR=/path/to/Cursor.exe` if needed.
- XSim in Vivado is still used for CPU/SoC; class-heavy SV testbenches may not elaborate on XSim.
