# CoreMark on the RV32I FPGA

From the FPGA project root, run the performance and validation images in sequence.

```sh
make -C cpu/firmware board-test BOARD_TEST=coremark
make -C cpu/firmware board-test BOARD_TEST=coremark_validation
```

Both images use the upstream 2,000-byte static working set, one context, and
1,500 iterations. They differ only in their volatile seed values. The runner
allows 180 seconds per image after boot and prints the benchmark's UART report.
Both images must pass for a comparable performance result.

The port measures elapsed milliseconds using the machine timer. Upstream's
integer-only output rounds elapsed time down to whole seconds. Use the
performance run's `Measured iterations/sec` line for the score calculated
from elapsed milliseconds. UART reporting takes place after timing.

## External source

The build reads CoreMark from `~/Design/third_party/coremark`. To provision
a new machine, check out the pinned revision once:

```sh
mkdir -p ~/Design/third_party
git clone https://github.com/eembc/coremark.git ~/Design/third_party/coremark
git -C ~/Design/third_party/coremark checkout --detach b56889a7bd9d5624fbeff2a73ba69878e6a9c146
```

Set `THIRD_PARTY_DIR` to use a different dependency root. The build applies
`ee_printf.patch` to the upstream barebones formatter in the build directory
to forward UART output and recognize the benchmark verdict.
