# Firmware working notes

## 1. High Priority TODOs

### Firmware/bootloader
 - Python bindings for C++ protocol and transport layer
 - Python client
 - Python image generator

### Firmware/hdlc
 - Develop C++ Transmitter class (for both host and client)

### Firmware/crc
Features:
 - Support incremental computation (update ... finalize)


## 2. Medium Priority TODOs

## Build System
 - Investigate tests not rebuilding when header dependencies modified
 - Add way to clean make build targets without re-fetching gtest
 - Evalute using Bazel instead of make

### Firmware/crc
Features:
 - Implement lookup table approach
 - API: add a way to select the implementation type
 - Support input / output reflection
Tests:
 - Check against reputable sources
 - Automate test data generation
 - Add test vectors of other lengths
 - Add adversarial test vectors (ex: leading zero, trailing zero, etc.)
 - Test correct output formatting for negative tests / errors

### Firmware/osal
 - High-level design of the Operating System Abstraction Layer
 - Scope additional word

## 3. Low Priority TODOs

### Firmware/util
 - Add bounded integer types (eg. Uint24_s)

### Firmware/math
 - Add saturating arithmetic
 - Add special integer instructions: CLZ, CTZ, popcount, log2, etc

### Firmware/logging
- **Logging 64-bit integers**: snprintf support for uint64_t/int64_t.

### General Testing
- **CI**: run `make test` from `Firmware/` in the pipeline (WSL/Linux agent).
- **Sanitizers**: optional `make test` with `CXXFLAGS="-fsanitize=address,undefined -g"`.

### Firmware/hdlc
- **Shared golden vectors**: share CRC/HDLC vectors between `FPGA/tb/tb_crc.sv` and Firmware tests (generated header or script).
- **Parameterized test template**: document pattern for new `*_test.cpp` modules.
- **gtest filter**: `make test MODULE=hdlc SIM_ARGS='--gtest_filter=HdlcByteStuff.*'`.
- **Multi-byte CRC**: align `crc_test` with FPGA `test_len[]` multi-byte cases.
- **Python parity**: cross-check `hdlc/hdlc.py` against C++ encode/decode tests.
- **Coverage**: optional `gcov` / `llvm-cov` Makefile target.
