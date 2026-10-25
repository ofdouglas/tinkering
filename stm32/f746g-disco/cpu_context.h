#pragma once

#include <cstdint>

// struct FloatRegisters {
//     float _s[16];
//     uint32_t _fpscr;
// };

// static_assert(sizeof(FloatRegisters) == 68, "FloatRegisters must be 68 bytes");

struct IntegerRegisters {
    static constexpr size_t kTotalNumRegisters = 17;

    static constexpr char kNames[kTotalNumRegisters][4] = {" r0", " r1", " r2", " r3", " r4", " r5",
                                                           " r6", " r7", " r8", " r9", "r10", "r11",
                                                           "r12", " lr", " pc", "psr", " sp"};

    union {
        uint32_t _data[kTotalNumRegisters];  // For array / loop access
        struct {                             // For individual member access
            uint32_t _r[13];                 // r0 - r12
            uint32_t _lr;                    // link register
            uint32_t _pc;                    // program counter
            uint32_t _psr;                   // program status register
            uint32_t _sp;                    // stack pointer
        };
    };
};

static_assert(sizeof(IntegerRegisters) == 68, "Incorrect size of IntegerRegisters");
static_assert(sizeof(IntegerRegisters) == (IntegerRegisters::kTotalNumRegisters * sizeof(uint32_t)),
              "Incorrect size of IntegerRegisters");

/* Basic Exception Frame Quick Reference
   {padding word} <-- For 8-byte alignment
   xPSR  (sp[7])
   PC
   LR
   R12
   R3
   R2
   R1
   R0    (sp[0] -- top of stack in exception frame)
*/

/* EXC_RETURN[7:0] quick reference:
   - 0x_1 - Return to handler mode (MSP)
   - 0x_9 - Return to thread mode (PSP)
   - 0x_D - Return to handler mode (MSP)

   E: Extended frame (FPU registers are present)
   F: Basic frame (integer only)
*/

struct ExcReturnBits {
    static constexpr uint8_t kFrameTypeBit = 0x10;
    static constexpr uint8_t kThreadModeBit = 0x08;
    static constexpr uint8_t kStackTypeBit = 0x04;

    enum class FrameType : uint8_t {
        kExtended = 0,  // Floating point + integer
        kBasic = 1      // Integer only
    };

    enum class StackType : uint8_t {
        kMsp = 0,  // Main stack pointer
        kPsp = 1   // Process stack pointer
    };

    enum class ExecMode : uint8_t {
        kHandler = 0,  // Return to handler mode
        kThread = 1    // Return to thread mode
    };
};

extern "C" void log_crash(const IntegerRegisters* regs);
