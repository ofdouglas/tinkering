# Google Test (host)

Host-side C++ unit tests for STM32 project code (e.g. `motor-controller/Inc`).

Google Test is fetched at configure time via CMake `FetchContent` (tag `v1.15.2`). Requires **git**, **cmake** (3.16+), and a C++17 compiler on the build machine.

## Build (WSL / Linux)

```bash
cd /mnt/c/Design/MCU/STM32/gtest
./build.sh
```

Or manually:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run a single binary:

```bash
./build/motor_controller_tests
```

## Layout

| Path | Purpose |
|------|---------|
| `CMakeLists.txt` | Fetches googletest, defines test targets |
| `tests/smoke_test.cpp` | Verifies gtest wiring |
| `tests/ring_buffer_test.cpp` | Example tests for `motor-controller/Inc/ring_buffer.h` |
| `build/` | Out-of-tree build (gitignored) |

## Adding tests

1. Add `tests/your_feature_test.cpp`.
2. Register in `CMakeLists.txt`:

```cmake
add_executable(your_feature_tests tests/your_feature_test.cpp)
target_include_directories(your_feature_tests PRIVATE "${MOTOR_CONTROLLER_INC}")
target_link_libraries(your_feature_tests PRIVATE GTest::gtest_main)
gtest_discover_tests(your_feature_tests)
```

Keep tests limited to headers and logic that do not need HAL, FreeRTOS, or the ARM toolchain.
