# CoreMark score history

Run `make coremark` from the FPGA project root with the RTL revision under test programmed on the board. The target resets the CPU and uploads two SRAM applications over UART. Record the performance image’s `Measured iterations/sec` after both seed sets pass. Use the bootloader’s reported RTL_BUILD_ID to identify the programmed bitstream when available.

These measurements use a Nexys Video at 50 MHz, one RV32I core, SRAM code and static data, a 2,000-byte CoreMark working set, and 1,500 iterations per seed set. GCC 13.4.0 flags are `-march=rv32i_zicsr -mabi=ilp32 -O3 -ffreestanding -nostdlib`. The [board port](../firmware/benchmarks/coremark/README.md) documents the upstream revision and timer.

| Measured | Date added | RTL commit | Fetch configuration | Performance time (s) | CoreMark (iterations/s) | CoreMark/MHz | Validation |
|---|---|---|---|---:|---:|---:|---|
| 2026-11-21 | 2026-11-22 | `ada60943fe23` | Baseline. ROM and SRAM fetch the requested PC and wait for a matching registered response. | 54.197 | 27.676 | 0.554 | PASS |
| 2026-11-22 | 2026-11-22 | `6432c5cd099f` | Pipelined ROM/SRAM fetch. A matching pending fetch issues the next sequential word. A miss requests the current PC. | 39.165 | 38.299 | 0.766 | PASS |
| 2026-11-22 | 2026-11-22 | `6e134d8ebc0e` | Pipelined fetch with static conditional branch prediction in decode. Backward branches predict taken and forward branches predict fall-through. | 37.969 | 39.505 | 0.790 | PASS |

The pipelined-fetch-only board run used the same performance and validation firmware CRC32s as the baseline (`0x01666959` and `0x0ecb6b1f`). The pipelined build’s `system.bit` SHA-256 was `163c0e2e0eed283e824a22b42722cc5408e868baf6c684456593f72156bea727`.

Both static-prediction runs reported RTL_BUILD_ID `6e134d8e`, matching the synthesis log. The amended commit `0ce6ede50d6e` has the same RTL and bootloader sources. The performance and validation images had CRC32s `0xd7d4528d` and `0x00d581f1`, respectively.
