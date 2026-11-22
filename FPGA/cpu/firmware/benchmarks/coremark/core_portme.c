#include "coremark.h"

extern void coremark_uart_init(void);
extern uint64_t coremark_milliseconds(void);

ee_u32 default_num_contexts = 1;
#ifdef VALIDATION_RUN
volatile ee_s32 seed1_volatile = 0x3415, seed2_volatile = 0x3415;
#else
volatile ee_s32 seed1_volatile = 0, seed2_volatile = 0;
#endif
volatile ee_s32 seed3_volatile = 0x66, seed4_volatile = 1500, seed5_volatile = 0;

static uint64_t start_ms;
static CORE_TICKS elapsed_ms;
static int validated;

void start_time(void) { start_ms = coremark_milliseconds(); }
void stop_time(void) { elapsed_ms = coremark_milliseconds() - start_ms; }
CORE_TICKS get_time(void) { return elapsed_ms; }
secs_ret time_in_secs(CORE_TICKS ticks) { return ticks / 1000; }

void portable_init(core_portable *p, int *argc, char *argv[]) {
    (void)argc;
    (void)argv;
    coremark_uart_init();
    p->portable_id = 1;
}

// Upstream main returns zero even on failure. Only its validated-results
// branch prints this message, after the CRC, type and duration checks.
void coremark_observe_message(const char *text) {
    const char *success = "Correct operation validated. See README.md for run and reporting rules.\n";
    while (*success && *text == *success) {
        ++success;
        ++text;
    }
    if (!*success && !*text) validated = 1;
}

void portable_fini(core_portable *p) {
    p->portable_id = 0;
    if (validated) {
        ee_u32 milli_score = (uint64_t)seed4_volatile * 1000000 / elapsed_ms;
        ee_printf("Measured iterations/sec: %u.%03u\n", milli_score / 1000, milli_score % 1000);
        ee_printf("CoreMark PASS\n");
    } else {
        ee_printf("CoreMark FAIL\n");
    }
}
