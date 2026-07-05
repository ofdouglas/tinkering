# CPU firmware — test gaps & next steps

Initial asm/C suite lives in `cputest/`. Run: `make -C cputest sim`.

## cputest coverage (done)

- RV32I ALU (incl. boundaries), branches, JAL/JALR, AUIPC/LUI
- Load/store all widths, sub-word misalignment, store-data EX forwarding
- Load-use stalls (incl. lh/lhu/sh, back-to-back loads), branch + load-use, pipelined ALU forwarding
- C smoke: CRC32, compiler store regression (`store_hazard`)

## Gaps to close (asm / sim)

- **Tests clobber registers with earlier results** -- Split tests up and *document* not to clobber registers, or find a way to verify registers at multiple points through a test.
- **Rules for ASM test development** -- Register values to be verified should have an easily traceable value (ideally from just a handful of preceding instructions). Try not to clobber registers unless there is a way to verify them first.

- **Multi-cycle memory** — run key tests with `MEM_LATENCY>1`; targets `refresh_decode_ops` during load-use stall
- **Stores** — WB-stage forward into store source; store-then-load same address, no gap
- **Control** — explicit `x0` writeback; invalid-opcode flush (when RTL handles it)
- **C harness** — check `x30`/`x31` on C tests (ASSERT path in `assert.h`); optional SRAM “done” word instead of fixed cycle count


## Later (C integration)

- Broader freestanding C tests once asm suite is stable
- Peripherals (UART/LED) via SYS region — not in cputest today
- FENCE / SYSTEM / traps — RTL TODO; add when implemented
- Basic C runtime support
- Computation-heavy C tests

