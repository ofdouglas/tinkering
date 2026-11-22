#pragma once
#include <stdint.h>
#include <stddef.h>

#define HAS_FLOAT 0
#define HAS_STDIO 0
#define HAS_PRINTF 0
#define MAIN_HAS_NOARGC 1
#define MULTITHREAD 1
#define SEED_METHOD SEED_VOLATILE
#define MEM_METHOD MEM_STATIC
#define MEM_LOCATION "Static SRAM data, SRAM code"
#define COMPILER_VERSION "GCC " __VERSION__

typedef int16_t ee_s16;
typedef uint16_t ee_u16;
typedef int32_t ee_s32;
typedef uint32_t ee_u32;
typedef uint8_t ee_u8;
typedef uintptr_t ee_ptr_int;
typedef size_t ee_size_t;
typedef uint32_t CORE_TICKS;  // milliseconds
#define align_mem(p) ((void *)(((uintptr_t)(p) + 3u) & ~(uintptr_t)3u))

typedef struct { ee_u8 portable_id; } core_portable;
extern ee_u32 default_num_contexts;
void portable_init(core_portable *p, int *argc, char *argv[]);
void portable_fini(core_portable *p);
int ee_printf(const char *fmt, ...);
