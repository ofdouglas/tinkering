# Agents (FPGA/cpu)

Read `docs/README.md` first.

## test-fw assembly

`cpu_tb` checks `x1`–`x31` (and optional SRAM) only at end of test.

- `/* verify xN = … */` must still hold at end of test, or that value must be stored to a word listed in `expected/<test>.sram`.
- Do not reuse a verified destination until it has been snapshotted. Snapshot at block boundaries only, never inside a 0-gap hazard sequence.
- Scratch registers must not have `verify` comments.
- Fail-path poisons (`addi xN, -1` on a taken-wrong path) are allowed; they are not result clobbers.
- Prefer unique destinations. Split a file only when one register file cannot hold every live result plus a snapshot pointer.
