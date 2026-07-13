# CPU Validation Working doc 

**Keep this document concise!**

## cputest coverage (done)

- RV32I ALU (incl. boundaries), branches, JAL/JALR, AUIPC/LUI
- Load/store all widths, sub-word misalignment, store-data EX forwarding
- Load-use stalls (incl. lh/lhu/sh, back-to-back loads), branch + load-use, pipelined ALU forwarding
- Trap/control smoke: ECALL/MRET, injected machine external IRQ, illegal instruction traps
- C smoke: startup/data init, CRC32, compiler store regression (`store_hazard`); `.sram` expected-file checks for memory results
- CSR read/write (`csr_basic`, `csr_immediate`, `csr_mixed`): CSRRW/CSRRS/CSRRC and CSRRWI/CSRRSI/CSRRCI on implemented CSRs; load-use and ALU→CSR operand hazards, CSR rd→branch/store/CSR-rs1, CSR WB + load-use overlap, immediate/register CSR mix
- Spike ISA cross-check: `make spike-ref` in `../test-fw/` (RV32I asm tests vs `expected/*.regs`; skips CSR/trap tests where Spike is not the intended reference — see `SPIKE_SKIP_TESTS` in `tests.mk`)
- System smoke: `system_tb` checks firmware LED0, UART "Hello!\n", debug LEDs, and optional SRAM expected contents

## Gaps to close (asm / sim)

- **TODO** - Add an integration smoke that exercises callee-saved register save/restore across a call that performs multi-cycle memory accesses (or timer writes/reads). Run it with simulated memory latency (e.g. MEM_LATENCY>1) and verify: (1) callee-saved registers are preserved after return, (2) the stack slot contents match the saved value, and (3) no stale rd_valid/rd_data or wr_ack responses are consumed by the CPU.
- **Test completion contract** — define SRAM/MMIO status words so firmware can signal pass/fail/done instead of relying on fixed cycle counts; use it from ASM and C tests, with the existing timeout as fallback.
- **Shared testbench utility layer** — keep growing `test_data_pkg.sv` / `test_sram_block.sv` for reusable path handling and SRAM expected checks while keeping CPU-specific debug, IRQ, and register checks local to `cpu_tb`.
- **Component-oriented SoC integration tests** — add small system firmwares/tests for UART RX/TX, GPIO, timer IRQ, and bus/peripheral behavior; use optional SRAM expected files for side effects such as copied UART input.
- **System Verilator target** — add a Make target to build/run `system_tb` with Verilator, including firmware build and `+FIRMWARE_HEX=...` wiring.
- **ISR injection more flexible / less brittle** — `cpu_tb` currently keys off a specific register value and PC in `trap.s`
- **Control negative cases** — expand beyond current illegal-opcode smoke for bus errors, invalid CSR access, privilege faults as RTL handles them
- **Multi-cycle memory** — run key tests with `MEM_LATENCY>1`
- **C harness** — C tests skip register checks today; rely on `.sram` expected data. Consider checking `x30`/`x31` ASSERT path or a SRAM done word instead of fixed cycle count

## Later (C integration)

- Broader freestanding C tests once asm suite is stable
- Peripherals (UART/LED) via SYS region — covered by `system_tb` smoke, not cputest today
- FENCE and remaining SYSTEM/privileged behavior — add when implemented
- Broader C runtime support beyond simple `start.S`/data-init smoke
- Computation-heavy C tests

## CSR instruction test gaps

- **Whole-CSR write with unchanged data** — CSRRS/CSRRC with `rs1≠x0` but `rs1=0` still take the write path; we only check that the CSR *value* is unchanged, not that a write cycle occurred (no architected side effects on `mie`/`mip` today).
- **Read-only CSR write attempts** — `csrrw`/`csrrs`/`csrrc` to `misa` verify the value stays the same, not that the core suppressed a write or raised an illegal-instruction trap (trap RTL TODO).
- **Per-field CSR side effects** — not modeled; tests cannot distinguish “bit unchanged” vs “field not written”.

## Lower Priority

- **Tests clobber registers with earlier results** — Many tests clobber registers that contain earlier test results. `csr.s` hazard and rs1/x0 blocks overwrite earlier x-reg results; only end-of-test register check. Split tests or add mid-test checkpoints if needed.
- **Rules for ASM test development** -- Register values to be verified should have an easily traceable value (ideally from just a handful of preceding instructions). Try not to clobber registers unless there is a way to verify them first.