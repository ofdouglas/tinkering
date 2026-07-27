# Firmware working notes

Follow-up items for a more robust host test setup (not all implemented yet).

## TODO

- **CI**: run `make test` from `Firmware/` in the pipeline (WSL/Linux agent).
- **Sanitizers**: optional `make test` with `CXXFLAGS="-fsanitize=address,undefined -g"`.
- **Shared golden vectors**: share CRC/HDLC vectors between `FPGA/tb/tb_crc.sv` and Firmware tests (generated header or script).
- **Parameterized test template**: document pattern for new `*_test.cpp` modules.
- **gtest filter**: `make test MODULE=hdlc SIM_ARGS='--gtest_filter=HdlcByteStuff.*'`.
- **Multi-byte CRC**: align `crc_test` with FPGA `test_len[]` multi-byte cases.
- **Python parity**: cross-check `hdlc/hdlc.py` against C++ encode/decode tests.
- **Coverage**: optional `gcov` / `llvm-cov` Makefile target.

- **Logging 64-bit integers**: snprintf support for uint64_t/int64_t.