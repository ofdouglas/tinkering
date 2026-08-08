# C++ Coding Rules / Style Guide

**Background:** Embedded C++ firmware hobby projects, emphasizing platform firmware on MCUs and FPGA softcores. The goal is to develop embedded systems projects and reusable libraries for personal use, at a profesional-ish standard. Note that this entire repository is for hobby use, there are no actual safety requirements or product / service being shipped.

## C++ Rules
* Use -std=c++17 by default.

* Write code suitable for MCUs by default:
  - No dynamic allocation (except during 1-time init at boot)
  - No exceptions
  - No RTTI
  - No std lib components that use any of the above

* Prefer conservative / safety-minded implementation choices by default: compliance with standards like MISRA or AUTOSAR C++ rules is desirable.

* The MCU platform code should be robust, simple, and portable to various hardware, OS (RTOS or bare-metal), and application types.

* Embedded C++ Abstractions:
 - Use structs for basic data containers that have at most a few simple methods (like isValid(), serialize(), deserialize(), clear(), etc).
 - Use classes (all members private) for anything more complex than a basic data container.
 - Use namespaces, but only a few layers at most. Avoid having things in the global namespace.
 - Use basic templates freely but be conservative about complex template expressions or definitions.
 - Use virtual methods or pure interfaces where appropriate. Example: CanDriverInterface (for portability and mockability)
 - Inheritance hierarchies must be very simple and at most several layers deep. No multi-inheritance.
 - Link-seam injection is also an option.
 - Be conservative with the preprocessor.
 - Prefer brace initialization: `uint32_t value{0x3FFU};`

* Code that is strictly on-host (e.g. unit tests) may use any available C++ features.


## File Organization
* Design/Firmware contains reusable, platform-independent libraries.
  - When creating or updating code, consider moving it into Design/Firmware if the code is reusable.

* Use either one of these structures for software component directories:
  - Simple component like Firmware/crc:
  - Firmware/component_name/:
    - /test:    {component_test.cpp, <optional: mock_files, test_infra>}
    - {several .cpp or .h files}

  - Complex component like Firmware/bootloader:
  - Firmware/component_name/:
    - /test:    {component_test.cpp, <optional: mock_files, test_infra>}
    - /subdirA: {several .cpp or .h files}
    - /subdirB: {more c++, or python, data files, scripts, etc}
    - ...       
    - <optional: working.md> // Notes file


## Naming Conventions
namespaces:              snake_case
Enums / Classes:         PascalCase
Class methods:           camelCase
Locals / struct members: snake_case
Class private members:   snake_case_
Enums values:            kEnumValue


## Documentation Comments
* Don't delete comments or TODOs when editing code, unless you are resolving the TODO
* All classes and non-trivial methods definitions should have a doxygen comment:
  - Concise description of what it is for.
  - Documentation of the API contract (in/out params, preconditions, thread-safety concerns, etc, where relevant)
  - You can write more prose if there are non-obvious things to explain, but don't document the obvious.
* Anything in implementation code that is subtle or unclear should have some documentation. But prefer self-documenting code if possible.
* Example style:
```c++

// CRC algorithm name string
// TODO: detect truncation / name overflow
static constexpr size_t kMaxNameLength{16U};
using NameString = StaticString<kMaxNameLength>;

/**
 * @brief Calculate the CRC of a given input data using the bitwise software algorithm.
 *
 * @tparam    CrcAlgorithm Class type defining a specific CRC algorithm
 * @param[in] input        The input data to calculate the CRC of.
 * @return    The CRC of the input data.
 *
 * @todo Handle reflect_in and reflect_out
 * @todo Support incremental processing (update, ... finalize)
 */
 template <typename CrcAlgorithm>
 typename CrcAlgorithm::value_type crcBitwise(util::Span<const uint8_t> input) { ... }
```

## Header Files
* Each header file should start with a #pragma once and a brief documentation comment:
```c++
    #pragma once
    /*
     * @file  ostream_helpers.h
     * @brief Helpers for std::ostream printing, for use in host-side unit tests.
     * @note  This file is not intended for use in production code.
     */
```           
