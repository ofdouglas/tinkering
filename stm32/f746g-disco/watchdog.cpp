#include "watchdog.h"

#include "stm32f746xx.h"

void Watchdog::init() noexcept {
    IWDG->KR = 0x5555U;
    IWDG->KR = 0xCCCCU;
}

void Watchdog::kick() noexcept {
    IWDG->KR = 0xAAAAU;
}
