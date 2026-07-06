# CPU firmware — test gaps & next steps

Initial asm/C suite lives in `cputest/`. Run: `make -C cputest sim`.

## cputest coverage (done)

- RV32I ALU (incl. boundaries), branches, JAL/JALR, AUIPC/LUI
- Load/store all widths, sub-word misalignment, store-data EX forwarding
- Load-use stalls (incl. lh/lhu/sh, back-to-back loads), branch + load-use, pipelined ALU forwarding
- C smoke: CRC32, compiler store regression (`store_hazard`)
- CSR read/write (`csr_basic`, `csr_immediate`, `csr_mixed`): CSRRW/CSRRS/CSRRC and CSRRWI/CSRRSI/CSRRCI on implemented CSRs; load-use and ALU→CSR operand hazards, CSR rd→branch/store/CSR-rs1, CSR WB + load-use overlap, immediate/register CSR mix
- Spike ISA cross-check: `make spike-ref` in `cputest/` (RV32I asm tests vs `.expected`; skips CSR and PC-at-0x0 tests — see `SPIKE_SKIP_TESTS` in `tests.mk`)

## Gaps to close (asm / sim)

- **Control** — invalid-opcode flush (when RTL handles it)
- **Multi-cycle memory** — run key tests with `MEM_LATENCY>1`;
- **C harness** — check `x30`/`x31` on C tests (ASSERT path in `assert.h`); optional SRAM “done” word instead of fixed cycle count

## Later (C integration)

- Broader freestanding C tests once asm suite is stable
- Peripherals (UART/LED) via SYS region — not in cputest today
- FENCE / SYSTEM / traps — RTL TODO; add when implemented
- Basic C runtime support
- Computation-heavy C tests

## CSR instruction test gaps

- **Whole-CSR write with unchanged data** — CSRRS/CSRRC with `rs1≠x0` but `rs1=0` still take the write path; we only check that the CSR *value* is unchanged, not that a write cycle occurred (no architected side effects on `mie`/`mip` today).
- **Read-only CSR write attempts** — `csrrw`/`csrrs`/`csrrc` to `misa` verify the value stays the same, not that the core suppressed a write or raised an illegal-instruction trap (trap RTL TODO).
- **Per-field CSR side effects** — not modeled; tests cannot distinguish “bit unchanged” vs “field not written”.

## Lower Priority

- **Tests clobber registers with earlier results** — Many tests clobber registers that contain earlier test results. `csr.s` hazard and rs1/x0 blocks overwrite earlier x-reg results; only end-of-test register check. Split tests or add mid-test checkpoints if needed.
- **Rules for ASM test development** -- Register values to be verified should have an easily traceable value (ideally from just a handful of preceding instructions). Try not to clobber registers unless there is a way to verify them first.