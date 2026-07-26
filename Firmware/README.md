# Firmware

Host-side C++14 libraries and unit tests (protocol stacks, CRC reference models, shared data structures).

## Tests

From `Firmware/`:

```sh
make list
make test MODULE=hdlc
make test MODULE=crc
make test
make build
make clean
```

Each testable module lives in its own directory and provides `<module>_test.cpp`. Binaries go to `build/<module>/<module>_test`. Tests use [Google Test](https://github.com/google/googletest) (`gtest_main`).

The first `make test` clones googletest **v1.15.2** into `build/third_party/googletest` (requires `git`, network once). Override with `GTEST_TAG=...` or `make clean` to refetch.

Run a subset of tests:

```sh
make test MODULE=hdlc SIM_ARGS='--gtest_filter=HdlcByteStuff.*'
```

## Layout

| Path | Contents |
|------|----------|
| `hdlc/` | HDLC framing and protocol routing |
| `crc/` | CRC reference (`crcSaeJ1850`, matches FPGA `tb_crc` vectors) |
| `can/` | CAN frame types |
| `data_structures/` | `Span`, ring buffer, etc. |
| `bootloader/` | Boot image headers |
| `logging/` | Logging utilities |
| `mk/` | Shared Makefile rules (`host.mk`, `gtest.mk`) |
| `working.md` | Follow-up test infrastructure TODOs |
| `build/` | Test binaries (gitignored at repo root) |

## Adding a module

1. Create `Firmware/foo/foo_test.cpp` (and library sources as needed).
2. Register in `mk/modules.mk`:

```makefile
MODULES := hdlc crc foo

foo_SRCS := foo/foo_test.cpp foo/foo.cpp
foo_TEST := foo_test
```

3. `make test MODULE=foo`

Includes use the Firmware root: `#include "foo/foo.h"`, `#include "data_structures/span.h"`.
