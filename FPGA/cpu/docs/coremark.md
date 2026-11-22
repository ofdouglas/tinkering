# CoreMark score history

Run `make coremark` from the FPGA project root with the RTL revision under test programmed on the board. The target resets the CPU and uploads two SRAM applications over UART. Record the performance image’s `Measured iterations/sec` after both seed sets pass. Use the RTL commit that produced the programmed bitstream.

These measurements use a Nexys Video at 50 MHz, one RV32I core, SRAM code and static data, a 2,000-byte CoreMark working set, and 1,500 iterations per seed set. GCC 13.4.0 flags are `-march=rv32i_zicsr -mabi=ilp32 -O3 -ffreestanding -nostdlib`. The [board port](../firmware/benchmarks/coremark/README.md) documents the upstream revision and timer.

| Measured | Date added | RTL commit | Fetch configuration | Performance time (s) | CoreMark (iterations/s) | CoreMark/MHz | Validation |
|---|---|---|---|---:|---:|---:|---|
| 2026-11-21 | 2026-11-22 | `ada60943fe23` | Baseline. ROM and SRAM fetch the requested PC and wait for a matching registered response. | 54.197 | 27.676 | 0.554 | PASS |
| 2026-11-22 | 2026-11-22 | `6432c5cd099f` | Pipelined ROM/SRAM fetch. A matching pending fetch issues the next sequential word. A miss requests the current PC. | 39.165 | 38.299 | 0.766 | PASS |

The November 22 board run used the same performance and validation firmware CRC32s as the baseline (`0x01666959` and `0x0ecb6b1f`). The pipelined build’s `system.bit` SHA-256 was `163c0e2e0eed283e824a22b42722cc5408e868baf6c684456593f72156bea727`.
